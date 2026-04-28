module;

#include "core/math/Glm.hpp"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <random>
#include <ranges>
#include <memory>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <chrono>
#include <functional>
#include <utility>
#include <optional>

export module Shared.Core.World;

import Shared.Core.IWorld;
import Shared.Core.WorldEvents;
import Shared.Core.Physics.BulletPhysics;
import Shared.Core.Physics.ItemPhysics;
import Shared.Core.Physics.SoldierPhysics;
import Shared.Core.Physics.SoldierSkeletonPhysics;
import Shared.Core.State.StateManager;
import Shared.Core.State.State;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Physics.PhysicsEvents;

import Shared.Core.Animations;
import Shared.Core.Animations.States;

import Shared.Core.Map.PMSEnums;
import Shared.Core.Physics.Particles;
import Shared.Core.State.Control;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
class World final : public IWorld
{
public:
    World()

        : physics_events_(std::make_unique<PhysicsEvents>())
        , world_events_(std::make_unique<WorldEvents>())
        , fps_limit_(0)
    {
        animation_data_manager_.LoadAllAnimationDatas();
        state_manager_ = std::make_shared<StateManager>(animation_data_manager_);
    }

    void RunLoop() final
    {
        std::chrono::time_point<std::chrono::system_clock> last_frame_time;
        std::chrono::time_point<std::chrono::system_clock> last_render_time =
          std::chrono::system_clock::now();
        std::chrono::time_point<std::chrono::system_clock> last_update_time =
          std::chrono::system_clock::now();
        auto last_fps_check_time = std::chrono::system_clock::now();
        int frame_count_since_last_fps_check = 0;
        int last_fps = 0;

        auto timecur = std::chrono::system_clock::now();
        auto timeprv = timecur;
        std::chrono::duration<double> timeacc{ 0 };

        unsigned int game_tick = 0;
        int world_updates = 0;
        auto should_run_game_loop_iteration = [&]() {
            if (should_stop_game_loop_callback_) {
                return !should_stop_game_loop_callback_();
            }
            return true;
        };
        while (should_run_game_loop_iteration()) {
            if (pre_game_loop_iteration_callback_) {
                pre_game_loop_iteration_callback_();
            }

            auto current_frame_time = std::chrono::system_clock::now();
            std::chrono::duration<double> delta_time = current_frame_time - last_frame_time;
            last_frame_time = current_frame_time;

            frame_count_since_last_fps_check++;
            std::chrono::duration<double> diff = current_frame_time - last_fps_check_time;
            if (diff.count() >= 1.0) {
                spdlog::info("{} ms/frame", 1000.0 / double(frame_count_since_last_fps_check));
                spdlog::info("FPS: {}", frame_count_since_last_fps_check);
                last_fps = frame_count_since_last_fps_check;
                frame_count_since_last_fps_check = 0;
                last_fps_check_time = current_frame_time;

                spdlog::info("World updates: {}", world_updates);
                world_updates = 0;
            }

            double dt = 1.0 / 60.0;

            while (timeacc.count() >= dt) {
                std::chrono::duration<double> dt_in_duration{ dt };
                timeacc -= dt_in_duration;

                if (!state_manager_->IsGamePaused()) {
                    if (pre_world_update_callback_) {
                        pre_world_update_callback_();
                    }
                    Update(delta_time.count());
                    if (post_world_update_callback_) {
                        post_world_update_callback_(*state_manager_);
                    }

                    world_updates++;
                    game_tick++;
                    state_manager_->SetGameTick(game_tick);
                }

                timecur = std::chrono::system_clock::now();
                timeacc += timecur - timeprv;
                timeprv = timecur;
            }

            double frame_percent = 1.0F;
            if (!state_manager_->IsGamePaused()) {
                frame_percent = std::min(1.0, std::max(0.0, timeacc.count() / dt));
            }

            if (post_game_loop_iteration_callback_) {
                post_game_loop_iteration_callback_(*state_manager_, frame_percent, last_fps);
            }

            timecur = std::chrono::system_clock::now();
            timeacc += (timecur - timeprv);
            timeprv = timecur;

            std::chrono::duration<double> render_time_delta =
              std::chrono::system_clock::now() - last_render_time;

            double render_time_limit = 1.0 / (double)fps_limit_;

            while (fps_limit_ != 0 && render_time_delta.count() <= render_time_limit) {
                double time_to_wait_nanos = render_time_limit - render_time_delta.count();
                // sleep_for isn't super accurate that's why we don't want to wait the exact
                // amount of time, that's why we do - 0.2
                int time_to_wait_ms = std::max(0, (int)(1000.0 * time_to_wait_nanos - 0.2));

                // TODO: Don't use sleep when VSync is on
                // sleep_for to give the CPU to other processes
                // with lower FPS, the CPU usage should be lower
                std::this_thread::yield();
                std::this_thread::sleep_for(std::chrono::milliseconds(time_to_wait_ms));

                timecur = std::chrono::system_clock::now();
                timeacc += timecur - timeprv;
                timeprv = timecur;

                render_time_delta = std::chrono::system_clock::now() - last_render_time;
            }

            last_render_time = std::chrono::system_clock::now();
        }
    }

