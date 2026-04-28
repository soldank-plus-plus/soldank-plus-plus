module;

#include "core/math/Glm.hpp"

#include <memory>
#include <functional>
#include <optional>

export module Shared.Core.IWorld;

import Shared.Core.State.StateManager;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.WorldEvents;
import Shared.Core.Animations;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
class IWorld
{
protected:
    using TShouldStopGameLoopCallback = std::function<bool()>;
    using TPreGameLoopIterationCallback = std::function<void()>;
    using TPreWorldUpdateCallback = std::function<void()>;
    using TPostWorldUpdateCallback = std::function<void(const StateManager& state)>;
    using TPostGameLoopIterationCallback = std::function<void(const StateManager&, double, int)>;

    using TPreSoldierUpdateCallback = std::function<bool(const Soldier&)>;
    using TPreProjectileSpawnCallback = std::function<bool(const BulletParams&)>;

public:
    virtual ~IWorld() = default;
    virtual void RunLoop() = 0;
    virtual void Update(double delta_time) = 0;
    virtual void UpdateSoldier(unsigned int soldier_id) = 0;
    virtual const std::shared_ptr<StateManager>& GetStateManager() const = 0;
    virtual const Soldier& GetSoldier(unsigned int soldier_id) const = 0;
    virtual PhysicsEvents& GetPhysicsEvents() = 0;
    virtual WorldEvents& GetWorldEvents() = 0;
    virtual std::shared_ptr<const AnimationData> GetAnimationData(
      AnimationType animation_type) const = 0;
    virtual std::shared_ptr<AnimationState> GetBodyAnimationState(
      AnimationType animation_type) const = 0;
    virtual std::shared_ptr<AnimationState> GetLegsAnimationState(
      AnimationType animation_type) const = 0;

    virtual const Soldier& CreateSoldier(
      std::optional<unsigned int> force_soldier_id = std::nullopt) = 0;
    virtual glm::vec2 SpawnSoldier(unsigned int soldier_id,
                                   std::optional<glm::vec2> spawn_position = std::nullopt) = 0;
    virtual void KillSoldier(unsigned int soldier_id) = 0;
    virtual void HitSoldier(unsigned int soldier_id, float damage) = 0;

    virtual void UpdateWeaponChoices(unsigned int soldier_id,
                                     WeaponType primary_weapon_type,
                                     WeaponType secondary_weapon_type) = 0;

    virtual void SetShouldStopGameLoopCallback(TShouldStopGameLoopCallback callback) = 0;
    virtual void SetPreGameLoopIterationCallback(TPreGameLoopIterationCallback callback) = 0;
    virtual void SetPreWorldUpdateCallback(TPreWorldUpdateCallback callback) = 0;
    virtual void SetPostWorldUpdateCallback(TPostWorldUpdateCallback callback) = 0;
    virtual void SetPostGameLoopIterationCallback(TPostGameLoopIterationCallback callback) = 0;

    virtual void SetPreSoldierUpdateCallback(TPreSoldierUpdateCallback callback) = 0;
    virtual void SetPreProjectileSpawnCallback(TPreProjectileSpawnCallback callback) = 0;

    virtual void SetFPSLimit(int new_fps_limit) = 0;
};
} // namespace Soldank
