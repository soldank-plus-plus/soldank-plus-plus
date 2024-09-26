#ifndef __WORLD_HPP__
#define __WORLD_HPP__

#include "core/IWorld.hpp"
#include "core/state/StateManager.hpp"
#include "core/entities/Soldier.hpp"
#include "core/physics/PhysicsEvents.hpp"
#include "core/WorldEvents.hpp"
#include "core/animations/AnimationData.hpp"

#include <functional>
#include <utility>
#include <random>
#include <optional>

namespace Soldank
{
class World final : public IWorld
{
public:
    World();

    void RunLoop(int fps_limit) final;
    void Update(double delta_time) final;
    void UpdateSoldier(unsigned int soldier_id) final;
    const std::shared_ptr<StateManager>& GetStateManager() const final;
    const Soldier& GetSoldier(unsigned int soldier_id) const final;
    PhysicsEvents& GetPhysicsEvents() final;
    WorldEvents& GetWorldEvents() final;
    std::shared_ptr<const AnimationData> GetAnimationData(
      AnimationType animation_type) const override;
    std::shared_ptr<AnimationState> GetBodyAnimationState(AnimationType animation_type) const final;
    std::shared_ptr<AnimationState> GetLegsAnimationState(AnimationType animation_type) const final;

    const Soldier& CreateSoldier(std::optional<unsigned int> force_soldier_id = std::nullopt) final;
    glm::vec2 SpawnSoldier(unsigned int soldier_id,
                           std::optional<glm::vec2> spawn_position = std::nullopt) final;
    void KillSoldier(unsigned int soldier_id) final;
    void HitSoldier(unsigned int soldier_id, float damage) final;

    void UpdateWeaponChoices(unsigned int soldier_id,
                             WeaponType primary_weapon_type,
                             WeaponType secondary_weapon_type) final;

    void SetShouldStopGameLoopCallback(TShouldStopGameLoopCallback callback) final
    {
        should_stop_game_loop_callback_ = std::move(callback);
    }

    void SetPreGameLoopIterationCallback(TPreGameLoopIterationCallback callback) final
    {
        pre_game_loop_iteration_callback_ = std::move(callback);
    }

    void SetPreWorldUpdateCallback(TPreWorldUpdateCallback callback) final
    {
        pre_world_update_callback_ = std::move(callback);
    }

    void SetPostWorldUpdateCallback(TPostWorldUpdateCallback callback) final
    {
        post_world_update_callback_ = std::move(callback);
    }

    void SetPostGameLoopIterationCallback(TPostGameLoopIterationCallback callback) final
    {
        post_game_loop_iteration_callback_ = std::move(callback);
    }

    void SetPreSoldierUpdateCallback(TPreSoldierUpdateCallback callback) final
    {
        pre_soldier_update_callback_ = std::move(callback);
    }

    void SetPreProjectileSpawnCallback(TPreProjectileSpawnCallback callback) final
    {
        pre_projectile_spawn_callback_ = std::move(callback);
    }

private:
    std::shared_ptr<StateManager> state_manager_;
    std::unique_ptr<PhysicsEvents> physics_events_;
    std::unique_ptr<WorldEvents> world_events_;

    TShouldStopGameLoopCallback should_stop_game_loop_callback_;
    TPreGameLoopIterationCallback pre_game_loop_iteration_callback_;
    TPreWorldUpdateCallback pre_world_update_callback_;
    TPostWorldUpdateCallback post_world_update_callback_;
    TPostGameLoopIterationCallback post_game_loop_iteration_callback_;

    TPreSoldierUpdateCallback pre_soldier_update_callback_;
    TPreProjectileSpawnCallback pre_projectile_spawn_callback_;

    std::random_device random_device_;
    std::mt19937 mersenne_twister_engine_;

    AnimationDataManager animation_data_manager_;
};

} // namespace Soldank

#endif