    void Update(double /*delta_time*/) final
    {
        // TODO: move this somewhere else
        static float gravity = 0.06F;

        std::vector<BulletParams> bullet_emitter;

        state_manager_->TransformSoldiers([&](auto& soldier) {
            bool should_update_current_soldier = true;
            if (pre_soldier_update_callback_) {
                should_update_current_soldier = pre_soldier_update_callback_(soldier);
            }

            if (should_update_current_soldier) {
                SoldierPhysics::Update(*state_manager_,
                                       soldier,
                                       *physics_events_,
                                       animation_data_manager_,
                                       bullet_emitter,
                                       gravity);
                if (soldier.dead_meat) {
                    soldier.ticks_to_respawn--;
                    if (soldier.ticks_to_respawn <= 0) {
                        SpawnSoldier(soldier.id);
                    }
                }
            }
        });

        state_manager_->TransformBullets([&](auto& bullet) {
            BulletPhysics::UpdateBullet(
              *physics_events_, bullet, state_manager_->GetMap(), *state_manager_);
        });

        for (const auto& bullet_params : state_manager_->GetBulletEmitter()) {
            bool should_spawn_projectile = false;
            if (pre_projectile_spawn_callback_) {
                should_spawn_projectile = pre_projectile_spawn_callback_(bullet_params);
            }

            if (should_spawn_projectile) {
                state_manager_->CreateProjectile(bullet_params);
            }
        }

        state_manager_->TransformItems(
          [&](auto& item) { ItemPhysics::Update(*state_manager_, item, *physics_events_); });

        state_manager_->ClearBulletEmitter();
    }

    void UpdateSoldier(unsigned int soldier_id) final
    {
        static float gravity = 0.06F;
        std::vector<BulletParams> bullet_emitter;

        state_manager_->TransformSoldier(soldier_id, [&](Soldier& soldier) {
            SoldierPhysics::Update(*state_manager_,
                                   soldier,
                                   *physics_events_,
                                   animation_data_manager_,
                                   bullet_emitter,
                                   gravity);
        });
    }

    const std::shared_ptr<StateManager>& GetStateManager() const final { return state_manager_; }

    const Soldier& GetSoldier(unsigned int soldier_id) const final
    {
        return state_manager_->GetSoldier(soldier_id);
    }

    PhysicsEvents& GetPhysicsEvents() final { return *physics_events_; }

    WorldEvents& GetWorldEvents() final { return *world_events_; }

    std::shared_ptr<const AnimationData> GetAnimationData(
      AnimationType animation_type) const override
    {
        return animation_data_manager_.Get(animation_type);
    }

    std::shared_ptr<AnimationState> GetBodyAnimationState(AnimationType animation_type) const final
    {
        switch (animation_type) {
            case AnimationType::Stand:
                return std::make_shared<BodyStandAnimationState>(animation_data_manager_);
            case AnimationType::Throw:
                return std::make_shared<BodyThrowAnimationState>(animation_data_manager_);
            case AnimationType::Change:
                return std::make_shared<BodyChangeAnimationState>(animation_data_manager_);
            case AnimationType::ThrowWeapon:
                return std::make_shared<BodyThrowWeaponAnimationState>(animation_data_manager_);
            case AnimationType::Punch:
                return std::make_shared<BodyPunchAnimationState>(animation_data_manager_);
            case AnimationType::Roll:
                return std::make_shared<BodyRollAnimationState>(animation_data_manager_);
            case AnimationType::RollBack:
                return std::make_shared<BodyRollBackAnimationState>(animation_data_manager_);
            case AnimationType::Prone:
                return std::make_shared<BodyProneAnimationState>(animation_data_manager_);
            case AnimationType::Aim:
                return std::make_shared<BodyAimAnimationState>(animation_data_manager_);
            case AnimationType::ProneMove:
                return std::make_shared<BodyProneMoveAnimationState>(animation_data_manager_);
            case AnimationType::GetUp:
                return std::make_shared<BodyGetUpAnimationState>(animation_data_manager_);
            default:
                // TODO: for now throw error, it will be easier to find this place. Once everything
                // is implemented, do std::unreachable()
                throw std::runtime_error("Body animation not implemented!");
        }
    }

