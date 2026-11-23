module;

#include <cstdint>
#include <vector>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <algorithm>

export module Shared.Core.Physics.SoldierPhysics;

import Extern.Glm;

import Shared.Core.Animations;
import Shared.Core.Animations.SoldierEmotes;
import Shared.Core.Animations.States;
import Shared.Core.Entities.Weapon;
import Shared.Core.Physics.Weapons.WeaponPhysics;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Math.Calc;
import Shared.Core.Map.Map;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.BulletType;
import Shared.Core.Physics.Constants;
import Shared.Core.Physics.Particles;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Physics.SoldierSkeletonPhysics;
import Shared.Core.State.StateManager;
import Shared.Core.State.Control;
import Shared.Core.Map.Polygons.PolygonTraits;
import Shared.Core.Physics.Polygons.PolygonContact;
import Shared.Core.Physics.Polygons.PolygonContacts;
import Shared.Core.Physics.Polygons.PolygonSurfaceResponse;
import Shared.Core.Physics.Polygons.PolygonContactEffects;
import Shared.Core.Physics.Polygons.FlagCarry;

export namespace Soldank::SoldierPhysics
{
bool CheckMapCollision(Soldier& soldier,
                       const Map& map,
                       float x,
                       float y,
                       int area,
                       const PhysicsEvents& physics_events);
bool CheckMapVerticesCollision(Soldier& soldier,
                               const Map& map,
                               float x,
                               float y,
                               float r,
                               bool has_collided,
                               const PhysicsEvents& physics_events);
bool CheckRadiusMapCollision(Soldier& soldier,
                             const Map& map,
                             float x,
                             float y,
                             bool has_collided,
                             const PhysicsEvents& physics_events);
bool CheckSkeletonMapCollision(Soldier& soldier, const Map& map, unsigned int i, float x, float y);

void Fire(Soldier& soldier, std::vector<BulletParams>& bullet_emitter);
const Weapon& GetPrimaryWeapon(Soldier& soldier)
{
    return soldier.weapons[soldier.active_weapon];
}

const Weapon& GetSecondaryWeapon(Soldier& soldier)
{
    return soldier.weapons[(soldier.active_weapon + 1) % 2];
}

const Weapon& GetTertiaryWeapon(Soldier& soldier)
{
    return soldier.weapons[2];
}

void UpdateKeys(Soldier& soldier, const Control& control)
{
    soldier.control = control;
}

void RecordContact(Soldier& soldier, const PMSPolygon& polygon, glm::vec2 point, PolygonProbe probe)
{
    if (!soldier.IsAlive()) {
        return;
    }
    if (std::ranges::any_of(soldier.polygon_contacts, [&](const auto& contact) {
            return contact.polygon == polygon.id && contact.position == point &&
                   contact.probe == probe;
        })) {
        return;
    }
    soldier.polygon_contacts.push_back(
      { polygon.id, polygon.polygon_type, point, soldier.particle.GetVelocity(), probe });
}

std::shared_ptr<AnimationState> CreateBodyAnimationState(
  AnimationType animation_type,
  const AnimationDataManager& animation_data_manager)
{
    if (SoldierEmotes::IsEmoteAnimation(animation_type)) {
        return std::make_shared<EmoteAnimationState>(animation_data_manager, animation_type);
    }
    switch (animation_type) {
        case AnimationType::Melee:
        case AnimationType::Recoil:
        case AnimationType::SmallRecoil:
        case AnimationType::Shotgun:
        case AnimationType::Barret:
        case AnimationType::AimRecoil:
        case AnimationType::HandsUpRecoil:
        case AnimationType::HandsUpAim:
            return std::make_shared<BodyWeaponActionAnimationState>(animation_data_manager,
                                                                    animation_type);
        case AnimationType::Reload:
        case AnimationType::ReloadBow:
        case AnimationType::ClipOut:
        case AnimationType::ClipIn:
        case AnimationType::SlideBack:
            return std::make_shared<BodyReloadAnimationState>(animation_data_manager,
                                                              animation_type);
        case AnimationType::Stand:
            return std::make_shared<BodyStandAnimationState>(animation_data_manager);
        case AnimationType::Throw:
            return std::make_shared<BodyThrowAnimationState>(animation_data_manager);
        case AnimationType::Change:
            return std::make_shared<BodyChangeAnimationState>(animation_data_manager);
        case AnimationType::ThrowWeapon:
            return std::make_shared<BodyThrowWeaponAnimationState>(animation_data_manager);
        case AnimationType::Punch:
            return std::make_shared<BodyPunchAnimationState>(animation_data_manager);
        case AnimationType::Roll:
            return std::make_shared<BodyRollAnimationState>(animation_data_manager);
        case AnimationType::RollBack:
            return std::make_shared<BodyRollBackAnimationState>(animation_data_manager);
        case AnimationType::Prone:
            return std::make_shared<BodyProneAnimationState>(animation_data_manager);
        case AnimationType::Aim:
            return std::make_shared<BodyAimAnimationState>(animation_data_manager);
        case AnimationType::ProneMove:
            return std::make_shared<BodyProneMoveAnimationState>(animation_data_manager);
        case AnimationType::GetUp:
            return std::make_shared<BodyGetUpAnimationState>(animation_data_manager);
        default:
            throw std::runtime_error("Body animation not implemented!");
    }
}

std::shared_ptr<AnimationState> CreateLegsAnimationState(
  AnimationType animation_type,
  const AnimationDataManager& animation_data_manager)
{
    if (animation_type == AnimationType::Mercy || animation_type == AnimationType::Mercy2 ||
        animation_type == AnimationType::Own) {
        return std::make_shared<EmoteAnimationState>(animation_data_manager, animation_type);
    }
    switch (animation_type) {
        case AnimationType::Stand:
            return std::make_shared<LegsStandAnimationState>(animation_data_manager);
        case AnimationType::Run:
            return std::make_shared<LegsRunAnimationState>(animation_data_manager);
        case AnimationType::RunBack:
            return std::make_shared<LegsRunBackAnimationState>(animation_data_manager);
        case AnimationType::Jump:
            return std::make_shared<LegsJumpAnimationState>(animation_data_manager);
        case AnimationType::JumpSide:
            return std::make_shared<LegsJumpSideAnimationState>(animation_data_manager);
        case AnimationType::Fall:
            return std::make_shared<LegsFallAnimationState>(animation_data_manager);
        case AnimationType::Crouch:
            return std::make_shared<LegsCrouchAnimationState>(animation_data_manager);
        case AnimationType::CrouchRun:
            return std::make_shared<LegsCrouchRunAnimationState>(animation_data_manager);
        case AnimationType::Roll:
            return std::make_shared<LegsRollAnimationState>(animation_data_manager);
        case AnimationType::RollBack:
            return std::make_shared<LegsRollBackAnimationState>(animation_data_manager);
        case AnimationType::CrouchRunBack:
            return std::make_shared<LegsCrouchRunBackAnimationState>(animation_data_manager);
        case AnimationType::Prone:
            return std::make_shared<LegsProneAnimationState>(animation_data_manager);
        case AnimationType::ProneMove:
            return std::make_shared<LegsProneMoveAnimationState>(animation_data_manager);
        case AnimationType::GetUp:
            return std::make_shared<LegsGetUpAnimationState>(animation_data_manager);
        default:
            throw std::runtime_error("Legs animation not implemented!");
    }
}

void ApplyTransitionInitialFrame(AnimationState& animation_state,
                                 const AnimationState::Transition& transition)
{
    if (transition.initial_frame.has_value()) {
        animation_state.SetFrame(*transition.initial_frame);
    }
}

AnimationType ReloadAnimation(const Weapon& weapon)
{
    if (weapon.GetWeaponParameters().kind == WeaponType::Spas12) {
        return AnimationType::Reload;
    }
    if (weapon.IsAny({ WeaponType::Bow, WeaponType::FlameBow })) {
        return AnimationType::ReloadBow;
    }
    if (!weapon.GetWeaponParameters().clip_reload ||
        weapon.GetReloadTimeCount() > weapon.GetClipOutTime()) {
        return AnimationType::ClipOut;
    }
    return weapon.GetReloadTimeCount() > weapon.GetClipInTime() ? AnimationType::ClipIn
                                                                : AnimationType::SlideBack;
}

void UpdateReloadAnimation(Soldier& soldier, const AnimationDataManager& manager)
{
    const auto& weapon = soldier.GetActiveWeapon();
    const bool can_show_reload = soldier.body_animation->IsAny({ AnimationType::Stand,
                                                                 AnimationType::Aim,
                                                                 AnimationType::Prone,
                                                                 AnimationType::ProneMove,
                                                                 AnimationType::Reload,
                                                                 AnimationType::ReloadBow,
                                                                 AnimationType::ClipOut,
                                                                 AnimationType::ClipIn,
                                                                 AnimationType::SlideBack,
                                                                 AnimationType::SmallRecoil,
                                                                 AnimationType::AimRecoil,
                                                                 AnimationType::Shotgun });
    if (!weapon.IsReloading() || !can_show_reload) {
        return;
    }
    const auto type = ReloadAnimation(weapon);
    if (soldier.body_animation->GetType() != type) {
        soldier.body_animation = CreateBodyAnimationState(type, manager);
    }
}

void ApplyFiringAnimation(Soldier& soldier, const AnimationDataManager& manager)
{
    if (soldier.stance == PhysicsConstants::STANCE_PRONE ||
        soldier.body_animation->IsAny({ AnimationType::Throw,
                                        AnimationType::ThrowWeapon,
                                        AnimationType::Change,
                                        AnimationType::GetUp,
                                        AnimationType::Melee }) ||
        soldier.GetActiveWeapon().IsAny(
          { WeaponType::Knife, WeaponType::NoWeapon, WeaponType::Chainsaw, WeaponType::Flamer })) {
        return;
    }
    auto recoil = AnimationType::SmallRecoil;
    if (soldier.GetActiveWeapon().GetWeaponParameters().kind == WeaponType::Spas12) {
        recoil = AnimationType::Shotgun;
    } else if (soldier.stance == PhysicsConstants::STANCE_CROUCH) {
        recoil = AnimationType::AimRecoil;
    }
    soldier.body_animation = CreateBodyAnimationState(recoil, manager);
}

void AdvanceWeapons(StateManager& state_manager,
                    Soldier& soldier,
                    const PhysicsEvents& physics_events,
                    const AnimationDataManager& animation_data_manager)
{
    if (!soldier.IsAlive()) {
        return;
    }
    const auto notify_weapon_action = [&](WeaponActionKind kind, const WeaponState& state) {
        const WeaponActionEvent event{
            kind,
            soldier.id,
            state_manager.GetGameTick(),
            state,
            static_cast<std::uint8_t>(
              kind == WeaponActionKind::GrenadeThrown ? 2 : soldier.active_weapon)
        };
        physics_events.weapon_action_event.Notify(event);
    };
    if (state_manager.HasAuthoritativeProjectiles() && state_manager.GetGameTick() % 3U == 0 &&
        soldier.GetActiveWeapon().IsAny({ WeaponType::Bow, WeaponType::FlameBow }) &&
        soldier.health < state_manager.GetMaximumHealth()) {
        soldier.health = std::min(state_manager.GetMaximumHealth(), soldier.health + 1.0F);
    }
    const auto before = soldier.GetActiveWeapon().GetState();
    WeaponPhysics::Advance(soldier);
    const auto after = soldier.GetActiveWeapon().GetState();
    if (before.reloading != after.reloading) {
        notify_weapon_action(after.reloading ? WeaponActionKind::ReloadStarted
                                             : WeaponActionKind::ReloadCompleted,
                             after);
    }
    auto& weapon = soldier.GetActiveWeapon();
    const auto body_type = soldier.body_animation->GetType();
    if (weapon.IsReloading() && body_type == AnimationType::Reload &&
        soldier.body_animation->GetFrame() == 14 && soldier.body_animation->GetCount() == 0) {
        weapon.InsertShell();
        notify_weapon_action(WeaponActionKind::ShellInserted, weapon.GetState());
    }
    UpdateReloadAnimation(soldier, animation_data_manager);
    std::vector<BulletParams> grenades;
    if (WeaponPhysics::ThrowGrenade(soldier, state_manager.GetConstMap(), grenades)) {
        notify_weapon_action(WeaponActionKind::GrenadeThrown, soldier.weapons[2].GetState());
    }
    if (state_manager.HasAuthoritativeProjectiles()) {
        for (const auto& grenade : grenades) {
            state_manager.EnqueueNewProjectile(grenade);
        }
    }
}

void TryButtstock(StateManager& state_manager,
                  Soldier& soldier,
                  const AnimationDataManager& animation_data_manager)
{
    if (soldier.IsAlive() && soldier.control.fire && soldier.CanReceiveCombatImpact() &&
        soldier.mounted_weapon < 0 && soldier.stance == PhysicsConstants::STANCE_STAND &&
        soldier.body_animation->GetType() != AnimationType::Melee &&
        !soldier.GetActiveWeapon().IsAny(
          { WeaponType::NoWeapon, WeaponType::Knife, WeaponType::Chainsaw })) {
        const auto* target = state_manager.FindSoldier([&](const Soldier& candidate) {
            return candidate.id != soldier.id && candidate.IsAlive() && !candidate.IsSpectator() &&
                   candidate.stance == PhysicsConstants::STANCE_STAND &&
                   glm::distance(candidate.particle.position, soldier.particle.position) < 12.0F;
        });
        if (target != nullptr) {
            soldier.body_animation =
              CreateBodyAnimationState(AnimationType::Melee, animation_data_manager);
        }
    }
}

void TryFireWeapon(StateManager& state_manager,
                   Soldier& soldier,
                   const PhysicsEvents& physics_events,
                   const AnimationDataManager& animation_data_manager)
{
    const auto& map = state_manager.GetConstMap();
    const auto notify_weapon_action = [&](WeaponActionKind kind, const WeaponState& state) {
        const WeaponActionEvent event{
            kind,
            soldier.id,
            state_manager.GetGameTick(),
            state,
            static_cast<std::uint8_t>(
              kind == WeaponActionKind::GrenadeThrown ? 2 : soldier.active_weapon)
        };
        physics_events.weapon_action_event.Notify(event);
    };
    AnimationState::TryToShootParams try_to_shoot_params{
        soldier.weapons,
        soldier.active_weapon,
        soldier.control.fire || soldier.body_animation->GetType() == AnimationType::Punch ||
          soldier.body_animation->GetType() == AnimationType::Melee,
        false
    };
    soldier.body_animation->TryToShoot(try_to_shoot_params);
    if (try_to_shoot_params.should_fire_primary_weapon) {
        std::vector<BulletParams> shots;
        const auto before_shot = soldier.GetActiveWeapon().GetState();
        const bool accepted = WeaponPhysics::Fire(soldier, shots);
        const auto after_shot = soldier.GetActiveWeapon().GetState();
        if (after_shot.startup < before_shot.startup &&
            before_shot.startup == soldier.GetActiveWeapon().GetWeaponParameters().start_up_time) {
            notify_weapon_action(WeaponActionKind::StartupStarted, after_shot);
        }
        if (accepted) {
            notify_weapon_action(WeaponActionKind::Fired, after_shot);
            if (state_manager.HasAuthoritativeProjectiles()) {
                for (auto& params : shots) {
                    glm::vec2 perpendicular{};
                    if (map.CollisionTest(params.position, perpendicular)) {
                        params.position.y += 2.5F;
                    }
                    state_manager.EnqueueNewProjectile(params);
                }
            }
            ApplyFiringAnimation(soldier, animation_data_manager);
            physics_events.soldier_fires_primary_weapon.Notify(soldier);
        }
    } else {
        soldier.GetActiveWeapon().ResetStartUpTimeCount();
    }
}

void Update(StateManager& state_manager,
            Soldier& soldier,
            const PhysicsEvents& physics_events,
            const AnimationDataManager& animation_data_manager,
            std::vector<BulletParams>& bullet_emitter,
            float gravity)
{
    if (!soldier.active || soldier.IsSpectator()) {
        return;
    }
    if (soldier.combat_bonus == CombatBonus::FlameGod) {
        soldier.control.change = false;
        soldier.control.drop = false;
    }
    if (soldier.GetActiveWeapon().IsAny({ WeaponType::Bow, WeaponType::FlameBow })) {
        soldier.control.drop = false;
    }
    const auto notify_weapon_action = [&](WeaponActionKind kind, const WeaponState& state) {
        const WeaponActionEvent event{
            kind,
            soldier.id,
            state_manager.GetGameTick(),
            state,
            static_cast<std::uint8_t>(
              kind == WeaponActionKind::GrenadeThrown ? 2 : soldier.active_weapon)
        };
        physics_events.weapon_action_event.Notify(event);
    };
    const Map& map = state_manager.GetConstMap();
    FlagCarry::Refresh(state_manager, soldier);
    soldier.polygon_contacts.clear();

    float body_y = 0.0F;
    float arm_s = 0.0F;

    soldier.particle.Euler();
    PolygonContacts::Begin(
      map, soldier.polygon_background, soldier.background_touched, soldier.particle.position);

    if (!soldier.control.fire) {
        soldier.is_shooting = false;
    }

    if (soldier.legs_animation->GetSpeed() < 1) {
        soldier.legs_animation->SetSpeed(1);
    }

    if (soldier.legs_animation->GetSpeed() < 1) {
        soldier.legs_animation->SetSpeed(1);
    }

    if (soldier.body_animation->GetSpeed() < 1) {
        soldier.body_animation->SetSpeed(1);
    }

    auto conflicting_keys_pressed = [](const Control& c) {
        return ((int)c.throw_grenade + (int)c.change + (int)c.drop + (int)c.reload) > 1;
    };

    // Handle simultaneous key presses that would conflict
    if (conflicting_keys_pressed(soldier.control)) {
        // At least two buttons pressed, so deactivate any previous one
        if (soldier.control.was_throwing_grenade) {
            soldier.control.throw_grenade = false;
        } else if (soldier.control.was_changing_weapon) {
            soldier.control.change = false;
        } else if (soldier.control.was_throwing_weapon) {
            soldier.control.drop = false;
        } else if (soldier.control.was_reloading_weapon) {
            soldier.control.reload = false;
        }

        // If simultaneously pressing two or more new buttons, then deactivate them
        // in order of least preference
        while (conflicting_keys_pressed(soldier.control)) {
            if (soldier.control.reload) {
                soldier.control.reload = false;
            } else if (soldier.control.change) {
                soldier.control.change = false;
            } else if (soldier.control.drop) {
                soldier.control.drop = false;
            } else if (soldier.control.throw_grenade) {
                soldier.control.throw_grenade = false;
            }
        }
    } else {
        soldier.control.was_throwing_grenade = soldier.control.throw_grenade;
        soldier.control.was_changing_weapon = soldier.control.change;
        soldier.control.was_throwing_weapon = soldier.control.drop;
        soldier.control.was_reloading_weapon = soldier.control.reload;
    }

    // self.fired = 0;
    soldier.control.mouse_aim_x =
      (int)((float)soldier.control.mouse_aim_x + soldier.particle.GetVelocity().x);
    soldier.control.mouse_aim_y =
      (int)((float)soldier.control.mouse_aim_y + soldier.particle.GetVelocity().y);

    const bool authoritative = state_manager.HasAuthoritativeProjectiles();
    const auto idle_style = authoritative && soldier.idle_random < 0 && soldier.idle_time <= 1
                              ? state_manager.GetRandomInt(0, 3) : 0;
    SoldierEmotes::Update(soldier, animation_data_manager, authoritative, idle_style);

    AnimationState::HandleInputParams legs_handle_input_params{
        soldier.control,          soldier.weapons,    soldier.stance,
        soldier.active_weapon,    soldier.on_ground,  soldier.direction,
        soldier.old_direction,    soldier.jets_count, soldier.legs_animation->GetType(),
        soldier.grenade_can_throw
    };
    auto maybe_legs_animation_transition =
      soldier.legs_animation->HandleInput(legs_handle_input_params);
    soldier.control = legs_handle_input_params.control;
    soldier.old_direction = legs_handle_input_params.old_direction;
    soldier.grenade_can_throw = legs_handle_input_params.grenade_can_throw;
    if (maybe_legs_animation_transition.has_value()) {
        AnimationState::ExitParams exit_params{ false };
        soldier.legs_animation->Exit(exit_params);
        if (exit_params.should_throw_active_weapon) {
            const auto dropped = soldier.GetActiveWeapon().GetState();
            WeaponPhysics::ThrowWeapon(state_manager, soldier);
            if (soldier.GetActiveWeapon().GetState().kind != dropped.kind) {
                notify_weapon_action(WeaponActionKind::Dropped, dropped);
            }
            physics_events.soldier_throws_active_weapon.Notify(soldier);
        }
        soldier.legs_animation = CreateLegsAnimationState(
          maybe_legs_animation_transition->animation_type, animation_data_manager);
        ApplyTransitionInitialFrame(*soldier.legs_animation, *maybe_legs_animation_transition);
        AnimationState::EnterParams enter_params{
            soldier.on_ground,         soldier.direction, soldier.particle.GetForce(),
            soldier.grenade_can_throw, soldier.weapons,   soldier.active_weapon
        };
        soldier.legs_animation->Enter(enter_params);
        soldier.particle.SetForce(enter_params.force);
        soldier.grenade_can_throw = enter_params.grenade_can_throw;
    }

    AdvanceWeapons(state_manager, soldier, physics_events, animation_data_manager);
    TryButtstock(state_manager, soldier, animation_data_manager);
    TryFireWeapon(state_manager, soldier, physics_events, animation_data_manager);

    AnimationState::TryToThrowFlagsParams try_to_throw_flags_params{ soldier.control.flag_throw,
                                                                     soldier.is_holding_flags,
                                                                     false };
    soldier.body_animation->TryToThrowFlags(try_to_throw_flags_params);
    if (try_to_throw_flags_params.should_throw_flags) {
        state_manager.ThrowSoldierFlags(soldier.id);
        physics_events.soldier_throws_flags.Notify(soldier);
    }

    AnimationState::HandleInputParams body_handle_input_params{
        soldier.control,          soldier.weapons,    soldier.stance,
        soldier.active_weapon,    soldier.on_ground,  soldier.direction,
        soldier.old_direction,    soldier.jets_count, soldier.legs_animation->GetType(),
        soldier.grenade_can_throw
    };
    auto maybe_body_animation_transition =
      soldier.body_animation->HandleInput(body_handle_input_params);
    soldier.control = body_handle_input_params.control;
    soldier.old_direction = body_handle_input_params.old_direction;
    soldier.grenade_can_throw = body_handle_input_params.grenade_can_throw;
    if (maybe_body_animation_transition.has_value()) {
        AnimationState::ExitParams exit_params{ false };
        soldier.body_animation->Exit(exit_params);
        if (exit_params.should_throw_active_weapon) {
            const auto dropped = soldier.GetActiveWeapon().GetState();
            WeaponPhysics::ThrowWeapon(state_manager, soldier);
            if (soldier.GetActiveWeapon().GetState().kind != dropped.kind) {
                notify_weapon_action(WeaponActionKind::Dropped, dropped);
            }
            physics_events.soldier_throws_active_weapon.Notify(soldier);
        }
        soldier.body_animation = CreateBodyAnimationState(
          maybe_body_animation_transition->animation_type, animation_data_manager);
        ApplyTransitionInitialFrame(*soldier.body_animation, *maybe_body_animation_transition);
        AnimationState::EnterParams enter_params{
            soldier.on_ground,         soldier.direction, soldier.particle.GetForce(),
            soldier.grenade_can_throw, soldier.weapons,   soldier.active_weapon
        };
        soldier.body_animation->Enter(enter_params);
        soldier.particle.SetForce(enter_params.force);
        soldier.grenade_can_throw = enter_params.grenade_can_throw;
    }

    AnimationState::UpdateParams legs_update_params{ soldier.control,
                                                     soldier.direction,
                                                     soldier.on_ground,
                                                     soldier.stance,
                                                     soldier.particle.GetVelocity(),
                                                     soldier.particle.GetForce(),
                                                     false };
    soldier.legs_animation->Update(legs_update_params);
    soldier.stance = legs_update_params.stance;
    soldier.particle.SetVelocity(legs_update_params.velocity);
    soldier.particle.SetForce(legs_update_params.force);
    if (legs_update_params.should_switch_weapon && soldier.combat_bonus != CombatBonus::FlameGod) {
        state_manager.SwitchSoldierWeapon(soldier.id);
        notify_weapon_action(WeaponActionKind::Switched, soldier.GetActiveWeapon().GetState());
        physics_events.soldier_switches_weapon.Notify(soldier);
    }

    AnimationState::UpdateParams body_update_params{ soldier.control,
                                                     soldier.direction,
                                                     soldier.on_ground,
                                                     soldier.stance,
                                                     soldier.particle.GetVelocity(),
                                                     soldier.particle.GetForce(),
                                                     false };
    soldier.body_animation->Update(body_update_params);
    soldier.stance = body_update_params.stance;
    soldier.particle.SetVelocity(body_update_params.velocity);
    soldier.particle.SetForce(body_update_params.force);
    if (body_update_params.should_switch_weapon && soldier.combat_bonus != CombatBonus::FlameGod) {
        state_manager.SwitchSoldierWeapon(soldier.id);
        notify_weapon_action(WeaponActionKind::Switched, soldier.GetActiveWeapon().GetState());
        physics_events.soldier_switches_weapon.Notify(soldier);
    }

    bool jets_can_be_applied = true;
    if (soldier.legs_animation->GetType() == AnimationType::RollBack && soldier.control.up) {
        // jets cannot be applied during RollBack animation when "up" is pressed because
        // that animation adds its forces on the soldier
        jets_can_be_applied = false;
    }

    if (jets_can_be_applied && soldier.control.jets && (soldier.jets_count > 0)) {
        if (soldier.on_ground) {
            glm::vec2 particle_force = soldier.particle.GetForce();
            if (gravity > 0.05F) {
                soldier.particle.SetForce({ particle_force.x, -2.5F * PhysicsConstants::JETSPEED });
            } else {
                soldier.particle.SetForce({ particle_force.x, -2.5F * (gravity * 2.0F) });
            }
        } else if (soldier.stance != PhysicsConstants::STANCE_PRONE) {
            glm::vec2 particle_force = soldier.particle.GetForce();
            if (gravity > 0.05F) {
                soldier.particle.SetForce(
                  { particle_force.x, particle_force.y - PhysicsConstants::JETSPEED });
            } else {
                soldier.particle.SetForce({ particle_force.x, particle_force.y - gravity * 2.0F });
            }
        } else {
            glm::vec2 particle_force = soldier.particle.GetForce();
            soldier.particle.SetForce(
              { particle_force.x +
                  (float)soldier.direction *
                    (gravity > 0.05F ? PhysicsConstants::JETSPEED / 2.0F : gravity),
                particle_force.y });
        }
        soldier.jets_count -= 1;
    }

    RepositionSoldierSkeletonParts(soldier);

    if (soldier.dead_meat || soldier.half_dead) {
        soldier.on_ground = false;
        for (unsigned int i = 1; i <= 16; ++i) {
            if (i != 7 && i != 8) {
                const auto position = soldier.skeleton->GetPos(i);
                if (CheckSkeletonMapCollision(soldier, map, i, position.x, position.y)) {
                    soldier.on_ground = true;
                }
            }
        }
    }

    if (!soldier.dead_meat) {
        soldier.body_animation->DoAnimation();
        soldier.legs_animation->DoAnimation();

        soldier.on_ground = false;

        auto xy = soldier.particle.position;

        CheckMapCollision(soldier, map, xy.x - 3.5, xy.y - 12.0, 1, physics_events);

        xy = soldier.particle.position;

        CheckMapCollision(soldier, map, xy.x + 3.5, xy.y - 12.0, 1, physics_events);

        body_y = 0.0;
        arm_s = 0.0;

        // Walking either left or right (though only one can be active at once)
        if (soldier.control.left ^ soldier.control.right) {
            if (soldier.control.left ^ (soldier.direction == 1)) {
                // WRONG
                arm_s = 0.25;
            } else {
                body_y = 0.25;
            }
        }
        // If a leg is inside a polygon, caused by the modification of ArmS and
        // BodyY, this is there to not lose contact to ground on slope polygons
        if (std::abs(body_y) <= 0.00001) {
            glm::vec2 leg_vector = { soldier.particle.position.x + 2.0F,
                                     soldier.particle.position.y + 1.9F };
            float leg_distance = 0.0F;
            if (map.RayCast(leg_vector, leg_vector, leg_distance, 10)) {
                body_y = 0.25;
            }
        }
        if (std::abs(arm_s) <= 0.00001) {
            glm::vec2 leg_vector = { soldier.particle.position.x - 2.0F,
                                     soldier.particle.position.y + 1.9F };
            float leg_distance = 0.0F;
            if (map.RayCast(leg_vector, leg_vector, leg_distance, 10)) {
                arm_s = 0.25;
            }
        }

        xy = soldier.particle.position;
        soldier.on_ground =
          CheckMapCollision(soldier, map, xy.x + 2.0, xy.y + 2.0 - body_y, 0, physics_events);

        xy = soldier.particle.position;
        soldier.on_ground =
          soldier.on_ground ||
          CheckMapCollision(soldier, map, xy.x - 2.0, xy.y + 2.0 - arm_s, 0, physics_events);

        xy = soldier.particle.position;
        auto grounded = soldier.on_ground;
        soldier.on_ground_for_law =
          CheckRadiusMapCollision(soldier, map, xy.x, xy.y - 1.0, grounded, physics_events);

        xy = soldier.particle.position;
        grounded = soldier.on_ground || soldier.on_ground_for_law;
        soldier.on_ground =
          CheckMapVerticesCollision(soldier, map, xy.x, xy.y, 3.0, grounded, physics_events) ||
          soldier.on_ground;

        if (!(soldier.on_ground ^ soldier.on_ground_last_frame)) {
            soldier.on_ground_permanent = soldier.on_ground;
        }

        soldier.on_ground_last_frame = soldier.on_ground;

        if ((soldier.jets_count < map.GetJetCount()) && !(soldier.control.jets)) {
            // if self.on_ground
            /* (MainTickCounter mod 2 = 0) */
            {
                soldier.jets_count += 1;
            }
        }
        soldier.alpha = 255;

        state_manager.TransformItems([&](auto& item) {
            if (item.holding_soldier_id == soldier.id && item.style == ItemType::Parachute) {
                glm::vec2 force = soldier.particle.GetForce();
                soldier.particle.SetForce(
                  { force.x, PhysicsConstants::SOLDIER_SPEED_WITH_PARACHUTE });
                if (authoritative && soldier.GetCeaseFireCounter() < 1 &&
                    (soldier.on_ground || soldier.control.jets)) {
                    item.holding_soldier_id = -1;
                    item.time_out = 3 * 60;
                }
            }
        });

        soldier.skeleton->DoVerletTimestepFor(22, 29);
        soldier.skeleton->DoVerletTimestepFor(24, 30);
    }

    if (soldier.dead_meat) {
        soldier.skeleton->DoVerletTimestep();
        soldier.particle.position = soldier.skeleton->GetPos(12);
        // CheckSkeletonOutOfBounds;

        state_manager.TransformItems([&](auto& item) {
            if (item.holding_soldier_id == soldier.id && item.style == ItemType::Parachute) {
                glm::vec2 force = soldier.skeleton->GetForce(12);
                soldier.skeleton->SetForce(
                  12, { force.x, 25 * PhysicsConstants::SOLDIER_SPEED_WITH_PARACHUTE });
                if (authoritative && soldier.on_ground) {
                    item.holding_soldier_id = -1;
                    item.time_out = 3 * 60;
                }
            }
        });
    }

    PolygonContacts::Finish(soldier.polygon_background, soldier.background_touched);
    PolygonContactEffects::ApplyAll(state_manager, soldier, physics_events);
    if (soldier.particle.velocity_.x > PhysicsConstants::MAX_VELOCITY) {
        soldier.particle.velocity_.x = PhysicsConstants::MAX_VELOCITY;
    }
    if (soldier.particle.velocity_.x < -PhysicsConstants::MAX_VELOCITY) {
        soldier.particle.velocity_.x = -PhysicsConstants::MAX_VELOCITY;
    }
    if (soldier.particle.velocity_.y > PhysicsConstants::MAX_VELOCITY) {
        soldier.particle.velocity_.y = PhysicsConstants::MAX_VELOCITY;
    }
    if (soldier.particle.velocity_.y < -PhysicsConstants::MAX_VELOCITY) {
        soldier.particle.velocity_.y = -PhysicsConstants::MAX_VELOCITY;
    }
}

bool CheckMapCollision(Soldier& soldier,
                       const Map& map,
                       float x,
                       float y,
                       int area,
                       const PhysicsEvents& physics_events)
{
    auto pos = glm::vec2(x, y) + soldier.particle.velocity_;
    const glm::ivec2 sector_index = map.GetSectorIndex(pos);
    auto rx = sector_index.x;
    auto ry = sector_index.y;

    if ((rx > 0) && (rx < 2 * map.GetSectorsCount()) && (ry > 0) &&
        (ry < 2 * map.GetSectorsCount())) {
        for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
            auto poly = map.GetSector(rx, ry).polygons[j] - 1;
            auto polytype = map.GetPolygons()[poly].polygon_type;

            if (IsValidPolygonType(polytype)) {
                auto polygons = map.GetPolygons()[poly];

                if (Map::PointInPoly(pos, polygons) &&
                    PolygonContacts::SoldierContact(soldier, map, poly, pos)) {
                    RecordContact(soldier, polygons, pos, PolygonProbe::Body);
                    const float fall_speed = std::abs(soldier.particle.GetVelocity().y);
                    if ((fall_speed > 2.2F && fall_speed < 3.4F &&
                         polytype != PMSPolygonType::Bouncy) ||
                        fall_speed > 3.5F) {
                        RecordContact(soldier, polygons, pos, PolygonProbe::Landing);
                    }

                    auto dist = 0.0F;
                    auto k = 0;

                    auto perp = map.ClosestPerpendicular(poly, pos, &dist, &k);

                    auto step = perp;

                    perp = Calc::Vec2Normalize(perp);
                    perp *= dist;
                    dist = Calc::Vec2Length(soldier.particle.velocity_);

                    if (Calc::Vec2Length(perp) > dist) {
                        perp = Calc::Vec2Normalize(perp);
                        perp *= dist;
                    }
                    if ((area == 0) ||
                        ((area == 1) &&
                         ((soldier.particle.velocity_.y < 0.0) ||
                          (soldier.particle.velocity_.x > PhysicsConstants::SLIDELIMIT) ||
                          (soldier.particle.velocity_.x < -PhysicsConstants::SLIDELIMIT)))) {
                        soldier.particle.old_position = soldier.particle.position;
                        soldier.particle.position -= perp;
                        if (map.GetPolygons()[poly].polygon_type == PMSPolygonType::Bouncy) {
                            perp = PolygonSurfaceResponse::BounceImpulse(
                              perp, soldier.particle.GetVelocity(), polygons.bounciness);
                            if (glm::length(perp) > 1.0F) {
                                soldier.polygon_contacts.push_back(
                                  { polygons.id, polytype, pos, perp, PolygonProbe::Bounce });
                            }
                        }
                        soldier.particle.velocity_ -= perp;
                    }

                    if (area == 0) {
                        if ((soldier.legs_animation->GetType() == AnimationType::Stand) ||
                            (soldier.legs_animation->GetType() == AnimationType::Crouch) ||
                            (soldier.legs_animation->GetType() == AnimationType::Prone) ||
                            (soldier.legs_animation->GetType() == AnimationType::ProneMove) ||
                            (soldier.legs_animation->GetType() == AnimationType::GetUp) ||
                            (soldier.legs_animation->GetType() == AnimationType::Fall) ||
                            (soldier.legs_animation->GetType() == AnimationType::Mercy) ||
                            (soldier.legs_animation->GetType() == AnimationType::Mercy2) ||
                            (soldier.legs_animation->GetType() == AnimationType::Own)) {
                            if ((soldier.particle.velocity_.x < PhysicsConstants::SLIDELIMIT) &&
                                (soldier.particle.velocity_.x > -PhysicsConstants::SLIDELIMIT) &&
                                (step.y > PhysicsConstants::SLIDELIMIT)) {
                                soldier.particle.position = soldier.particle.old_position;
                                glm::vec2 particle_force = soldier.particle.GetForce();
                                particle_force.y -= PhysicsConstants::GRAV;
                                soldier.particle.SetForce(particle_force);
                            }
                        }
                        const auto response = PolygonSurfaceResponse::Friction(
                          GetPolygonTraits(polytype).surface,
                          soldier.legs_animation->GetType(),
                          soldier.legs_animation->GetFrame(),
                          soldier.control.down && (soldier.control.left || soldier.control.right),
                          step,
                          soldier.particle.GetVelocity(),
                          soldier.particle.GetForce());
                        soldier.particle.SetVelocity(response.velocity);
                        soldier.particle.SetForce(response.force);
                    }

                    physics_events.soldier_collides_with_polygon.Notify(soldier, polygons);
                    return true;
                }
            }
        }
    }

