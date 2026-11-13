module;

#include "rendering/shaders/ShaderSources.hpp"
#include <glad/glad.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

export module MinimapRenderer;

import Extern.Glm;
import Renderer;
import Shader;
import Shared.Core.State.StateManager;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.ItemType;

export namespace Soldank
{
class MinimapRenderer
{
public:
    MinimapRenderer()
        : shader_(ShaderSources::NO_TEXTURE_VERTEX_SHADER_SOURCE,
                  ShaderSources::NO_TEXTURE_FRAGMENT_SHADER_SOURCE)
    {
        glGenBuffers(1, &map_vbo_);
        glGenBuffers(1, &markers_vbo_);
    }
    ~MinimapRenderer()
    {
        Renderer::FreeVBO(map_vbo_);
        Renderer::FreeVBO(markers_vbo_);
    }
    MinimapRenderer(const MinimapRenderer&) = delete;
    MinimapRenderer& operator=(const MinimapRenderer&) = delete;
    MinimapRenderer(MinimapRenderer&&) = delete;
    MinimapRenderer& operator=(MinimapRenderer&&) = delete;

    void Render(const StateManager& state, glm::vec2 viewport, std::optional<std::uint8_t> local_id)
    {
        if (viewport.x <= 0.0F || viewport.y <= 0.0F) {
            return;
        }
        const auto& map = state.GetConstMap();
        auto fingerprint = map.GetPolygonFingerprint();
        // Collision fingerprints omit vertex colors, which are visible in the minimap.
        for (const auto& polygon : map.GetPolygons()) {
            for (const auto& vertex : polygon.vertices) {
                const auto& color = vertex.color;
                const auto rgba = static_cast<std::uint32_t>(color.red) |
                                  (static_cast<std::uint32_t>(color.green) << 8U) |
                                  (static_cast<std::uint32_t>(color.blue) << 16U) |
                                  (static_cast<std::uint32_t>(color.alpha) << 24U);
                fingerprint = (fingerprint ^ rgba) * 1099511628211ULL;
            }
        }
        if (!fingerprint_ || fingerprint != *fingerprint_) {
            fingerprint_ = fingerprint;
            const auto bounds = map.GetBoundaries();
            minimum_ = { bounds[2], bounds[0] };
            extent_ =
              glm::max(glm::vec2(bounds[3] - bounds[2], bounds[1] - bounds[0]), glm::vec2(1.0F));
            std::vector<float> vertices;
            AppendQuad(vertices, {}, { 1.0F, 1.0F }, { 0.0F, 0.0F, 0.0F, 0.4F });
            for (const auto& polygon : map.GetPolygons()) {
                for (const auto& vertex : polygon.vertices) {
                    const auto position = (glm::vec2(vertex.x, vertex.y) - minimum_) / extent_;
                    const auto& color = vertex.color;
                    AppendVertex(vertices,
                                 position,
                                 { color.red / 255.0F,
                                   color.green / 255.0F,
                                   color.blue / 255.0F,
                                   color.alpha / 255.0F });
                }
            }
            vertex_count_ = static_cast<int>(vertices.size() / 7);
            Upload(map_vbo_, vertices);
        }
        const float ui_scale = std::min(viewport.x / 640.0F, viewport.y / 480.0F);
        const glm::vec2 size(140.0F * ui_scale, 90.0F * ui_scale);
        const float fit = std::min(size.x / extent_.x, size.y / extent_.y);
        const glm::vec2 fitted = extent_ * fit;
        const glm::vec2 origin(viewport.x - fitted.x - 12.0F * ui_scale, 12.0F * ui_scale);
        auto transform = glm::ortho(0.0F, viewport.x, viewport.y, 0.0F);
        transform = glm::translate(transform, glm::vec3(origin, 0.0F));
        transform = glm::scale(transform, glm::vec3(fitted, 1.0F));
        shader_.Use();
        shader_.SetMatrix4("transform", transform);
        Renderer::SetupVertexArray(map_vbo_, std::nullopt, true, false);
        Renderer::DrawArrays(GL_TRIANGLES, 0, vertex_count_);
        std::vector<float> markers;
        const auto* local =
          local_id ? state.FindSoldier([&](const auto& soldier) { return soldier.id == *local_id; })
                   : nullptr;
        state.ForEachSoldier([&](const auto& soldier) {
            if (!soldier.IsAlive() || (state.IsRealistic() && soldier.visible == 0 &&
                                       (!local_id || soldier.id != *local_id))) {
                return;
            }
            if (local != nullptr && soldier.id != local->id &&
                (local->GetTeam() == TeamType::None || soldier.GetTeam() != local->GetTeam())) {
                return;
            }
            const auto position = (soldier.particle.position - minimum_) / extent_;
            const glm::vec2 radius = glm::vec2(2.5F * ui_scale) / fitted;
            AppendQuad(markers,
                       position - radius,
                       radius * 2.0F,
                       local_id && soldier.id == *local_id ? glm::vec4(1.0F)
                                                           : glm::vec4(0.3F, 0.8F, 1.0F, 1.0F));
        });
        state.ForEachItem([&](const auto& item) {
            if (!item.active || !IsItemTypeFlag(item.style)) {
                return;
            }
            const auto position = (item.skeleton->GetPos(1) - minimum_) / extent_;
            const glm::vec2 radius = glm::vec2(2.0F * ui_scale) / fitted;
            AppendQuad(markers,
                       position - radius,
                       radius * 2.0F,
                       item.style == ItemType::AlphaFlag ? glm::vec4(1.0F, 0.2F, 0.2F, 1.0F)
                                                         : glm::vec4(0.2F, 0.4F, 1.0F, 1.0F));
        });
        if (!markers.empty()) {
            Upload(markers_vbo_, markers);
            Renderer::SetupVertexArray(markers_vbo_, std::nullopt, true, false);
            Renderer::DrawArrays(GL_TRIANGLES, 0, static_cast<int>(markers.size() / 7));
        }
    }

private:
    static void AppendVertex(std::vector<float>& vertices, glm::vec2 position, glm::vec4 color)
    {
        vertices.insert(vertices.end(),
                        { position.x, position.y, 1.0F, color.r, color.g, color.b, color.a });
    }
    static void AppendQuad(std::vector<float>& vertices,
                           glm::vec2 p,
                           glm::vec2 size,
                           glm::vec4 color)
    {
        for (const auto corner : { glm::vec2(0.0F),
                                   glm::vec2(1.0F, 0.0F),
                                   glm::vec2(0.0F, 1.0F),
                                   glm::vec2(1.0F, 0.0F),
                                   glm::vec2(1.0F),
                                   glm::vec2(0.0F, 1.0F) }) {
            AppendVertex(vertices, p + corner * size, color);
        }
    }
    static void Upload(unsigned int vbo, const std::vector<float>& vertices)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                     vertices.data(),
                     GL_DYNAMIC_DRAW);
    }
    Shader shader_;
    unsigned int map_vbo_{};
    unsigned int markers_vbo_{};
    int vertex_count_{};
    std::optional<std::uint64_t> fingerprint_;
    glm::vec2 minimum_{};
    glm::vec2 extent_{ 1.0F };
};
} // namespace Soldank
