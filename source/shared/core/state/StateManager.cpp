#include "core/state/StateManager.hpp"

#include "core/entities/Weapon.hpp"
#include "core/math/Calc.hpp"
#include "core/physics/Particles.hpp"
#include "core/physics/SoldierSkeletonPhysics.hpp"
#include "core/state/Control.hpp"
#include "core/entities/WeaponParametersFactory.hpp"

#include "core/types/ItemType.hpp"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <utility>

namespace Soldank
{
// TODO: Move these somewhere
const int SECOND = 60;
const int GUN_RADIUS = 10;
const int BOW_RADIUS = 20;
const int KIT_RADIUS = 12;
const int STAT_RADIUS = 15;
const int FLAG_RADIUS = 19;
const int FLAG_TIMEOUT = SECOND * 25;
const int GUN_TIMEOUT = SECOND * 20;
// TODO: why the duplication?
const int WAYPOINT_TIMEOUT_SMALL = SECOND * 5 + 20; // = 320
const int WAYPOINT_TIMEOUT_BIG = SECOND * 8;        // = 480
const float FLAGTHROW_POWER = 4.225;
const int SOLDIER_START_HEALTH = 150;
const int SOLDIER_DEFAULT_VEST = 100;

WeaponType ItemTypeToWeaponType(ItemType item_type)
{
    switch (item_type) {
        case ItemType::USSOCOM:
            return WeaponType::USSOCOM;
        case ItemType::DesertEagles:
            return WeaponType::DesertEagles;
        case ItemType::MP5:
            return WeaponType::MP5;
        case ItemType::Ak74:
            return WeaponType::Ak74;
        case ItemType::SteyrAUG:
            return WeaponType::SteyrAUG;
        case ItemType::Spas12:
            return WeaponType::Spas12;
        case ItemType::Ruger77:
            return WeaponType::Ruger77;
        case ItemType::M79:
            return WeaponType::M79;
        case ItemType::Barrett:
            return WeaponType::Barrett;
        case ItemType::Minimi:
            return WeaponType::Minimi;
        case ItemType::Minigun:
            return WeaponType::Minigun;
        case ItemType::Bow:
            return WeaponType::Bow;
        case ItemType::Knife:
            return WeaponType::Knife;
        case ItemType::Chainsaw:
            return WeaponType::Chainsaw;
        case ItemType::LAW:
            return WeaponType::LAW;
        case ItemType::AlphaFlag:
        case ItemType::BravoFlag:
        case ItemType::PointmatchFlag:
        case ItemType::MedicalKit:
        case ItemType::GrenadeKit:
        case ItemType::FlamerKit:
        case ItemType::PredatorKit:
        case ItemType::VestKit:
        case ItemType::BerserkKit:
        case ItemType::ClusterKit:
        case ItemType::Parachute:
        case ItemType::M2:
            break;
    }

    spdlog::critical("Invalid ItemType to WeaponType conversion");
    std::unreachable();
}

void StateManager::ChangeSoldierControlActionState(std::uint8_t soldier_id,
                                                   ControlActionType control_action_type,
                                                   bool new_state)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    auto get_action_field = [](Control& soldier_control,
                               ControlActionType control_action_type) -> bool& {
        switch (control_action_type) {
            case ControlActionType::MoveLeft:
                return soldier_control.left;
            case ControlActionType::MoveRight:
                return soldier_control.right;
            case ControlActionType::Jump:
                return soldier_control.up;
            case ControlActionType::Crouch:
                return soldier_control.down;
            case ControlActionType::Fire:
                return soldier_control.fire;
            case ControlActionType::UseJets:
                return soldier_control.jets;
            case ControlActionType::ChangeWeapon:
                return soldier_control.change;
            case ControlActionType::ThrowGrenade:
                return soldier_control.throw_grenade;
            case ControlActionType::DropWeapon:
                return soldier_control.drop;
            case ControlActionType::Reload:
                return soldier_control.reload;
            case ControlActionType::Prone:
                return soldier_control.prone;
            case ControlActionType::ThrowFlag:
                return soldier_control.flag_throw;
        }
    };
    bool& target_field_to_change = get_action_field(soldier.control, control_action_type);
    target_field_to_change = new_state;
}

