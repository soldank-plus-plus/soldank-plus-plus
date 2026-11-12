module;

#include <memory>
#include <algorithm>
#include <cstdint>

export module Networking.ProjectilePhysicsNetworkEventHandler;

import Extern.Glm;
import ClientState;
import Shared.Core.IWorld;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.WeaponType;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Physics.Bullets.BulletTypes;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkEventDispatcher;

export namespace Soldank
{
class ProjectilePhysicsNetworkEventHandler : public NetworkEventHandlerBase<ProjectilePhysicsPacket>
{
public:
    ProjectilePhysicsNetworkEventHandler(const std::shared_ptr<IWorld>& world,
                                         const std::shared_ptr<ClientState>& client_state)
        : world_(world)
        , client_state_(client_state)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::ProjectilePhysics; }

    void ApplySoldierEffect(const ProjectilePhysicsPacket& packet, glm::vec2 impulse)
    {
        auto state = world_->GetStateManager();
        if (packet.soldier_id != 255) {
            state->TransformSoldiers([&](Soldier& soldier) {
                if (soldier.id != packet.soldier_id) {
                    return;
                }
                if (packet.kind == ProjectileEventKind::SoldierProjectilePushScheduled) {
                    const auto local_apply_tick = static_cast<unsigned int>(std::max<std::int64_t>(
                      0,
                      static_cast<std::int64_t>(packet.apply_tick) -
                        client_state_->network.server_tick_offset.value_or(0)));
                    if (!soldier.latest_authoritative_tick ||
                        packet.apply_tick > *soldier.latest_authoritative_tick) {
                        soldier.AddProjectilePush(local_apply_tick, impulse);
                        soldier.ApplyProjectilePushes(state->GetGameTick());
                    }
                } else if (packet.kind == ProjectileEventKind::ExplosionAffectedThing &&
                           soldier.skeleton && packet.particle_id > 0 &&
                           packet.particle_id <= soldier.skeleton->GetParticles().size()) {
                    soldier.skeleton->SetOldPos(packet.particle_id,
                                                soldier.skeleton->GetOldPos(packet.particle_id) -
                                                  impulse);
                } else if (packet.kind == ProjectileEventKind::WorldItemSpawnedFromProjectile) {
                    soldier.GetActiveWeapon() =
                      WeaponParametersFactory::GetParameters(WeaponType::NoWeapon, false);
                }
            });
        }
    }

    void ApplyItemEffect(const ProjectilePhysicsPacket& packet, glm::vec2 impulse)
    {
        auto state = world_->GetStateManager();
        if (packet.kind == ProjectileEventKind::WorldItemSpawnedFromProjectile) {
            state->CreateItem(
              { packet.position_x, packet.position_y }, packet.owner_id, packet.item_style);
        } else if (packet.item_id != 255 &&
                   (packet.kind == ProjectileEventKind::ExplosionAffectedThing ||
                    packet.kind == ProjectileEventKind::ThingHitByProjectile)) {
            state->TransformItems([&](Item& item) {
                if (item.id != packet.item_id || !item.skeleton || packet.particle_id == 0 ||
                    packet.particle_id > item.skeleton->GetParticles().size()) {
                    return;
                }
                if (packet.kind == ProjectileEventKind::ThingHitByProjectile) {
                    item.skeleton->SetPos(packet.particle_id,
                                          item.skeleton->GetPos(packet.particle_id) + impulse);
                } else {
                    item.skeleton->SetOldPos(
                      packet.particle_id, item.skeleton->GetOldPos(packet.particle_id) - impulse);
                }
                item.static_type = false;
            });
        }
    }

    void ApplyProjectileEffect(const ProjectilePhysicsPacket& packet)
    {
        world_->GetStateManager()->TransformBullets([&](Bullet& bullet) {
            if (bullet.id != packet.projectile_id) {
                return;
            }
            switch (packet.kind) {
                case ProjectileEventKind::ProjectileDestroyed:
                case ProjectileEventKind::ProjectileExploded:
                    bullet.active = false;
                    break;
                case ProjectileEventKind::ProjectileBounced:
                case ProjectileEventKind::ProjectileRicocheted:
                case ProjectileEventKind::ProjectilePenetratedSoldier:
                case ProjectileEventKind::ProjectileStuck:
                    bullet.particle.position = { packet.position_x, packet.position_y };
                    bullet.particle.old_position = bullet.particle.position;
                    bullet.particle.SetVelocity(
                      { packet.velocity_after_x, packet.velocity_after_y });
                    if (packet.kind == ProjectileEventKind::ProjectileStuck) {
                        bullet.stuck = true;
                        if (packet.soldier_id != 255 && packet.particle_id > 0) {
                            bullet.attached_soldier = packet.soldier_id;
                            bullet.attached_part = packet.particle_id;
                        }
                    }
                    break;
                default:
                    break;
            }
        });
    }

    static ProjectilePhysicsEvent DecodeEvent(const ProjectilePhysicsPacket& packet)
    {
        const glm::vec2 impulse{ packet.impulse_x, packet.impulse_y };
        ProjectilePhysicsEvent event{
            .kind = packet.kind,
            .projectile_id = packet.projectile_id,
            .style = packet.style,
            .weapon = packet.weapon,
            .owner_id = packet.owner_id,
            .seed = packet.seed,
            .tick = packet.tick,
            .position = { packet.position_x, packet.position_y },
            .impulse = impulse,
            .radius = packet.radius,
            .apply_tick = packet.apply_tick,
        };
        if (packet.soldier_id != 255) {
            event.soldier_id = packet.soldier_id;
        }
        if (packet.item_id != 255) {
            event.item_id = packet.item_id;
        }
        event.velocity_before = { packet.velocity_before_x, packet.velocity_before_y };
        event.velocity_after = { packet.velocity_after_x, packet.velocity_after_y };
        event.damage = packet.damage;
        event.distance = packet.distance;
        event.item_style = packet.item_style;
        event.reason = packet.reason;
        if (packet.particle_id != 0) {
            event.particle_id = packet.particle_id;
        }
        if (packet.target_projectile_id != 0) {
            event.target_projectile_id = packet.target_projectile_id;
        }
        if (packet.has_collision) {
            event.collision = BulletCollisionResult{
                .kind = packet.collision_kind,
                .position = event.position,
                .distance = packet.distance,
                .surface_normal = glm::vec2{ packet.normal_x, packet.normal_y },
            };
            event.collision->soldier_id = event.soldier_id;
            event.collision->item_id = event.item_id;
            if (packet.particle_id != 0) {
                event.collision->body_part_id = static_cast<int>(packet.particle_id);
                event.collision->item_particle_id = packet.particle_id;
            }
            if (packet.polygon_id != 0xffffffffU) {
                event.collision->polygon_id = packet.polygon_id;
            }
            if (packet.collider_id != 0xffffffffU) {
                event.collision->collider_id = packet.collider_id;
            }
        }
        return event;
    }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender*/,
                                                       ProjectilePhysicsPacket packet) override
    {
        const glm::vec2 impulse{ packet.impulse_x, packet.impulse_y };
        ApplyProjectileEffect(packet);
        ApplySoldierEffect(packet, impulse);
        ApplyItemEffect(packet, impulse);
        const auto event = DecodeEvent(packet);
        world_->GetPhysicsEvents().projectile_event.Notify(event);
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
    std::shared_ptr<ClientState> client_state_;
};
} // namespace Soldank
