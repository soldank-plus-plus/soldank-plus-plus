module;

#include "spdlog/spdlog.h"
#include <algorithm>
#include <utility>
#include <memory>
#include <filesystem>
#include <functional>
#include <random>
#include <stdexcept>
#include <vector>

export module Shared.Core.State.StateManager;

import Extern.Glm;

import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParameters;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Entities.Bullet;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;
import Shared.Core.Math.Calc;
import Shared.Core.Map.Map;
import Shared.Core.Map.MapDocument;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.RuntimeMap;
import Shared.Core.Physics.SoldierSkeletonPhysics;
import Shared.Core.Physics.Particles;
import Shared.Core.State.Control;
import Shared.Core.State.State;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.WeaponType;
import Shared.Core.Animations;
import Shared.Core.Utility.Observable;
import Shared.Core.Physics.Polygons.PolygonContact;

export namespace Soldank
{
class StateManager
{
public:
    Observable<bool> game_pause_changed;
    Observable<std::uint8_t> soldier_removed;
    using SoldierStates = std::array<Soldier, MAX_SOLDIERS_COUNT>;

    StateManager(
      AnimationDataManager& animation_data_manager,
      std::shared_ptr<ParticleSystem> skeleton = ParticleSystem::Load(ParticleSystemType::Soldier));

    Map& GetMap() { return state_.map; } // TODO: Change this to const
    const Map& GetConstMap() const { return state_.map; }
    void OverrideMap(const Map& map) { state_.map.ReplaceContents(map); }
    MapDocument CreateMapDocumentSnapshot() const { return MapDocument::FromMap(state_.map); }
    void ApplyMapDocument(const MapDocument& map_document)
    {
        state_.map.ReplaceContents(map_document.GetMap());
    }
    void SpawnWeaponMapItems();
    void UpdateWeaponMapItems();
    void CreateEmptyMapDocument()
    {
        MapDocument map_document;
        map_document.CreateEmptyMap();
        ApplyMapDocument(map_document);
    }
    void LoadMapDocument(const std::filesystem::path& map_path)
    {
        MapDocument map_document;
        map_document.LoadMap(map_path);
        ApplyMapDocument(map_document);
        SpawnWeaponMapItems();
    }
    RuntimeMap BuildRuntimeMapFromDocument() const
    {
        return RuntimeMap::BuildFromDocument(CreateMapDocumentSnapshot());
    }
    void ApplyRuntimeMap(const RuntimeMap& runtime_map)
    {
        state_.map.ReplaceContents(runtime_map.GetMap());
    }
    SoldierStates CreateSoldiersSnapshot() const { return state_.soldiers; }
    SoldierStates CreateEmptySoldiersSnapshot() const
    {
        SoldierStates soldiers = state_.soldiers;
        for (auto& soldier : soldiers) {
            soldier.active = false;
            soldier.particle.active = false;
        }
        return soldiers;
    }
    void ApplySoldiersSnapshot(const SoldierStates& soldiers)
    {
        state_.soldiers = soldiers;
        for (std::size_t soldier_id = 0; soldier_id < state_.soldiers.size(); ++soldier_id) {
            Soldier& soldier = state_.soldiers.at(soldier_id);
            soldier.id = static_cast<std::uint8_t>(soldier_id);
            if (soldier.active) {
                RepositionSoldierSkeletonParts(soldier);
            }
        }
    }

    void ChangeSoldierControlActionState(std::uint8_t soldier_id,
                                         ControlActionType control_action_type,
                                         bool new_state);
    void SoldierControlApply(
      std::uint8_t soldier_id,
      const std::function<void(const Soldier& soldier, Control& control)>& apply_function);