void StateManager::SoldierControlApply(
  std::uint8_t soldier_id,
  const std::function<void(const Soldier& soldier, Control& control)>& apply_function)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    apply_function(soldier, soldier.control);
}

void StateManager::ChangeSoldierMouseMapPosition(std::uint8_t soldier_id,
                                                 glm::ivec2 new_mouse_position)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    soldier.control.mouse_aim_x = new_mouse_position.x;
    soldier.control.mouse_aim_y = new_mouse_position.y;
}

void StateManager::SwitchSoldierWeapon(std::uint8_t soldier_id)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    int new_active_weapon = (soldier.active_weapon + 1) % 2;
    soldier.active_weapon = new_active_weapon;
    // weapons[new_active_weapon].start_up_time_count =
    //   weapons[new_active_weapon].GetWeaponParameters().start_up_time;
    soldier.weapons[new_active_weapon].ResetStartUpTimeCount();
    // weapons[new_active_weapon].reload_time_prev = weapons[new_active_weapon].reload_time_count;
    soldier.weapons[new_active_weapon].SetReloadTimePrev(
      soldier.weapons[new_active_weapon].GetReloadTimeCount());
}

void StateManager::ChangeSoldierPrimaryWeapon(std::uint8_t soldier_id, WeaponType new_weapon_type)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    auto new_weapon_parameters = WeaponParametersFactory::GetParameters(new_weapon_type, false);
    soldier.weapons[soldier.active_weapon] = new_weapon_parameters;
}

void StateManager::SoldierPickupWeapon(std::uint8_t soldier_id, const Item& item)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    WeaponType new_weapon_type = ItemTypeToWeaponType(item.style);
    auto new_weapon_parameters = WeaponParametersFactory::GetParameters(new_weapon_type, false);
    new_weapon_parameters.ammo = item.ammo_count;
    soldier.weapons[soldier.active_weapon] = new_weapon_parameters;
}

void StateManager::SoldierPickupKit(std::uint8_t soldier_id, std::uint8_t item_id)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    Item& item = GetItemRef(item_id);

    if (item.style == ItemType::MedicalKit) {
        if (soldier.health >= SOLDIER_START_HEALTH) {
            return;
        }

        soldier.health = SOLDIER_START_HEALTH;
        item.active = false;
    }

    if (item.style == ItemType::GrenadeKit) {
        if (soldier.weapons[2].GetWeaponParameters().kind == WeaponType::ClusterGrenade &&
            soldier.weapons[2].GetAmmoCount() > 0) {
            // Don't pickup normal grenades if Soldier has any Cluster grenades already
            return;
        }

        if (soldier.weapons[2].GetAmmoCount() >= soldier.weapons[2].GetWeaponParameters().ammo) {
            // Don't pickup if Soldier has all the grenades (or more)
            return;
        }

        soldier.weapons[2] = WeaponParametersFactory::GetParameters(WeaponType::FragGrenade, false);
        item.active = false;
    }

    if (item.style == ItemType::FlamerKit) {
        // TODO
    }

    if (item.style == ItemType::PredatorKit) {
        // TODO
    }

    if (item.style == ItemType::VestKit) {
        soldier.vest = SOLDIER_DEFAULT_VEST;
        item.active = false;
    }

    if (item.style == ItemType::BerserkKit) {
        // TODO
    }

    if (item.style == ItemType::ClusterKit) {
        if (soldier.weapons[2].GetWeaponParameters().kind == WeaponType::ClusterGrenade &&
            soldier.weapons[2].GetAmmoCount() > 0) {
            // Don't pickup more cluster grenades if Soldier has already some
            return;
        }

        soldier.weapons[2] =
          WeaponParametersFactory::GetParameters(WeaponType::ClusterGrenade, false);
        item.active = false;
    }
}

