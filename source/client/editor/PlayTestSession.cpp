module;

#include "core/math/Glm.hpp"

#include <cstdint>

export module Editor.PlayTestSession;

import Application.Window;
import MapEditor;
import ClientState;

import Shared.Core.IWorld;
import Shared.Core.State.StateManager;

export namespace Soldank
{
class PlayTestSession
{
public:
    bool IsActive() const { return is_active_; }

    void Start(ClientState& client_state, IWorld& world, Window& window, MapEditor& map_editor)
    {
        if (client_state.client_soldier_id.has_value()) {
            std::uint8_t client_soldier_id = *client_state.client_soldier_id;
            bool is_soldier_active =
              world.GetStateManager()->GetSoldier(client_soldier_id).active;
            bool is_soldier_alive =
              !world.GetStateManager()->GetSoldier(client_soldier_id).dead_meat;

            if (!is_soldier_active || !is_soldier_alive) {
                world.SpawnSoldier(*client_state.client_soldier_id);
            }
        }

        client_state.draw_game_interface = true;
        client_state.draw_map_editor_interface = false;
        client_state.draw_game_debug_interface = true;
        client_state.camera_component.ResetZoom();
        world.GetStateManager()->UnPauseGame();
        RegenerateMapSectorsAndMoveSoldiers(world);
        window.SetCursorMode(CursorMode::Locked);
        map_editor.Lock();
        is_active_ = true;
    }

    void Stop(ClientState& client_state, IWorld& world, Window& window, MapEditor& map_editor)
    {
        client_state.draw_game_interface = false;
        client_state.draw_map_editor_interface = true;
        client_state.draw_game_debug_interface = false;
        world.GetStateManager()->PauseGame();
        window.SetCursorMode(CursorMode::Normal);
        map_editor.Unlock();
        is_active_ = false;
    }

private:
    static void RegenerateMapSectorsAndMoveSoldiers(IWorld& world)
    {
        auto& map = world.GetStateManager()->GetMap();
        if (map.GetPolygons().empty()) {
            return;
        }

        auto vertex = map.GetPolygons().at(0).vertices.at(0);
        glm::vec2 old_polygon_position = { vertex.x, vertex.y };

        map.GenerateSectors();

        vertex = map.GetPolygons().at(0).vertices.at(0);
        glm::vec2 move_offset = { vertex.x - old_polygon_position.x,
                                  vertex.y - old_polygon_position.y };

        world.GetStateManager()->TransformSoldiers([&](auto& soldier) {
            world.GetStateManager()->MoveSoldier(soldier.id, move_offset);
        });
    }

    bool is_active_ = false;
};
} // namespace Soldank
