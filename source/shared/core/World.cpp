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

#include "core/physics/Particles.hpp"
#include "core/state/Control.hpp"
#include "core/physics/BulletPhysics.hpp"
#include "core/physics/SoldierPhysics.hpp"
#include "core/physics/ItemPhysics.hpp"
#include "core/entities/WeaponParametersFactory.hpp"

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
    : state_manager_(std::make_shared<StateManager>())
    , physics_events_(std::make_unique<PhysicsEvents>())
    , world_events_(std::make_unique<WorldEvents>())
    , mersenne_twister_engine_(random_device_())
{
    animation_data_manager_.LoadAllAnimationDatas();
}

void World::RunLoop(int fps_limit)
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

        timecur = std::chrono::system_clock::now();
        timeacc += (timecur - timeprv);
        timeprv = timecur;

        std::chrono::duration<double> render_time_delta =
          std::chrono::system_clock::now() - last_render_time;
        std::chrono::duration<double> update_time_delta =
          std::chrono::system_clock::now() - last_update_time;

        // TODO: Figure out why fps limiting is inaccurate. Leaving it as it is for now because at
        // least it's working...
        while (fps_limit != 0 && render_time_delta.count() < 1.0 / (double)fps_limit &&
               update_time_delta.count() < dt) {

            // TODO: Don't use sleep when VSync is on
            // Sleep for 0 milliseconds to give the resource to other processes
            std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::milliseconds(0));

            timecur = std::chrono::system_clock::now();
            timeacc += timecur - timeprv;
            timeprv = timecur;

            render_time_delta = std::chrono::system_clock::now() - last_render_time;
            update_time_delta = std::chrono::system_clock::now() - last_update_time;
        }

        while (timeacc.count() >= dt) {
            std::chrono::duration<double> dt_in_duration{ dt };
            timeacc -= dt_in_duration;

            if (!state_manager_->GetState().paused) {
                if (pre_world_update_callback_) {
                    pre_world_update_callback_();
                }
                Update(delta_time.count());
                last_update_time = std::chrono::system_clock::now();
                if (post_world_update_callback_) {
                    post_world_update_callback_(state_manager_->GetState());
                }

                world_updates++;
                game_tick++;
                state_manager_->GetState().game_tick = game_tick;
            }

            timecur = std::chrono::system_clock::now();
            timeacc += timecur - timeprv;
            timeprv = timecur;
        }

        double frame_percent = 1.0F;
        if (!state_manager_->GetState().paused) {
            frame_percent = std::min(1.0, std::max(0.0, timeacc.count() / dt));
        }

        if (post_game_loop_iteration_callback_) {
            post_game_loop_iteration_callback_(state_manager_->GetState(), frame_percent, last_fps);
        }

        last_render_time = std::chrono::system_clock::now();
    }
}

void World::Update(double /*delta_time*/)
{
    auto removed_bullets_range = std::ranges::remove_if(
      state_manager_->GetState().bullets, [](const Bullet& bullet) { return !bullet.active; });
    state_manager_->GetState().bullets.erase(removed_bullets_range.begin(),
                                             removed_bullets_range.end());

    auto removed_items_range = std::ranges::remove_if(
      state_manager_->GetState().items, [](const Item& item) { return !item.active; });
    state_manager_->GetState().items.erase(removed_items_range.begin(), removed_items_range.end());

    std::vector<BulletParams> bullet_emitter;

    for (auto& soldier : state_manager_->GetState().soldiers) {
        if (soldier.active) {
            bool should_update_current_soldier = true;
            if (pre_soldier_update_callback_) {
                should_update_current_soldier = pre_soldier_update_callback_(soldier);
            }

            if (should_update_current_soldier) {
                SoldierPhysics::Update(state_manager_->GetState(),
                                       soldier,
                                       *physics_events_,
                                       animation_data_manager_,
                                       bullet_emitter);
                if (soldier.dead_meat) {
                    soldier.ticks_to_respawn--;
                    if (soldier.ticks_to_respawn <= 0) {
                        SpawnSoldier(soldier.id);
                    }
                }
            }
        }
    }

    for (auto& bullet : state_manager_->GetState().bullets) {
        BulletPhysics::UpdateBullet(
          *physics_events_, bullet, state_manager_->GetState().map, state_manager_->GetState());
    }

    for (const auto& bullet_params : state_manager_->GetBulletEmitter()) {
        bool should_spawn_projectile = false;
        if (pre_projectile_spawn_callback_) {
            should_spawn_projectile = pre_projectile_spawn_callback_(bullet_params);
        }

        if (should_spawn_projectile) {
            state_manager_->GetState().bullets.emplace_back(bullet_params);
        }
    }

    for (auto& item : state_manager_->GetState().items) {
        ItemPhysics::Update(state_manager_->GetState(), item, *physics_events_);
    }

    state_manager_->ClearBulletEmitter();
}

