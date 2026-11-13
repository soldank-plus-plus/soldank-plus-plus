export module Shared.Core.Types.WeaponType;

export namespace Soldank
{
enum class WeaponType
{
    DesertEagles,
    MP5,
    Ak74,
    SteyrAUG,
    Spas12,
    Ruger77,
    M79,
    Barrett,
    Minimi,
    Minigun,
    USSOCOM,
    Knife,
    Chainsaw,
    LAW,
    FlameBow,
    Bow,
    Flamer,
    M2,
    NoWeapon,
    FragGrenade,
    ClusterGrenade,
    Cluster,
    ThrownKnife,
};

bool IsPrimaryWeaponChoice(WeaponType type)
{
    return type >= WeaponType::DesertEagles && type <= WeaponType::Minigun;
}

bool IsSecondaryWeaponChoice(WeaponType type)
{
    return (type >= WeaponType::USSOCOM && type <= WeaponType::LAW) || type == WeaponType::NoWeapon;
}
} // namespace Soldank