    void ChangeSoldierMouseMapPosition(std::uint8_t soldier_id, glm::ivec2 new_mouse_position);
    void SwitchSoldierWeapon(std::uint8_t soldier_id);
    void ChangeSoldierPrimaryWeapon(std::uint8_t soldier_id, WeaponType new_weapon_type);
    void SoldierPickupWeapon(std::uint8_t soldier_id, const Item& item);
    void SoldierPickupKit(std::uint8_t soldier_id, std::uint8_t item_id);
    void MoveSoldier(std::uint8_t soldier_id, const glm::vec2& move_offset);
    void SetSoldierPosition(std::uint8_t soldier_id, const glm::vec2& new_position);
    void ThrowSoldierFlags(std::uint8_t soldier_id);
    void ReleaseSoldierFlags(std::uint8_t soldier_id)
    {
        auto& soldier = GetSoldierRef(soldier_id);
        TransformItems([&](Item& item) {
            if (IsItemTypeFlag(item.style) && item.holding_soldier_id == soldier_id) {
                item.holding_soldier_id = -1;
                item.static_type = false;
                item.polygon_background = soldier.polygon_background;
            }
            if (item.mounted_soldier == soldier_id) {
                item.mounted_soldier = -1;
            }
        });
        soldier.is_holding_flags = false;
        soldier.mounted_weapon = -1;
    }
    glm::vec2 GetSoldierAimDirection(std::uint8_t soldier_id);
    void TransformSoldier(std::uint8_t soldier_id,
                          const std::function<void(Soldier&)>& transform_soldier_function);
    void TransformSoldiers(const std::function<void(Soldier&)>& transform_soldier_function);
    Soldier& GetSoldier(std::uint8_t soldier_id);
    const Soldier& GetSoldier(std::uint8_t soldier_id) const;
    const Soldier& CreateSoldier(std::optional<unsigned int> force_soldier_id);
    glm::vec2 SpawnSoldier(unsigned int soldier_id, std::optional<glm::vec2> spawn_position);
    void ForEachSoldier(
      const std::function<void(const Soldier& soldier)>& for_each_soldier_function) const;
    void ForSoldier(std::uint8_t soldier_id,
                    const std::function<void(const Soldier& soldier)>& for_soldier_function) const;
    const Soldier* FindSoldier(const std::function<bool(const Soldier& soldier)>& predicate) const;
    void RemoveSoldier(std::uint8_t soldier_id);
    std::size_t GetSoldiersCount() const;

    void EnqueueNewProjectile(const BulletParams& bullet_params);
    void CreateProjectile(const BulletParams& bullet_params);
    BulletParams PrepareProjectile(BulletParams params)
    {
        if (params.id == 0) {
            params.id = next_projectile_id_++;
            if (params.seed == 0) {
                params.seed = GetRandomInt(1, 0x7fffffffU);
            }
            params.spawn_tick = GetGameTick();
            ForEachSoldier([&](const Soldier& owner) {
                if (!params.derived && owner.id == params.owner_id) {
                    params.owner_ping_ticks = owner.GetPingTicks();
                }
            });
        }
        return params;
    }
    void RecordProjectileEvent(ProjectilePhysicsEvent event)
    {
        projectile_events_.push_back(std::move(event));
    }
    const std::vector<ProjectilePhysicsEvent>& GetProjectileEvents() const
    {
        return projectile_events_;
    }
    void ClearProjectileEvents() { projectile_events_.clear(); }
    void SetFriendlyFire(bool enabled) { friendly_fire_ = enabled; }
    bool IsFriendlyFireEnabled() const { return friendly_fire_; }
    void SetMaxGrenades(std::uint8_t count) { max_grenades_ = count; }
    bool IsRealistic() const { return realistic_; }
    float GetMaximumHealth() const { return realistic_ ? 65.0F : 150.0F; }
    void SetRealistic(bool enabled)
    {
        realistic_ = enabled;
        for (auto& soldier : state_.soldiers) {
            for (std::size_t slot = 0; slot < soldier.weapons.size(); ++slot) {
                const auto kind = soldier.weapons.at(slot).GetWeaponParameters().kind;
                EquipWeapon(soldier, slot, WeaponParametersFactory::GetParameters(kind, enabled));
            }
        }
    }
    void SetAuthoritativeProjectiles(bool enabled) { authoritative_projectiles_ = enabled; }
    bool HasAuthoritativeProjectiles() const { return authoritative_projectiles_; }