void World::UpdateSoldier(unsigned int soldier_id)
{
    std::vector<BulletParams> bullet_emitter;

    for (auto& soldier : state_manager_->GetState().soldiers) {
        if (soldier.active && soldier.id == soldier_id) {
            SoldierPhysics::Update(state_manager_->GetState(),
                                   soldier,
                                   *physics_events_,
                                   animation_data_manager_,
                                   bullet_emitter);
        }
    }
}

const std::shared_ptr<StateManager>& World::GetStateManager() const
{
    return state_manager_;
}

const Soldier& World::GetSoldier(unsigned int soldier_id) const
{
    for (const auto& soldier : state_manager_->GetState().soldiers) {
        if (soldier.id == soldier_id) {
            return soldier;
        }
    }

    std::unreachable();
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
    unsigned int new_soldier_id = NAN;

    if (!force_soldier_id.has_value()) {
        std::vector<unsigned int> current_soldier_ids;
        for (const auto& soldier : state_manager_->GetState().soldiers) {
            current_soldier_ids.push_back(soldier.id);
        }
        std::sort(current_soldier_ids.begin(), current_soldier_ids.end());
        unsigned int free_soldier_id = 1;
        while (free_soldier_id <= current_soldier_ids.size() &&
               current_soldier_ids.at(free_soldier_id - 1) == free_soldier_id) {
            free_soldier_id++;
        }

        new_soldier_id = free_soldier_id;
    } else {
        new_soldier_id = *force_soldier_id;
    }

    std::vector<Weapon> weapons{
        { WeaponParametersFactory::GetParameters(WeaponType::DesertEagles, false) },
        { WeaponParametersFactory::GetParameters(WeaponType::Knife, false) },
        { WeaponParametersFactory::GetParameters(WeaponType::FragGrenade, false) }
    };
    state_manager_->GetState().soldiers.emplace_back(
      new_soldier_id,
      animation_data_manager_,
      ParticleSystem::Load(ParticleSystemType::Soldier),
      weapons);

    return state_manager_->GetState().soldiers.back();
}

glm::vec2 World::SpawnSoldier(unsigned int soldier_id, std::optional<glm::vec2> spawn_position)
{
    glm::vec2 initial_player_position;
    if (spawn_position.has_value()) {
        initial_player_position = *spawn_position;
    } else {
        std::uniform_int_distribution<unsigned int> spawnpoint_id_random_distribution(
          0, state_manager_->GetState().map.GetSpawnPoints().size() - 1);

        unsigned int random_spawnpoint_id =
          spawnpoint_id_random_distribution(mersenne_twister_engine_);

        const auto& chosen_spawnpoint =
          state_manager_->GetState().map.GetSpawnPoints().at(random_spawnpoint_id);
        initial_player_position = { chosen_spawnpoint.x, chosen_spawnpoint.y };
    }

    for (auto& soldier : state_manager_->GetState().soldiers) {
        if (soldier.id == soldier_id) {
            soldier.particle.position = initial_player_position;
            soldier.particle.old_position = initial_player_position;
            soldier.active = true;
            soldier.particle.active = true;
            soldier.health = 150.0;
            soldier.dead_meat = false;
            soldier.weapons[0] =
              WeaponParametersFactory::GetParameters(soldier.weapon_choices[0], false);
            soldier.weapons[1] =
              WeaponParametersFactory::GetParameters(soldier.weapon_choices[1], false);
            soldier.active_weapon = 0;
            world_events_->after_soldier_spawns.Notify(soldier);
            return initial_player_position;
        }
    }

    spdlog::critical("[SpawnSoldier] Wrong soldier_id ({})", soldier_id);
    std::unreachable();
}

void World::KillSoldier(unsigned int soldier_id)
{
    for (auto& soldier : state_manager_->GetState().soldiers) {
        if (soldier.id == soldier_id && !soldier.dead_meat) {
            soldier.health = 0;
            soldier.dead_meat = true;
            soldier.ticks_to_respawn = 180; // 3 seconds
            world_events_->soldier_died.Notify(soldier);
        }
    }
}

void World::HitSoldier(unsigned int soldier_id, float damage)
{
    for (auto& soldier : state_manager_->GetState().soldiers) {
        if (soldier.id == soldier_id) {
            soldier.health -= damage;
        }
    }
}

void World::UpdateWeaponChoices(unsigned int soldier_id,
                                WeaponType primary_weapon_type,
                                WeaponType secondary_weapon_type)
{
    for (auto& soldier : state_manager_->GetState().soldiers) {
        if (soldier.id == soldier_id) {
            soldier.weapon_choices[0] = primary_weapon_type;
            soldier.weapon_choices[1] = secondary_weapon_type;
        }
    }
}
} // namespace Soldank