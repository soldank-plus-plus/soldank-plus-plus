#include <glad/glad.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

import Extern.Glm;
import SpritesManager;
import BulletRenderer;
import SoldierRenderer;
import Shared.Core.World;
import Shared.Core.State.StateManager;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.TeamType;
import HudSpriteRenderer;
import WeaponVisualEffects;
import Shared.Core.Types.WeaponType;

using namespace Soldank;

namespace
{
class VisualEffectsRenderingTest : public testing::Test
{
protected:
    void SetUp() override
    {
        const auto get_display = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
          eglGetProcAddress("eglGetPlatformDisplayEXT"));
        if (get_display == nullptr) {
            GTEST_SKIP() << "Surfaceless EGL is unavailable";
        }
        display_ = get_display(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
        if (display_ == EGL_NO_DISPLAY || !eglInitialize(display_, nullptr, nullptr)) {
            GTEST_SKIP() << "Software EGL display is unavailable";
        }
        ASSERT_TRUE(eglBindAPI(EGL_OPENGL_API));
        const EGLint config_attributes[]{ EGL_SURFACE_TYPE,
                                          EGL_PBUFFER_BIT,
                                          EGL_RENDERABLE_TYPE,
                                          EGL_OPENGL_BIT,
                                          EGL_RED_SIZE,
                                          8,
                                          EGL_GREEN_SIZE,
                                          8,
                                          EGL_BLUE_SIZE,
                                          8,
                                          EGL_ALPHA_SIZE,
                                          8,
                                          EGL_NONE };
        EGLConfig config{};
        EGLint count = 0;
        ASSERT_TRUE(eglChooseConfig(display_, config_attributes, &config, 1, &count));
        ASSERT_GT(count, 0);
        const EGLint surface_attributes[]{ EGL_WIDTH, SIZE, EGL_HEIGHT, SIZE, EGL_NONE };
        surface_ = eglCreatePbufferSurface(display_, config, surface_attributes);
        ASSERT_NE(surface_, EGL_NO_SURFACE);
        context_ = eglCreateContext(display_, config, EGL_NO_CONTEXT, nullptr);
        ASSERT_NE(context_, EGL_NO_CONTEXT);
        ASSERT_TRUE(eglMakeCurrent(display_, surface_, surface_, context_));
        ASSERT_NE(gladLoadGLLoader(reinterpret_cast<GLADloadproc>(eglGetProcAddress)), 0);
        glViewport(0, 0, SIZE, SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.0F, 0.0F, 0.0F, 0.0F);
        projection_ = glm::ortho(-64.0F, 64.0F, -64.0F, 64.0F);
        sprites_ = std::make_unique<Sprites::SpriteManager>();
        renderer_ = std::make_unique<BulletRenderer>(*sprites_);
        ASSERT_EQ(glGetError(), GL_NO_ERROR);
    }
    void TearDown() override
    {
        renderer_.reset();
        sprites_.reset();
        if (display_ != EGL_NO_DISPLAY) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context_ != EGL_NO_CONTEXT) {
                eglDestroyContext(display_, context_);
            }
            if (surface_ != EGL_NO_SURFACE) {
                eglDestroySurface(display_, surface_);
            }
            eglTerminate(display_);
        }
    }
    static constexpr int SIZE = 256;
    static std::vector<std::uint8_t> ReadPixels()
    {
        std::vector<std::uint8_t> pixels(SIZE * SIZE * 4);
        glReadPixels(0, 0, SIZE, SIZE, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        return pixels;
    }
    EGLDisplay display_{ EGL_NO_DISPLAY };
    EGLSurface surface_{ EGL_NO_SURFACE };
    EGLContext context_{ EGL_NO_CONTEXT };
    glm::mat4 projection_{ 1.0F };
    std::unique_ptr<Sprites::SpriteManager> sprites_;
    std::unique_ptr<BulletRenderer> renderer_;
};
} // namespace