    const std::vector<BulletParams>& GetBulletEmitter() const;
    void ClearBulletEmitter();
    void ForEachBullet(
      const std::function<void(const Bullet& bullet)>& for_each_bullet_function) const;
    std::size_t GetBulletsCount() const;
    void TransformBullets(const std::function<void(Bullet& bullet)>& transform_bullet_function);

    Item& CreateItem(glm::vec2 position,
                     std::uint8_t owner_id,
                     ItemType style,
                     std::shared_ptr<ParticleSystem> skeleton = nullptr,
                     std::optional<std::uint8_t> force_id = std::nullopt);
    Item& RestoreItem(std::uint8_t id, ItemType style, std::uint8_t owner, glm::vec2 position)
    {
        auto& item = state_.items.at(id);
        if (!item.active || item.style != style) {
            return CreateItem(position, owner, style, nullptr, id);
        }
        return item;
    }
    void RemoveItem(std::uint8_t id) { state_.items.at(id).active = false; }
    void SetItemPosition(unsigned int id, glm::vec2 new_position);
    void MoveItemIntoDirection(unsigned int id, glm::vec2 direction);
    void TransformItems(const std::function<void(Item& item)>& transform_item_function);
    void ForEachItem(const std::function<void(const Item& item)>& for_each_item_function) const;

    void SetRandomSeed(std::uint32_t seed) { mersenne_twister_engine_.seed(seed); }
    unsigned int GetRandomInt(unsigned int minimum, unsigned int maximum)
    {
        return std::uniform_int_distribution<unsigned int>(minimum,
                                                           maximum)(mersenne_twister_engine_);
    }

    unsigned int GetGameTick() const { return state_.game_tick; }
    void SetGameTick(unsigned int new_game_tick) { state_.game_tick = new_game_tick; }

    bool IsGamePaused() const { return state_.paused; }
    void PauseGame() { SetGamePaused(true); }
    void UnPauseGame() { SetGamePaused(false); }
    void TogglePauseGame() { SetGamePaused(!state_.paused); }

private:
    std::uint32_t next_life_sequence_{};
    void SetGamePaused(bool paused)
    {
        if (state_.paused != paused) {
            state_.paused = paused;
            game_pause_changed.Notify(paused);
        }
    }
    void EquipWeapon(Soldier& soldier,
                     std::size_t slot,
                     const WeaponParameters& parameters,
                     std::optional<std::uint8_t> ammo = std::nullopt,
                     std::optional<std::uint8_t> capacity = std::nullopt)
    {
        Weapon weapon(parameters, ammo, capacity);
        weapon.SetInstanceId(++soldier.weapon_generation);
        soldier.weapons.at(slot) = std::move(weapon);
    }
    Soldier& GetSoldierRef(std::uint8_t soldier_id);
    Item& GetItemRef(std::uint8_t item_id);

    struct MapItemSpawn
    {
        std::uint8_t id;
        ItemType style;
        glm::vec2 position;
        std::optional<unsigned int> respawn_tick;
    };
    std::vector<MapItemSpawn> map_item_spawns_;
    State state_;
    std::vector<BulletParams> bullet_emitter_;
    std::vector<ProjectilePhysicsEvent> projectile_events_;
    std::uint32_t next_projectile_id_{ 1 };
    bool friendly_fire_{ true };
    bool authoritative_projectiles_{ true };
    bool realistic_{};
    std::uint8_t max_grenades_{ 5 };

    std::mt19937 mersenne_twister_engine_{ 0U };
};
} // namespace Soldank

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

StateManager::StateManager(AnimationDataManager& animation_data_manager,
                           std::shared_ptr<ParticleSystem> skeleton)
    : state_(animation_data_manager, std::move(skeleton))
{
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
    if (soldier.combat_bonus == CombatBonus::FlameGod) {
        return;
    }
    int new_active_weapon = (soldier.active_weapon + 1) % 2;
    soldier.active_weapon = new_active_weapon;
    soldier.weapons[new_active_weapon].ResetStartUpTimeCount();
    soldier.weapons[new_active_weapon].ResetBurst();
}