void StateManager::MoveSoldier(std::uint8_t soldier_id, const glm::vec2& move_offset)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    glm::vec2 new_soldier_position = soldier.particle.position + move_offset;
    soldier.particle.position = new_soldier_position;
    soldier.particle.old_position = new_soldier_position;
    RepositionSoldierSkeletonParts(soldier);
}

void StateManager::SetSoldierPosition(std::uint8_t soldier_id, const glm::vec2& new_position)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    soldier.particle.position = new_position;
    soldier.particle.old_position = new_position;
    RepositionSoldierSkeletonParts(soldier);
}

void StateManager::ThrowSoldierFlags(std::uint8_t soldier_id)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    for (auto item_it = state_.items.begin(); item_it != state_.items.end(); ++item_it) {
        if (IsItemTypeFlag(item_it->style)) {
            glm::vec2 aim_direction = GetSoldierAimDirection(soldier_id);
            aim_direction = Calc::Vec2Scale(aim_direction, FLAGTHROW_POWER);

            glm::vec2 flagger_offset = Calc::Vec2Scale(aim_direction, 5);

            glm::vec2 flagger_velocity = aim_direction + soldier.particle.GetVelocity();

            glm::vec2 new_pos_diff = flagger_offset + flagger_velocity;
            glm::vec2 look_point_1 = item_it->skeleton->GetPos(1) + new_pos_diff;
            glm::vec2 future_point_1 = look_point_1 + glm::vec2{ -10, -8 };
            glm::vec2 future_point_2 = look_point_1 + glm::vec2{ 10, -8 };
            glm::vec2 future_point_3 = look_point_1 + glm::vec2{ -10, 8 };
            glm::vec2 future_point_4 = look_point_1 + glm::vec2{ 10, 8 };

            glm::vec2 perp;
            float distance = 0.0F;

            glm::vec2 look_point_2 = item_it->skeleton->GetPos(2) + new_pos_diff;
            glm::vec2 look_point_3 = item_it->skeleton->GetPos(3) + new_pos_diff;
            glm::vec2 look_point_4 = item_it->skeleton->GetPos(4) + new_pos_diff;

            if (!state_.map.RayCast(
                  soldier.skeleton->GetPos(15), look_point_2, distance, 200, false, true, false) &&
                !state_.map.RayCast(
                  soldier.skeleton->GetPos(15), look_point_3, distance, 200, false, true, false) &&
                !state_.map.RayCast(
                  soldier.skeleton->GetPos(15), look_point_4, distance, 200, false, true, false) &&
                !state_.map.CollisionTest(future_point_1, perp, true) &&
                !state_.map.CollisionTest(future_point_2, perp, true) &&
                !state_.map.CollisionTest(future_point_3, perp, true) &&
                !state_.map.CollisionTest(future_point_4, perp, true)) {

                for (unsigned int j = 1; j <= 4; ++j) {
                    // Apply offset from flagger
                    item_it->skeleton->SetPos(j, item_it->skeleton->GetPos(j) + flagger_offset);

                    // Apply velocities
                    item_it->skeleton->SetPos(j, item_it->skeleton->GetPos(j) + flagger_velocity);
                    item_it->skeleton->SetOldPos(j,
                                                 item_it->skeleton->GetPos(j) - flagger_velocity);
                }

                // Add some spin for visual effect
                perp = { -flagger_velocity.y, flagger_velocity.x };
                perp = Calc::Vec2Normalize(perp);
                perp = Calc::Vec2Scale(perp, soldier.direction);
                item_it->skeleton->SetPos(1, item_it->skeleton->GetPos(1) - perp);
                item_it->skeleton->SetPos(2, item_it->skeleton->GetPos(2) + perp);

                // Release the flag
                item_it->static_type = false;
                item_it->holding_soldier_id = 0;
                soldier.is_holding_flags = false;
            }
        }
    }
}