    return false;
}

bool CheckMapVerticesCollision(Soldier& soldier,
                               const Map& map,
                               float x,
                               float y,
                               float r,
                               bool has_collided,
                               const PhysicsEvents& physics_events)
{
    auto pos = glm::vec2(x, y);
    const glm::ivec2 sector_index = map.GetSectorIndex(pos);
    auto rx = sector_index.x;
    auto ry = sector_index.y;

    if ((rx > 0) && (rx < 2 * map.GetSectorsCount()) && (ry > 0) &&
        (ry < 2 * map.GetSectorsCount())) {
        for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); ++j) {
            auto poly = map.GetSector(rx, ry).polygons[j] - 1;
            PMSPolygonType polytype = map.GetPolygons()[poly].polygon_type;

            // TODO: this if has more poly types
            if (IsValidPolygonType(polytype)) {
                for (int i = 0; i < 3; i++) {
                    auto vert = glm::vec2(map.GetPolygons()[poly].vertices[i].x,
                                          map.GetPolygons()[poly].vertices[i].y);

                    auto dist = Calc::Distance(vert, pos);
                    if (dist < r && PolygonContacts::SoldierContact(soldier, map, poly, pos)) {
                        if (!has_collided) {
                            RecordContact(
                              soldier, map.GetPolygons()[poly], pos, PolygonProbe::Vertex);
                        }
                        auto dir = pos - vert;
                        dir = Calc::Vec2Normalize(dir);
                        soldier.particle.position += dir;
                        physics_events.soldier_collides_with_polygon.Notify(
                          soldier, map.GetPolygons()[poly]);

                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool CheckRadiusMapCollision(Soldier& soldier,
                             const Map& map,
                             float x,
                             float y,
                             bool has_collided,
                             const PhysicsEvents& physics_events)
{
    auto s_pos = glm::vec2(x, y - 3.0f);

    auto det_acc = (int)Calc::Vec2Length(soldier.particle.velocity_);
    if (det_acc == 0) {
        det_acc = 1;
    }

    auto step = soldier.particle.velocity_ * (1.0f / (float)det_acc);

    for (int _z = 0; _z < det_acc; _z++) {
        s_pos += step;

        const glm::ivec2 sector_index = map.GetSectorIndex(s_pos);
        auto rx = sector_index.x;
        auto ry = sector_index.y;

        if ((rx > 0) && (rx < 2 * map.GetSectorsCount()) && (ry > 0) &&
            (ry < 2 * map.GetSectorsCount())) {
            for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
                auto poly = map.GetSector(rx, ry).polygons[j] - 1;
                auto polytype = map.GetPolygons()[poly].polygon_type;

                if (IsValidPolygonType(polytype)) {
                    for (int k = 0; k < 3; k++) {
                        auto norm = glm::vec2(map.GetPolygons()[poly].perpendiculars[k].x,
                                              map.GetPolygons()[poly].perpendiculars[k].y);
                        norm *= -PhysicsConstants::SOLDIER_COL_RADIUS;

                        auto pos = s_pos + norm;

                        if (map.PointInPolyEdges(pos.x, pos.y, poly) &&
                            PolygonContacts::SoldierContact(soldier, map, poly, pos)) {
                            if (!has_collided) {
                                RecordContact(
                                  soldier, map.GetPolygons()[poly], pos, PolygonProbe::Radius);
                            }
                            auto d = 0.0f;
                            auto b = 0;
                            auto perp = map.ClosestPerpendicular(poly, s_pos, &d, &b);

                            auto p1 = glm::vec2(0.0, 0.0);
                            auto p2 = glm::vec2(0.0, 0.0);
                            switch (b) {
                                case 1: {
                                    p1 = glm::vec2(map.GetPolygons()[poly].vertices[0].x,
                                                   map.GetPolygons()[poly].vertices[0].y);
                                    p2 = glm::vec2(map.GetPolygons()[poly].vertices[1].x,
                                                   map.GetPolygons()[poly].vertices[1].y);
                                    break;
                                }
                                case 2: {
                                    p1 = glm::vec2(map.GetPolygons()[poly].vertices[1].x,
                                                   map.GetPolygons()[poly].vertices[1].y);
                                    p2 = glm::vec2(map.GetPolygons()[poly].vertices[2].x,
                                                   map.GetPolygons()[poly].vertices[2].y);
                                    break;
                                }
                                case 3: {
                                    p1 = glm::vec2(map.GetPolygons()[poly].vertices[2].x,
                                                   map.GetPolygons()[poly].vertices[2].y);
                                    p2 = glm::vec2(map.GetPolygons()[poly].vertices[0].x,
                                                   map.GetPolygons()[poly].vertices[0].y);
                                    break;
                                }
                            }

                            auto p3 = pos;
                            d = Calc::PointLineDistance(p1, p2, p3);
                            perp *= d;

                            soldier.particle.position = soldier.particle.old_position;
                            soldier.particle.velocity_ = soldier.particle.GetForce() - perp;
                            physics_events.soldier_collides_with_polygon.Notify(
                              soldier, map.GetPolygons()[poly]);

                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool CheckSkeletonMapCollision(Soldier& soldier, const Map& map, unsigned int i, float x, float y)
{
    auto result = false;
    auto pos = glm::vec2(x - 1.0f, y + 4.0f);
    const glm::ivec2 sector_index = map.GetSectorIndex(pos);
    auto rx = sector_index.x;
    auto ry = sector_index.y;

    if ((rx > 0) && (rx < 2 * map.GetSectorsCount()) && (ry > 0) &&
        (ry < 2 * map.GetSectorsCount())) {
        for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
            auto poly = map.GetSector(rx, ry).polygons[j] - 1;

            if (map.PointInPolyEdges(pos.x, pos.y, poly) &&
                PolygonContacts::SoldierContact(soldier, map, poly, pos)) {
                auto dist = 0.0f;
                auto b = 0;
                auto perp = map.ClosestPerpendicular(poly, pos, &dist, &b);
                perp = Calc::Vec2Normalize(perp);
                perp *= dist;

                soldier.skeleton->SetPos(i, soldier.skeleton->GetOldPos(i) - perp);

                result = true;
            }
        }
    }

    if (result) {
        auto pos = glm::vec2(x, y + 1.0);
        const glm::ivec2 sector_index = map.GetSectorIndex(pos);
        auto rx = sector_index.x;
        auto ry = sector_index.y;

        if ((rx > 0) && (rx < 2 * map.GetSectorsCount()) && (ry > 0) &&
            (ry < 2 * map.GetSectorsCount())) {
            for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
                auto poly = map.GetSector(rx, ry).polygons[j] - 1;
                // if (Map.PolyType[poly] <> POLY_TYPE_DOESNT) and (Map.PolyType[poly] <>
                // POLY_TYPE_ONLY_BULLETS) then
                if (map.PointInPolyEdges(pos.x, pos.y, poly) &&
                    PolygonContacts::SoldierContact(soldier, map, poly, pos)) {
                    auto dist = 0.0F;
                    auto b = 0;
                    auto perp = map.ClosestPerpendicular(poly, pos, &dist, &b);
                    perp = Calc::Vec2Normalize(perp);
                    perp *= dist;

                    soldier.skeleton->SetPos(i, soldier.skeleton->GetOldPos(i) - perp);

                    result = true;
                }
            }
        }
    }

    return result;
}

bool CanFireLaw(const Soldier& soldier)
{
    return WeaponPhysics::CanFireLaw(soldier);
}

void Fire(Soldier& soldier, std::vector<BulletParams>& bullet_emitter)
{
    WeaponPhysics::Fire(soldier, bullet_emitter);
}
} // namespace Soldank::SoldierPhysics
