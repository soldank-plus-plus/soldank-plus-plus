module;

#include <cstdint>
#include <array>
#include <optional>
#include <random>
#include <vector>

export module Shared.Core.Physics.Bullets.BulletEffects;

import Extern.Glm;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.ItemType;
import Shared.Core.State.StateManager;
import Shared.Core.Physics.Bullets.BulletEvents;

export namespace Soldank::BulletEffects
{
void CreateKnife(StateManager& state, const Bullet& bullet, glm::vec2 position)
{
    if (!state.HasAuthoritativeProjectiles()) {
        return;
    }
    const auto& item = state.CreateItem(position, bullet.owner_id, ItemType::Knife);
    auto event = MakeProjectileEvent(
      ProjectileEventKind::WorldItemSpawnedFromProjectile, bullet, state.GetGameTick());
    event.item_id = item.id;
    event.item_style = ItemType::Knife;
    event.position = position;
    state.RecordProjectileEvent(event);
}

void DropWeapon(StateManager& state, Soldier& soldier, const Bullet& bullet)
{
    const auto weapon = soldier.GetActiveWeapon().GetWeaponParameters().kind;
    constexpr std::array<std::optional<ItemType>, 23> ITEMS{
        ItemType::DesertEagles, ItemType::MP5,     ItemType::Ak74,    ItemType::SteyrAUG,
        ItemType::Spas12,       ItemType::Ruger77, ItemType::M79,     ItemType::Barrett,
        ItemType::Minimi,       ItemType::Minigun, ItemType::USSOCOM, ItemType::Knife,
        ItemType::Chainsaw,     ItemType::LAW,     std::nullopt,      std::nullopt,
        ItemType::FlamerKit,    ItemType::M2,      std::nullopt,      std::nullopt,
        std::nullopt,           std::nullopt,      std::nullopt
    };
    const auto item = ITEMS.at(static_cast<unsigned int>(weapon));
    if (!item || !state.HasAuthoritativeProjectiles()) {
        return;
    }
    auto& dropped = state.CreateItem(soldier.skeleton->GetPos(16), soldier.id, *item);
    dropped.polygon_background = soldier.polygon_background;
    dropped.ammo_count = soldier.GetActiveWeapon().GetAmmoCount();
    state.ChangeSoldierPrimaryWeapon(soldier.id, WeaponType::NoWeapon);
    auto event = MakeProjectileEvent(
      ProjectileEventKind::WorldItemSpawnedFromProjectile, bullet, state.GetGameTick());
    event.item_id = dropped.id;
    event.item_style = *item;
    event.position = soldier.skeleton->GetPos(16);
    event.soldier_id = soldier.id;
    state.RecordProjectileEvent(event);
}

void Fragment(StateManager& state, const Bullet& bullet)
{
    if (!state.HasAuthoritativeProjectiles()) {
        return;
    }
    std::mt19937 random{ bullet.seed };
    const auto parameters =
      WeaponParametersFactory::GetParameters(WeaponType::Cluster, bullet.realistic);
    const auto grenade =
      WeaponParametersFactory::GetParameters(WeaponType::FragGrenade, bullet.realistic);
    auto event =
      MakeProjectileEvent(ProjectileEventKind::ProjectileFragmented, bullet, state.GetGameTick());
    for (unsigned int index = 0; index < 5; ++index) {
        const glm::vec2 velocity = bullet.particle.GetVelocity();
        const float spread_x = static_cast<float>(random() % 50U) / 10.0F;
        const float spread_y = static_cast<float>(random() % 25U) / 10.0F;
        BulletParams child{
            .style = BulletType::Cluster,
            .weapon = WeaponType::Cluster,
            .position = bullet.particle.position - velocity,
            .velocity = { velocity.x * 0.75F - 2.5F + spread_x,
                          velocity.y * -0.75F - 2.5F + spread_y },
            .timeout = static_cast<std::int16_t>(parameters.timeout),
            .hit_multiply = grenade.hit_multiply / 2.0F,
            .team = bullet.team,
            .owner_id = bullet.owner_id,
            .push = parameters.push,
            .owner_ping_ticks = bullet.owner_ping_ticks,
            .seed = static_cast<std::uint32_t>(random()),
            .derived = true,
            .realistic = bullet.realistic,
        };
        child = state.PrepareProjectile(child);
        state.EnqueueNewProjectile(child);
        event.children.push_back(child);
    }
    state.RecordProjectileEvent(event);
}

void PropagateFlame(StateManager& state, Bullet& bullet, const Soldier& target, unsigned int part)
{
    bullet.particle.position = target.skeleton->GetPos(part);
    bullet.particle.SetVelocity(target.dead_meat ? glm::vec2{} : target.particle.GetVelocity());
    if (bullet.timeout >= 3 || bullet.ricochet_count >= 2) {
        return;
    }
    const auto parameters =
      WeaponParametersFactory::GetParameters(WeaponType::Flamer, bullet.realistic);
    if (bullet.hit_multiply < parameters.hit_multiply / 3.0F) {
        return;
    }
    bullet.timeout = 31;
    ++bullet.ricochet_count;
    if (state.HasAuthoritativeProjectiles()) {
        state.EnqueueNewProjectile({
          .style = BulletType::Flame,
          .weapon = WeaponType::Flamer,
          .position = bullet.particle.position,
          .velocity = -target.particle.GetVelocity(),
          .timeout = 32,
          .hit_multiply = 2.0F * bullet.hit_multiply / 3.0F,
          .team = bullet.team,
          .owner_id = bullet.owner_id,
          .push = parameters.push,
          .owner_ping_ticks = bullet.owner_ping_ticks,
          .derived = true,
          .realistic = bullet.realistic,
        });
    }
}
} // namespace Soldank::BulletEffects
