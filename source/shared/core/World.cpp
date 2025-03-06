#include "World.hpp"

#include "core/animations/AnimationData.hpp"

#include "core/animations/states/BodyAimAnimationState.hpp"
#include "core/animations/states/BodyChangeAnimationState.hpp"
#include "core/animations/states/BodyGetUpAnimationState.hpp"
#include "core/animations/states/BodyProneAnimationState.hpp"
#include "core/animations/states/BodyProneMoveAnimationState.hpp"
#include "core/animations/states/BodyPunchAnimationState.hpp"
#include "core/animations/states/BodyRollAnimationState.hpp"
#include "core/animations/states/BodyRollBackAnimationState.hpp"
#include "core/animations/states/BodyStandAnimationState.hpp"
#include "core/animations/states/BodyThrowAnimationState.hpp"
#include "core/animations/states/BodyThrowWeaponAnimationState.hpp"
#include "core/animations/states/LegsCrouchAnimationState.hpp"
#include "core/animations/states/LegsCrouchRunAnimationState.hpp"
#include "core/animations/states/LegsCrouchRunBackAnimationState.hpp"
#include "core/animations/states/LegsFallAnimationState.hpp"
#include "core/animations/states/LegsGetUpAnimationState.hpp"
#include "core/animations/states/LegsJumpAnimationState.hpp"
#include "core/animations/states/LegsJumpSideAnimationState.hpp"
#include "core/animations/states/LegsProneAnimationState.hpp"
#include "core/animations/states/LegsProneMoveAnimationState.hpp"
#include "core/animations/states/LegsRollAnimationState.hpp"
#include "core/animations/states/LegsRollBackAnimationState.hpp"
#include "core/animations/states/LegsRunAnimationState.hpp"
#include "core/animations/states/LegsRunBackAnimationState.hpp"
#include "core/animations/states/LegsStandAnimationState.hpp"

#include "core/map/PMSEnums.hpp"
#include "core/physics/Particles.hpp"
#include "core/physics/SoldierSkeletonPhysics.hpp"
#include "core/state/Control.hpp"
#include "core/physics/BulletPhysics.hpp"
#include "core/physics/SoldierPhysics.hpp"
#include "core/physics/ItemPhysics.hpp"
#include "core/entities/WeaponParametersFactory.hpp"

#include "core/state/State.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <random>
#include <ranges>
#include <memory>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <chrono>

namespace Soldank
{
World::World()

    : physics_events_(std::make_unique<PhysicsEvents>())
    , world_events_(std::make_unique<WorldEvents>())
    , fps_limit_(0)
{
    animation_data_manager_.LoadAllAnimationDatas();
    state_manager_ = std::make_shared<StateManager>(animation_data_manager_);
}

void World::RunLoop()
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

        while (fps_limit_ != 0 && render_time_delta.count() <= 1.0 / (double)fps_limit_) {

            // TODO: Don't use sleep when VSync is on
            // Sleep for 0 milliseconds to give the resource to other processes
            std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::milliseconds(0));

            timecur = std::chrono::system_clock::now();
            timeacc += timecur - timeprv;
            timeprv = timecur;

            render_time_delta = std::chrono::system_clock::now() - last_render_time;
        }

        last_render_time = std::chrono::system_clock::now();
    }
}

void World::Update(double /*delta_time*/)
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

void World::UpdateSoldier(unsigned int soldier_id)
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

const std::shared_ptr<StateManager>& World::GetStateManager() const
{
    return state_manager_;
}

const Soldier& World::GetSoldier(unsigned int soldier_id) const
{
    return state_manager_->GetSoldier(soldier_id);
}

PhysicsEvents& World::GetPhysicsEvents()
{
    return *physics_events_;
}

WorldEvents& World::GetWorldEvents()
{
    return *world_events_;
}

std::shared_ptr<const AnimationData> World::GetAnimationData(AnimationType animation_type) const
{
    return animation_data_manager_.Get(animation_type);
}

std::shared_ptr<AnimationState> World::GetBodyAnimationState(AnimationType animation_type) const
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
            // TODO: for now throw error, it will be easier to find this place. Once everything is
            // implemented, do std::unreachable()
            throw std::runtime_error("Body animation not implemented!");
    }
}

std::shared_ptr<AnimationState> World::GetLegsAnimationState(AnimationType animation_type) const
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
            // TODO: for now throw error, it will be easier to find this place. Once everything is
            // implemented, do std::unreachable()
            throw std::runtime_error("Legs animation not implemented!");
    }
}

const Soldier& World::CreateSoldier(std::optional<unsigned int> force_soldier_id)
{
    return state_manager_->CreateSoldier(force_soldier_id);
}

glm::vec2 World::SpawnSoldier(unsigned int soldier_id, std::optional<glm::vec2> spawn_position)
{
    auto initial_soldier_position = state_manager_->SpawnSoldier(soldier_id, spawn_position);
    world_events_->after_soldier_spawns.Notify(state_manager_->GetSoldier(soldier_id));
    return initial_soldier_position;
}

void World::KillSoldier(unsigned int soldier_id)
{
    state_manager_->TransformSoldier(soldier_id, [&](Soldier& soldier) {
        soldier.health = 0;
        soldier.dead_meat = true;
        soldier.ticks_to_respawn = 180; // 3 seconds
        world_events_->soldier_died.Notify(soldier);
    });
}

void World::HitSoldier(unsigned int soldier_id, float damage)
{
    state_manager_->TransformSoldier(soldier_id,
                                     [damage](Soldier& soldier) { soldier.health -= damage; });
}

void World::UpdateWeaponChoices(unsigned int soldier_id,
                                WeaponType primary_weapon_type,
                                WeaponType secondary_weapon_type)
{
    state_manager_->TransformSoldier(
      soldier_id, [primary_weapon_type, secondary_weapon_type](Soldier& soldier) {
          soldier.weapon_choices[0] = primary_weapon_type;
          soldier.weapon_choices[1] = secondary_weapon_type;
      });
}
} // namespace Soldank