void StateManager::ChangeSoldierPrimaryWeapon(std::uint8_t soldier_id, WeaponType new_weapon_type)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    auto new_weapon_parameters =
      WeaponParametersFactory::GetParameters(new_weapon_type, realistic_);
    EquipWeapon(soldier, soldier.active_weapon, new_weapon_parameters);
}

void StateManager::SoldierPickupWeapon(std::uint8_t soldier_id, const Item& item)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    WeaponType new_weapon_type = ItemTypeToWeaponType(item.style);
    auto new_weapon_parameters =
      WeaponParametersFactory::GetParameters(new_weapon_type, realistic_);
    EquipWeapon(soldier, soldier.active_weapon, new_weapon_parameters, item.ammo_count);
    if (new_weapon_type == WeaponType::Bow) {
        EquipWeapon(soldier,
                    (soldier.active_weapon + 1) % 2,
                    WeaponParametersFactory::GetParameters(WeaponType::FlameBow, realistic_));
    }
}

void StateManager::SoldierPickupKit(std::uint8_t soldier_id, std::uint8_t item_id)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    Item& item = GetItemRef(item_id);

    if (item.style == ItemType::MedicalKit) {
        if (soldier.health >= GetMaximumHealth()) {
            return;
        }

        soldier.health = GetMaximumHealth();
        item.active = false;
    }

    if (item.style == ItemType::GrenadeKit) {
        if (soldier.weapons[2].GetWeaponParameters().kind == WeaponType::ClusterGrenade &&
            soldier.weapons[2].GetAmmoCount() > 0) {
            // Don't pickup normal grenades if Soldier has any Cluster grenades already
            return;
        }

        if (soldier.weapons[2].GetAmmoCount() >= soldier.weapons[2].GetCapacity()) {
            // Don't pickup if Soldier has all the grenades (or more)
            return;
        }

        EquipWeapon(soldier,
                    2,
                    WeaponParametersFactory::GetParameters(WeaponType::FragGrenade, realistic_),
                    max_grenades_,
                    max_grenades_);
        item.active = false;
    }

    if (item.style == ItemType::FlamerKit && soldier.combat_bonus == CombatBonus::None &&
        soldier.CanReceiveCombatImpact() &&
        !soldier.GetActiveWeapon().IsAny({ WeaponType::Bow, WeaponType::FlameBow })) {
        soldier.weapons[(soldier.active_weapon + 1) % 2] = soldier.GetActiveWeapon();
        EquipWeapon(soldier,
                    soldier.active_weapon,
                    WeaponParametersFactory::GetParameters(WeaponType::Flamer, realistic_));
        soldier.combat_bonus = CombatBonus::FlameGod;
        soldier.bonus_ticks = 600;
        soldier.health = GetMaximumHealth();
        item.active = false;
    }

    if (item.style == ItemType::PredatorKit) {
        // TODO
    }

    if (item.style == ItemType::VestKit) {
        soldier.vest = SOLDIER_DEFAULT_VEST;
        item.active = false;
    }

    if (item.style == ItemType::BerserkKit && soldier.combat_bonus == CombatBonus::None &&
        soldier.CanReceiveCombatImpact()) {
        soldier.combat_bonus = CombatBonus::Berserk;
        soldier.bonus_ticks = 900;
        item.active = false;
    }

    if (item.style == ItemType::ClusterKit) {
        if (soldier.weapons[2].GetWeaponParameters().kind == WeaponType::ClusterGrenade &&
            soldier.weapons[2].GetAmmoCount() > 0) {
            // Don't pickup more cluster grenades if Soldier has already some
            return;
        }

        EquipWeapon(soldier,
                    2,
                    WeaponParametersFactory::GetParameters(WeaponType::ClusterGrenade, realistic_),
                    3,
                    3);
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
    if (!HasAuthoritativeProjectiles()) {
        return;
    }
    Soldier& soldier = GetSoldierRef(soldier_id);
    for (auto item_it = state_.items.begin(); item_it != state_.items.end(); ++item_it) {
        if (IsItemTypeFlag(item_it->style) && item_it->holding_soldier_id == soldier_id) {
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
                item_it->holding_soldier_id = -1;
                item_it->polygon_background = soldier.polygon_background;
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

void StateManager::TransformSoldier(std::uint8_t soldier_id,
                                    const std::function<void(Soldier&)>& transform_soldier_function)
{
    Soldier& soldier = GetSoldierRef(soldier_id);
    if (!soldier.active) {
        spdlog::warn("Trying to transform inactive soldier of id: {}", soldier_id);
        return;
    }
    transform_soldier_function(soldier);
}

void StateManager::TransformSoldiers(
  const std::function<void(Soldier&)>& transform_soldier_function)
{
    for (auto& soldier : state_.soldiers) {
        if (!soldier.active) {
            continue;
        }

        transform_soldier_function(soldier);
    }
}

Soldier& StateManager::GetSoldier(std::uint8_t soldier_id)
{
    return GetSoldierRef(soldier_id);
}

const Soldier& StateManager::GetSoldier(std::uint8_t soldier_id) const
{
    if (soldier_id >= state_.soldiers.size()) {
        spdlog::critical("Trying to access soldier of invalid id: {}", soldier_id);
        std::unreachable();
    }

    return state_.soldiers.at(soldier_id);
}

const Soldier& StateManager::CreateSoldier(std::optional<unsigned int> force_soldier_id)
{
    unsigned int new_soldier_id = NAN;

    if (!force_soldier_id.has_value()) {
        unsigned int free_soldier_id = 0;
        for (const auto& soldier : state_.soldiers) {
            if (!soldier.active) {
                break;
            }

            ++free_soldier_id;
        }

        new_soldier_id = free_soldier_id;
    } else {
        new_soldier_id = *force_soldier_id;
    }

    spdlog::debug("new_soldier_id {}", new_soldier_id);
    state_.soldiers.at(new_soldier_id).SetDefaultValues();
    state_.soldiers.at(new_soldier_id).active = true;
    state_.soldiers.at(new_soldier_id).id = new_soldier_id;

    return state_.soldiers.at(new_soldier_id);
}

glm::vec2 StateManager::SpawnSoldier(unsigned int soldier_id,
                                     std::optional<glm::vec2> spawn_position)
{
    glm::vec2 initial_player_position{ 0.0F, 0.0F };
    if (spawn_position.has_value()) {
        initial_player_position = *spawn_position;
    } else {
        std::vector<glm::vec2> possible_spawn_point_positions;
        for (const auto& spawn_point : state_.map.GetSpawnPoints()) {
            if (spawn_point.type == PMSSpawnPointType::General ||
                spawn_point.type == PMSSpawnPointType::Alpha ||
                spawn_point.type == PMSSpawnPointType::Bravo ||
                spawn_point.type == PMSSpawnPointType::Charlie ||
                spawn_point.type == PMSSpawnPointType::Delta) {

                possible_spawn_point_positions.emplace_back(spawn_point.x, spawn_point.y);
            }
        }

        if (possible_spawn_point_positions.empty()) {
            for (const auto& spawn_point : state_.map.GetSpawnPoints()) {
                possible_spawn_point_positions.emplace_back(spawn_point.x, spawn_point.y);
            }
        }

        if (!possible_spawn_point_positions.empty()) {
            unsigned int random_spawnpoint_id =
              GetRandomInt(0, static_cast<unsigned int>(possible_spawn_point_positions.size()) - 1);

            initial_player_position = possible_spawn_point_positions.at(random_spawnpoint_id);
        }
    }

    ReleaseSoldierFlags(static_cast<std::uint8_t>(soldier_id));
    auto& soldier = GetSoldierRef(soldier_id);
    soldier.particle.position = initial_player_position;
    soldier.particle.old_position = initial_player_position;
    soldier.active = true;
    soldier.particle.active = true;
    soldier.health = GetMaximumHealth();
    soldier.polygon_background = {};
    soldier.background_touched = false;
    soldier.polygon_contacts.clear();
    soldier.life_sequence = ++next_life_sequence_;
    soldier.death_severity = DeathSeverity::Normal;
    soldier.death_committed = false;
    soldier.is_holding_flags = false;
    soldier.vest = 0.0F;
    soldier.fired = 0;
    soldier.projectile_pushes.clear();
    soldier.projectile_push_history.clear();
    soldier.latest_authoritative_tick.reset();
    soldier.position_history.clear();
    soldier.dead_meat = false;
    EquipWeapon(
      soldier, 0, WeaponParametersFactory::GetParameters(soldier.weapon_choices.at(0), realistic_));
    EquipWeapon(
      soldier, 1, WeaponParametersFactory::GetParameters(soldier.weapon_choices.at(1), realistic_));
    EquipWeapon(soldier,
                2,
                WeaponParametersFactory::GetParameters(WeaponType::FragGrenade, realistic_),
                static_cast<std::uint8_t>(max_grenades_ / 2),
                max_grenades_);
    soldier.grenade_can_throw = true;
    soldier.is_shooting = false;
    soldier.hit_spray = 0;
    soldier.mounted_weapon = -1;
    soldier.combat_bonus = CombatBonus::None;
    soldier.bonus_ticks = 0;
    soldier.active_weapon = 0;
    RepositionSoldierSkeletonParts(soldier);

    return initial_player_position;
}

void StateManager::ForEachSoldier(
  const std::function<void(const Soldier& soldier)>& for_each_soldier_function) const
{
    for (const Soldier& soldier : state_.soldiers) {
        if (!soldier.active) {
            continue;
        }

        for_each_soldier_function(soldier);
    }
}

void StateManager::ForSoldier(
  std::uint8_t soldier_id,
  const std::function<void(const Soldier& soldier)>& for_soldier_function) const
{
    if (soldier_id >= state_.soldiers.size()) {
        spdlog::critical("Trying to access soldier of invalid id: {}", soldier_id);
        std::unreachable();
    }

    for_soldier_function(state_.soldiers.at(soldier_id));
}

const Soldier* StateManager::FindSoldier(
  const std::function<bool(const Soldier& soldier)>& predicate) const
{
    for (const Soldier& soldier : state_.soldiers) {
        if (!soldier.active) {
            continue;
        }

        if (predicate(soldier)) {
            return &soldier;
        }
    }

    return nullptr;
}

void StateManager::RemoveSoldier(std::uint8_t soldier_id)
{
    if (soldier_id >= state_.soldiers.size()) {
        spdlog::critical("Trying to access soldier of invalid id: {}", soldier_id);
        std::unreachable();
    }

    ReleaseSoldierFlags(soldier_id);
    state_.soldiers.at(soldier_id).active = false;
    soldier_removed.Notify(soldier_id);
}

std::size_t StateManager::GetSoldiersCount() const
{
    return state_.soldiers.size();
}

void StateManager::EnqueueNewProjectile(const BulletParams& bullet_params)
{
    bullet_emitter_.push_back(PrepareProjectile(bullet_params));
}

void StateManager::CreateProjectile(const BulletParams& bullet_params)
{
    bool is_bullet_created = false;
    for (auto& bullet : state_.bullets) {
        if (bullet.active) {
            continue;
        }

        bullet = PrepareProjectile(bullet_params);
        RecordProjectileEvent(
          MakeProjectileEvent(ProjectileEventKind::ProjectileSpawned, bullet, GetGameTick()));
        bullet.active = true;
        is_bullet_created = true;

        break;
    }

    if (!is_bullet_created) {
        spdlog::warn("Could not create a new projectile because the limit is exceeded");
    }
}

const std::vector<BulletParams>& StateManager::GetBulletEmitter() const
{
    return bullet_emitter_;
}

void StateManager::ClearBulletEmitter()
{
    bullet_emitter_.clear();
}

void StateManager::ForEachBullet(
  const std::function<void(const Bullet& bullet)>& for_each_bullet_function) const
{
    for (const auto& bullet : state_.bullets) {
        if (!bullet.active) {
            continue;
        }

        for_each_bullet_function(bullet);
    }
}

std::size_t StateManager::GetBulletsCount() const
{
    std::size_t bullets_count = 0;

    for (const auto& bullet : state_.bullets) {
        if (!bullet.active) {
            continue;
        }

        ++bullets_count;
    }

    return bullets_count;
}

void StateManager::TransformBullets(
  const std::function<void(Bullet& bullet)>& transform_bullet_function)
{
    for (auto& bullet : state_.bullets) {
        if (!bullet.active) {
            continue;
        }

        transform_bullet_function(bullet);
    }
}

Item& StateManager::CreateItem(glm::vec2 position,
                               std::uint8_t owner_id,
                               ItemType style,
                               std::shared_ptr<ParticleSystem> skeleton,
                               std::optional<std::uint8_t> force_id)
{
    std::uint8_t new_id = 0;
    for (const auto& item : state_.items) {
        const bool reserved = std::ranges::any_of(
          map_item_spawns_, [new_id](const auto& spawn) { return spawn.id == new_id; });
        if (!item.active && !reserved) {
            break;
        }

        ++new_id;
    }

    new_id = force_id.value_or(new_id);
    if (new_id >= state_.items.size()) {
        spdlog::critical("Trying to create a new item but array is full, id: {}", new_id);
        throw std::runtime_error("Cannot create item: item array is full");
    }

    Item new_item{};
    new_item.active = true;
    new_item.style = style;
    new_item.id = new_id;
    new_item.holding_soldier_id = -1;
    new_item.owner = owner_id;
    new_item.time_out = 0;
    // new_item.skeleton = std::make_shared<ParticleSystem>();
    new_item.static_type = false;
    new_item.in_base = false;
    new_item.flipped = false;

    const auto create_skeleton = [&skeleton](ParticleSystemType particle_system_type,
                                             float particle_scale) {
        return skeleton ? skeleton : ParticleSystem::Load(particle_system_type, particle_scale);
    };

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
            new_item.skeleton = create_skeleton(ParticleSystemType::Flag, particle_scale);
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
            new_item.skeleton = create_skeleton(ParticleSystemType::Weapon, particle_scale);
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
            new_item.skeleton = create_skeleton(ParticleSystemType::Kit, particle_scale);
            // new_item.skeleton->VDamping = 0.989;
            // new_item.skeleton->GravityMultiplier = 1.07;
            new_item.radius = KIT_RADIUS;
            new_item.time_out = FLAG_TIMEOUT; // TODO
            // new_item.interest : = DEFAULT_INTEREST_TIME;
            new_item.collide_with_bullets = true; // TODO: sv_kits_collide.Value;
            break;
        case ItemType::Parachute:
            new_item.skeleton = create_skeleton(ParticleSystemType::Parachute, particle_scale);
            new_item.time_out = 3600;
            break;
        case ItemType::M2:
            new_item.skeleton = create_skeleton(ParticleSystemType::StationaryGun, particle_scale);
            new_item.time_out = 60;
            new_item.radius = STAT_RADIUS;
            new_item.collide_with_bullets = false;
            break;
    }

    if (IsItemTypeWeapon(style)) {
        new_item.ammo_count =
          WeaponParametersFactory::GetParameters(ItemTypeToWeaponType(style), realistic_).ammo;
    }
    state_.items.at(new_id) = new_item;
    SetItemPosition(new_id, position);

    return state_.items.at(new_id);
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

void StateManager::TransformItems(const std::function<void(Item& item)>& transform_item_function)
{
    for (auto& item : state_.items) {
        if (!item.active) {
            continue;
        }

        transform_item_function(item);
    }
}

void StateManager::ForEachItem(
  const std::function<void(const Item& item)>& for_each_item_function) const
{
    for (const Item& item : state_.items) {
        if (!item.active) {
            continue;
        }

        for_each_item_function(item);
    }
}

Soldier& StateManager::GetSoldierRef(std::uint8_t soldier_id)
{
    if (soldier_id >= state_.soldiers.size()) {
        spdlog::critical("Trying to access soldier of invalid id: {}", soldier_id);
        std::unreachable();
    }

    return state_.soldiers.at(soldier_id);
}

Item& StateManager::GetItemRef(std::uint8_t item_id)
{
    if (item_id >= state_.items.size()) {
        spdlog::critical("Trying to access item of invalid id: {}", item_id);
        std::unreachable();
    }

    return state_.items.at(item_id);
}
} // namespace Soldank

namespace Soldank
{
std::optional<ItemType> WeaponMapItemStyle(PMSSpawnPointType type)
{
    switch (type) {
        case PMSSpawnPointType::AlphaFlag:
            return ItemType::AlphaFlag;
        case PMSSpawnPointType::BravoFlag:
            return ItemType::BravoFlag;
        case PMSSpawnPointType::YellowFlag:
            return ItemType::PointmatchFlag;
        case PMSSpawnPointType::Grenades:
            return ItemType::GrenadeKit;
        case PMSSpawnPointType::Medkits:
            return ItemType::MedicalKit;
        case PMSSpawnPointType::Clusters:
            return ItemType::ClusterKit;
        case PMSSpawnPointType::Vest:
            return ItemType::VestKit;
        case PMSSpawnPointType::Flamer:
            return ItemType::FlamerKit;
        case PMSSpawnPointType::Berserker:
            return ItemType::BerserkKit;
        case PMSSpawnPointType::RamboBow:
            return ItemType::Bow;
        case PMSSpawnPointType::StatGun:
            return ItemType::M2;
        default:
            return std::nullopt;
    }
}

void StateManager::SpawnWeaponMapItems()
{
    for (const auto& spawn : map_item_spawns_) {
        state_.items.at(spawn.id).active = false;
    }
    map_item_spawns_.clear();
    if (!HasAuthoritativeProjectiles()) {
        return;
    }
    for (const auto& point : state_.map.GetSpawnPoints()) {
        if (!point.active) {
            continue;
        }
        const auto style = WeaponMapItemStyle(point.type);
        if (style) {
            // One pickup per category until game modes provide a pickup budget.
            // Stationary guns are placed at every explicit map spawn.
            if (*style != ItemType::M2 &&
                std::ranges::any_of(map_item_spawns_,
                                    [&](const auto& spawn) { return spawn.style == *style; })) {
                continue;
            }
            const glm::vec2 position(point.x, point.y);
            const auto& item = CreateItem(position, 255, *style);
            map_item_spawns_.push_back({ item.id, *style, position, std::nullopt });
        }
    }
}

void StateManager::UpdateWeaponMapItems()
{
    if (!HasAuthoritativeProjectiles()) {
        return;
    }
    for (auto& spawn : map_item_spawns_) {
        if (state_.items.at(spawn.id).active) {
            spawn.respawn_tick.reset();
            continue;
        }
        if (!spawn.respawn_tick) {
            spawn.respawn_tick = GetGameTick() + 1U;
            continue;
        }
        if (GetGameTick() >= *spawn.respawn_tick) {
            if (spawn.style == ItemType::Bow &&
                FindSoldier([](const Soldier& soldier) {
                    return soldier.IsAlive() && soldier.GetActiveWeapon().IsAny(
                                                  { WeaponType::Bow, WeaponType::FlameBow });
                }) != nullptr) {
                continue;
            }
            bool bow_in_world = false;
            ForEachItem([&](const Item& item) {
                bow_in_world =
                  bow_in_world || (spawn.style == ItemType::Bow && item.style == ItemType::Bow);
            });
            if (bow_in_world) {
                continue;
            }
            std::vector<glm::vec2> positions;
            for (const auto& point : state_.map.GetSpawnPoints()) {
                const glm::vec2 candidate(point.x, point.y);
                if (point.active && WeaponMapItemStyle(point.type) == spawn.style &&
                    candidate != spawn.position) {
                    positions.push_back(candidate);
                }
            }
            if (!positions.empty()) {
                spawn.position =
                  positions.at(GetRandomInt(0, static_cast<unsigned int>(positions.size() - 1)));
            }
            CreateItem(spawn.position, 255, spawn.style, nullptr, spawn.id);
            spawn.respawn_tick.reset();
        }
    }
}
} // namespace Soldank
