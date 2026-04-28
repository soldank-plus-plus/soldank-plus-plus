module;

#include <cstdint>

export module Shared.Core.Types.ItemType;

export namespace Soldank
{
enum class ItemType : std::uint8_t
{
    AlphaFlag = 1,
    BravoFlag,
    PointmatchFlag,
    USSOCOM,
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
    Bow,
    MedicalKit,
    GrenadeKit,
    FlamerKit,
    PredatorKit,
    VestKit,
    BerserkKit,
    ClusterKit,
    Parachute,
    Knife,
    Chainsaw,
    LAW,
    M2,
};

bool IsItemTypeFlag(ItemType item_type)
{
    switch (item_type) {
        case ItemType::AlphaFlag:
        case ItemType::BravoFlag:
        case ItemType::PointmatchFlag:
            return true;
        case ItemType::USSOCOM:
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
        case ItemType::Bow:
        case ItemType::MedicalKit:
        case ItemType::GrenadeKit:
        case ItemType::FlamerKit:
        case ItemType::PredatorKit:
        case ItemType::VestKit:
        case ItemType::BerserkKit:
        case ItemType::ClusterKit:
        case ItemType::Parachute:
        case ItemType::Knife:
        case ItemType::Chainsaw:
        case ItemType::LAW:
        case ItemType::M2:
            return false;
    }
}

bool IsItemTypeWeapon(ItemType item_type)
{
    switch (item_type) {
        case ItemType::AlphaFlag:
        case ItemType::BravoFlag:
        case ItemType::PointmatchFlag:
            return false;
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
        case ItemType::Bow:
            return true;
        case ItemType::MedicalKit:
        case ItemType::GrenadeKit:
        case ItemType::FlamerKit:
        case ItemType::PredatorKit:
        case ItemType::VestKit:
        case ItemType::BerserkKit:
        case ItemType::ClusterKit:
        case ItemType::Parachute:
        case ItemType::M2:
            return false;
    }
}

bool IsItemTypeKit(ItemType item_type)
{
    switch (item_type) {
        case ItemType::AlphaFlag:
        case ItemType::BravoFlag:
        case ItemType::PointmatchFlag:
        case ItemType::USSOCOM:
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
        case ItemType::Bow:
            return false;
        case ItemType::MedicalKit:
        case ItemType::GrenadeKit:
        case ItemType::FlamerKit:
        case ItemType::PredatorKit:
        case ItemType::VestKit:
        case ItemType::BerserkKit:
        case ItemType::ClusterKit:
            return true;
        case ItemType::Parachute:
        case ItemType::Knife:
        case ItemType::Chainsaw:
        case ItemType::LAW:
        case ItemType::M2:
            return false;
    }
}
} // namespace Soldank