    std::shared_ptr<AnimationState> GetLegsAnimationState(AnimationType animation_type) const final
    {
        switch (animation_type) {
            case AnimationType::Stand:
                return std::make_shared<LegsStandAnimationState>(animation_data_manager_);
            case AnimationType::Run:
                return std::make_shared<LegsRunAnimationState>(animation_data_manager_);
            case AnimationType::RunBack:
                return std::make_shared<LegsRunBackAnimationState>(animation_data_manager_);
            case AnimationType::Jump:
                return std::make_shared<LegsJumpAnimationState>(animation_data_manager_);
            case AnimationType::JumpSide:
                return std::make_shared<LegsJumpSideAnimationState>(animation_data_manager_);
            case AnimationType::Fall:
                return std::make_shared<LegsFallAnimationState>(animation_data_manager_);
            case AnimationType::Crouch:
                return std::make_shared<LegsCrouchAnimationState>(animation_data_manager_);
            case AnimationType::CrouchRun:
                return std::make_shared<LegsCrouchRunAnimationState>(animation_data_manager_);
            case AnimationType::Roll:
                return std::make_shared<LegsRollAnimationState>(animation_data_manager_);
            case AnimationType::RollBack:
                return std::make_shared<LegsRollBackAnimationState>(animation_data_manager_);
            case AnimationType::CrouchRunBack:
                return std::make_shared<LegsCrouchRunBackAnimationState>(animation_data_manager_);
            case AnimationType::Prone:
                return std::make_shared<LegsProneAnimationState>(animation_data_manager_);
            case AnimationType::ProneMove:
                return std::make_shared<LegsProneMoveAnimationState>(animation_data_manager_);
            case AnimationType::GetUp:
                return std::make_shared<LegsGetUpAnimationState>(animation_data_manager_);
            default:
                // TODO: for now throw error, it will be easier to find this place. Once everything
                // is implemented, do std::unreachable()
                throw std::runtime_error("Legs animation not implemented!");
        }
    }

    const Soldier& CreateSoldier(std::optional<unsigned int> force_soldier_id = std::nullopt) final
    {
        return state_manager_->CreateSoldier(force_soldier_id);
    }

    glm::vec2 SpawnSoldier(unsigned int soldier_id,
                           std::optional<glm::vec2> spawn_position = std::nullopt) final
    {
        auto initial_soldier_position = state_manager_->SpawnSoldier(soldier_id, spawn_position);
        world_events_->after_soldier_spawns.Notify(state_manager_->GetSoldier(soldier_id));
        return initial_soldier_position;
    }

    void KillSoldier(unsigned int soldier_id) final
    {
        state_manager_->TransformSoldier(soldier_id, [&](Soldier& soldier) {
            soldier.health = 0;
            soldier.dead_meat = true;
            soldier.ticks_to_respawn = 180; // 3 seconds
            world_events_->soldier_died.Notify(soldier);
        });
    }

    void HitSoldier(unsigned int soldier_id, float damage) final
    {
        state_manager_->TransformSoldier(soldier_id,
                                         [damage](Soldier& soldier) { soldier.health -= damage; });
    }

    void UpdateWeaponChoices(unsigned int soldier_id,
                             WeaponType primary_weapon_type,
                             WeaponType secondary_weapon_type) final
    {
        state_manager_->TransformSoldier(
          soldier_id, [primary_weapon_type, secondary_weapon_type](Soldier& soldier) {
              soldier.weapon_choices[0] = primary_weapon_type;
              soldier.weapon_choices[1] = secondary_weapon_type;
          });
    }

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

    void SetFPSLimit(int new_fps_limit) final { fps_limit_ = new_fps_limit; }

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

    AnimationDataManager animation_data_manager_;

    int fps_limit_;
};

} // namespace Soldank