glm::vec2 StateManager::GetSoldierAimDirection(std::uint8_t soldier_id)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    glm::vec2 mouse_aim = { soldier.control.mouse_aim_x, soldier.control.mouse_aim_y };

    glm::vec2 aim_direction = mouse_aim - soldier.skeleton->GetPos(15);
    aim_direction = Calc::Vec2Normalize(aim_direction);

    return aim_direction;
}

void StateManager::CreateProjectile(const BulletParams& bullet_params)
{
    bullet_emitter_.push_back(bullet_params);
}

const std::vector<BulletParams>& StateManager::GetBulletEmitter() const
{
    return bullet_emitter_;
}

void StateManager::ClearBulletEmitter()
{
    bullet_emitter_.clear();
}

Item& StateManager::CreateItem(glm::vec2 position, std::uint8_t owner_id, ItemType style)
{
    std::vector<std::uint8_t> current_ids;
    std::uint8_t new_id = 0;
    current_ids.reserve(state_.items.size());
    for (const auto& item : state_.items) {
        current_ids.push_back(item.id);
    }
    std::sort(current_ids.begin(), current_ids.end());
    while (new_id < current_ids.size() && new_id == current_ids.at(new_id)) {
        new_id++;
    }
    Item new_item;
    new_item.active = true;
    new_item.style = style;
    new_item.id = new_id;
    new_item.holding_soldier_id = 0;
    new_item.owner = owner_id;
    new_item.time_out = 0;
    // new_item.skeleton = std::make_shared<ParticleSystem>();
    new_item.static_type = false;
    new_item.in_base = false;
    new_item.flipped = false;

    for (std::uint8_t& i : new_item.collide_count) {
        i = 0;
    }

    if (owner_id != 255 && owner_id != 0) {
        Soldier& soldier = GetSoldierRef(owner_id);
        if (soldier.direction == -1) {
            new_item.flipped = true;
        }
    }

    // TODO: handle this better
    float particle_scale = 1.0F;

    switch (style) {
        case ItemType::USSOCOM:
            particle_scale = 1.0F;
            break;
        case ItemType::DesertEagles:
            particle_scale = 1.1F;
            break;
        case ItemType::Knife:
            particle_scale = 1.8F;
            break;
        case ItemType::MedicalKit:
        case ItemType::GrenadeKit:
        case ItemType::FlamerKit:
        case ItemType::PredatorKit:
        case ItemType::VestKit:
        case ItemType::BerserkKit:
        case ItemType::ClusterKit:
            particle_scale = 2.15F;
            break;
        case ItemType::MP5:
            particle_scale = 2.2F;
            break;
        case ItemType::M79:
        case ItemType::Chainsaw:
        case ItemType::LAW:
            particle_scale = 2.8F;
            break;
        case ItemType::Spas12:
        case ItemType::Ruger77:
            particle_scale = 3.6F;
            break;
        case ItemType::Ak74:
        case ItemType::SteyrAUG:
            particle_scale = 3.7F;
            break;
        case ItemType::Minimi:
            particle_scale = 3.9F;
            break;
        case ItemType::AlphaFlag:
        case ItemType::BravoFlag:
        case ItemType::PointmatchFlag:
        case ItemType::M2:
            particle_scale = 4.0F;
            break;
        case ItemType::Barrett:
            particle_scale = 4.3F;
            break;
        case ItemType::Bow:
        case ItemType::Parachute:
            particle_scale = 5.0F;
            break;
        case ItemType::Minigun:
            particle_scale = 5.5F;
            break;
    }

    switch (style) {
        case ItemType::AlphaFlag:
        case ItemType::BravoFlag:
        case ItemType::PointmatchFlag: {
            new_item.skeleton = ParticleSystem::Load(ParticleSystemType::Flag, particle_scale);
            new_item.radius = FLAG_RADIUS;
            new_item.time_out = FLAG_TIMEOUT;
            new_item.collide_with_bullets = true;
            // TODO: inf flag doesn't collide
            break;
        }
        case ItemType::DesertEagles:
        case ItemType::MP5:
        case ItemType::Ak74:
        case ItemType::SteyrAUG:
        case ItemType::Spas12:
        case ItemType::Ruger77:
        case ItemType::M79:
        case ItemType::Barrett:
        case ItemType::Minimi:
        case ItemType::Minigun:
        case ItemType::USSOCOM:
        case ItemType::Knife:
        case ItemType::Chainsaw:
        case ItemType::LAW:
        case ItemType::Bow: // TODO: bow has different condition
            new_item.skeleton = ParticleSystem::Load(ParticleSystemType::Weapon, particle_scale);
            // new_item.skeleton->VDamping = 0.989;
            // new_item.skeleton->GravityMultiplier = 1.07;
            new_item.radius = GUN_RADIUS;
            new_item.time_out = GUN_TIMEOUT;
            // new_item.interest : = DEFAULT_INTEREST_TIME;
            new_item.collide_with_bullets = true; // TODO: sv_guns_collide.Value;
            break;
        case ItemType::FlamerKit:
        case ItemType::PredatorKit:
        case ItemType::BerserkKit:
        case ItemType::MedicalKit:
        case ItemType::ClusterKit:
        case ItemType::VestKit:
        case ItemType::GrenadeKit:
            new_item.skeleton = ParticleSystem::Load(ParticleSystemType::Kit, particle_scale);
            // new_item.skeleton->VDamping = 0.989;
            // new_item.skeleton->GravityMultiplier = 1.07;
            new_item.radius = KIT_RADIUS;
            new_item.time_out = FLAG_TIMEOUT; // TODO
            // new_item.interest : = DEFAULT_INTEREST_TIME;
            new_item.collide_with_bullets = true; // TODO: sv_kits_collide.Value;
            break;
        case ItemType::Parachute:
            new_item.skeleton = ParticleSystem::Load(ParticleSystemType::Parachute, particle_scale);
            new_item.time_out = 3600;
            break;
        case ItemType::M2:
            new_item.skeleton =
              ParticleSystem::Load(ParticleSystemType::StationaryGun, particle_scale);
            new_item.time_out = 60;
            new_item.radius = STAT_RADIUS;
            new_item.collide_with_bullets = false;
            break;
    }

    state_.items.push_back(new_item);
    SetItemPosition(state_.items.size() - 1, position);

    return state_.items.back();
}