TEST_F(VisualEffectsRenderingTest, BatchingPreservesPixelsAndTransparentDrawOrder)
{
    const std::array effects{ WeaponVisualEffect{ .kind = WeaponVisualEffectKind::Smoke,
                                                  .position = { -4.0F, -4.0F },
                                                  .old_position = { -4.0F, -4.0F },
                                                  .lifetime = 50,
                                                  .initial_lifetime = 50 },
                              WeaponVisualEffect{ .kind = WeaponVisualEffectKind::JetFire,
                                                  .lifetime = 30,
                                                  .initial_lifetime = 40,
                                                  .tint = { 0.3F, 0.7F, 1.0F, 1.0F } },
                              WeaponVisualEffect{ .kind = WeaponVisualEffectKind::Smoke,
                                                  .lifetime = 30,
                                                  .initial_lifetime = 50 },
                              WeaponVisualEffect{ .kind = WeaponVisualEffectKind::Explosion,
                                                  .weapon = WeaponType::FragGrenade,
                                                  .lifetime = 20,
                                                  .initial_lifetime = 48 } };
    glClear(GL_COLOR_BUFFER_BIT);
    for (const auto& effect : effects) {
        renderer_->RenderEffect(projection_, effect, 0.5F);
    }
    const auto immediate = ReadPixels();
    ASSERT_TRUE(std::ranges::any_of(immediate, [](auto pixel) { return pixel != 0; }));
    glClear(GL_COLOR_BUFFER_BIT);
    renderer_->BeginBatch(projection_);
    for (const auto& effect : effects) {
        renderer_->RenderEffect(projection_, effect, 0.5F);
    }
    renderer_->EndBatch();
    const auto batched = ReadPixels();
    // CPU and shader matrix arithmetic can differ by a subpixel at texture edges.
    std::size_t different = 0;
    for (std::size_t index = 0; index < immediate.size(); ++index) {
        if (std::abs(static_cast<int>(immediate[index]) - static_cast<int>(batched[index])) > 1) {
            ++different;
        }
    }
    EXPECT_LT(different, immediate.size() / 1000);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(VisualEffectsRenderingTest, EveryNewEffectFamilyHasADrawableTexture)
{
    for (const auto kind : { WeaponVisualEffectKind::SmallBlood,
                             WeaponVisualEffectKind::Blood,
                             WeaponVisualEffectKind::Splat,
                             WeaponVisualEffectKind::Flame,
                             WeaponVisualEffectKind::BlackSmoke,
                             WeaponVisualEffectKind::Exhaust,
                             WeaponVisualEffectKind::JetFire,
                             WeaponVisualEffectKind::Spawn,
                             WeaponVisualEffectKind::Pin,
                             WeaponVisualEffectKind::Debris,
                             WeaponVisualEffectKind::Cloth,
                             WeaponVisualEffectKind::Rain,
                             WeaponVisualEffectKind::Sand,
                             WeaponVisualEffectKind::Snow,
                             WeaponVisualEffectKind::Cigar,
                             WeaponVisualEffectKind::Spit,
                             WeaponVisualEffectKind::Match,
                             WeaponVisualEffectKind::Urine,
                             WeaponVisualEffectKind::Headgear,
                             WeaponVisualEffectKind::Ember }) {
        SCOPED_TRACE(static_cast<int>(kind));
        glClear(GL_COLOR_BUFFER_BIT);
        const WeaponVisualEffect effect{ .kind = kind, .lifetime = 30, .initial_lifetime = 40 };
        renderer_->RenderEffect(projection_, effect, 1.0F);
        const auto pixels = ReadPixels();
        EXPECT_TRUE(std::ranges::any_of(pixels, [](auto pixel) { return pixel != 0; }));
        EXPECT_EQ(glGetError(), GL_NO_ERROR);
    }
}

TEST_F(VisualEffectsRenderingTest, HudBarsClipAndEmptyBarsDrawNothing)
{
    HudSpriteRenderer hud;
    glClear(GL_COLOR_BUFFER_BIT);
    hud.Render(HudSprite::HealthBar, { 0.0F, 0.0F }, { 256.0F, 192.0F }, 0.0F);
    const auto empty = ReadPixels();
    EXPECT_TRUE(std::ranges::all_of(empty, [](auto pixel) { return pixel == 0; }));
    hud.Render(HudSprite::HealthBar, { 0.0F, 0.0F }, { 256.0F, 192.0F }, 1.0F);
    const auto full = ReadPixels();
    const auto full_count = std::ranges::count_if(full, [](auto pixel) { return pixel != 0; });
    ASSERT_GT(full_count, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    hud.Render(HudSprite::HealthBar, { 0.0F, 0.0F }, { 256.0F, 192.0F }, 0.5F);
    const auto half = ReadPixels();
    const auto half_count = std::ranges::count_if(half, [](auto pixel) { return pixel != 0; });
    EXPECT_GT(half_count, 0);
    EXPECT_LT(half_count, full_count);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(VisualEffectsRenderingTest, DreadlocksAndTeamVariantsRenderForBothDirections)
{
    World world;
    world.CreateSoldier(1);
    world.SpawnSoldier(1, glm::vec2{});
    world.Update(1.0 / 60.0);
    auto& soldier = world.GetStateManager()->GetSoldier(1);
    SoldierRenderer soldier_renderer(*sprites_);
    for (const int direction : { 1, -1 }) {
        for (const auto team : { TeamType::None, TeamType::Bravo, TeamType::Delta }) {
            soldier.direction = static_cast<std::int8_t>(direction);
            soldier.team = team;
            soldier.appearance.hair_style = HairStyle::None;
            glClear(GL_COLOR_BUFFER_BIT);
            soldier_renderer.Render(projection_, *sprites_, soldier, 1.0F);
            const auto bald = ReadPixels();
            soldier.appearance.hair_style = HairStyle::Dreadlocks;
            glClear(GL_COLOR_BUFFER_BIT);
            soldier_renderer.Render(projection_, *sprites_, soldier, 1.0F);
            const auto dreadlocks = ReadPixels();
            EXPECT_NE(bald, dreadlocks);
            EXPECT_EQ(glGetError(), GL_NO_ERROR);
        }
    }
}

TEST_F(VisualEffectsRenderingTest, InvisibleSoldierDoesNotLeaveOpaqueWeapons)
{
    World world;
    world.CreateSoldier(1);
    world.SpawnSoldier(1, glm::vec2{});
    world.Update(1.0 / 60.0);
    auto& soldier = world.GetStateManager()->GetSoldier(1);
    soldier.alpha = 0;
    SoldierRenderer soldier_renderer(*sprites_);
    glClear(GL_COLOR_BUFFER_BIT);
    soldier_renderer.Render(projection_, *sprites_, soldier, 1.0F);
    const auto pixels = ReadPixels();
    EXPECT_TRUE(std::ranges::all_of(pixels, [](auto pixel) { return pixel == 0; }));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
}
