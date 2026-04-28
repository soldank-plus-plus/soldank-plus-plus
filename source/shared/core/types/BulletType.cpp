export module Shared.Core.Types.BulletType;

export namespace Soldank
{
enum class BulletType : unsigned int
{
    Bullet = 1,
    FragGrenade = 2,
    GaugeBullet = 3,
    M79Grenade = 4,
    Flame = 5,
    Fist = 6,
    Arrow = 7,
    FlameArrow = 8,
    ClusterGrenade = 9,
    Cluster = 10,
    Blade = 11, // used for knife and chainsaw
    LAWMissile = 12,
    ThrownKnife = 13,
    M2Bullet = 14,
};
} // namespace Soldank