void StateManager::SetItemPosition(unsigned int id, glm::vec2 new_position)
{
    Item& item = state_.items.at(id);
    glm::vec2 direction = new_position - item.skeleton->GetPos(1);
    MoveItemIntoDirection(id, direction);
}

void StateManager::MoveItemIntoDirection(unsigned int id, glm::vec2 direction)
{
    Item& item = state_.items.at(id);
    for (unsigned int i = 1; i <= item.skeleton->GetParticles().size(); ++i) {
        glm::vec2 new_position = item.skeleton->GetPos(i) + direction;
        item.skeleton->SetPos(i, new_position);
        item.skeleton->SetOldPos(i, new_position);
    }
}

Soldier& StateManager::GetSoldierRef(std::uint8_t soldier_id)
{
    for (auto& soldier : state_.soldiers) {
        if (soldier.id == soldier_id) {
            return soldier;
        }
    }

    spdlog::critical("Trying to access soldier of invalid id: {}", soldier_id);
    std::unreachable();
}

Item& StateManager::GetItemRef(std::uint8_t item_id)
{
    for (auto& item : state_.items) {
        if (item.id == item_id) {
            return item;
        }
    }

    spdlog::critical("Trying to access item of invalid id: {}", item_id);
    std::unreachable();
}
} // namespace Soldank
