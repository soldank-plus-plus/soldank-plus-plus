module;

#include "spdlog/spdlog.h"
#include <SimpleIni.h>

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>
#include <bit>
#include <array>

export module Shared.Core.Entities.WeaponParametersFactory;

import Shared.Core.Data.IFileReader;
import Shared.Core.Data.FileReader;

import Shared.Core.Config.Config;
import Shared.Core.Entities.WeaponParameters;

import Shared.Core.Types.WeaponType;
import Shared.Core.Types.BulletType;

namespace Soldank::WeaponParametersFactory
{
const std::uint16_t SECOND = 60;
const std::uint16_t BULLET_TIMEOUT = SECOND * 7;
const std::uint16_t GRENADE_TIMEOUT = SECOND * 3;
const std::uint16_t M2BULLET_TIMEOUT = SECOND;
const std::uint16_t FLAMER_TIMEOUT = 32;
const std::uint16_t MELEE_TIMEOUT = 1;

// TODO: Move to Bullet classes
BulletType ConvertToBulletStyleEnum(uint8_t bullet_type_id)
{
    switch (bullet_type_id) {
        case 1:
            return BulletType::Bullet;
        case 2:
            return BulletType::FragGrenade;
        case 3:
            return BulletType::GaugeBullet;
        case 4:
            return BulletType::M79Grenade;
        case 5:
            return BulletType::Flame;
        case 6:
            return BulletType::Fist;
        case 7:
            return BulletType::Arrow;
        case 8:
            return BulletType::FlameArrow;
        case 9:
            return BulletType::ClusterGrenade;
        case 10:
            return BulletType::Cluster;
        case 11:
            return BulletType::Blade;
        case 12:
            return BulletType::LAWMissile;
        case 13:
            return BulletType::ThrownKnife;
        case 14:
            return BulletType::M2Bullet;
        default:
            throw std::invalid_argument("Unknown weapon bullet style");
    }
}
} // namespace Soldank::WeaponParametersFactory

export namespace Soldank::WeaponParametersFactory
{
std::string GetNameForWeaponType(WeaponType weapon_type);
std::string GetININameForWeaponType(WeaponType weapon_type);
bool GetClipReload(WeaponType weapon_type);

WeaponParameters LoadFromINI(const CSimpleIniA& ini_config, WeaponType weapon_type)
{
    WeaponParameters weapon_parameters;

    weapon_parameters.kind = weapon_type;
    weapon_parameters.name = GetNameForWeaponType(weapon_type);
    weapon_parameters.ini_name = GetININameForWeaponType(weapon_type);
    std::string ini_name = weapon_parameters.ini_name;

    weapon_parameters.hit_multiply = (float)ini_config.GetDoubleValue(ini_name.c_str(), "Damage");
    weapon_parameters.fire_interval = ini_config.GetLongValue(ini_name.c_str(), "FireInterval");
    weapon_parameters.ammo = ini_config.GetLongValue(ini_name.c_str(), "Ammo");
    weapon_parameters.reload_time = ini_config.GetLongValue(ini_name.c_str(), "ReloadTime");
    weapon_parameters.speed = (float)ini_config.GetDoubleValue(ini_name.c_str(), "Speed");
    weapon_parameters.start_up_time = ini_config.GetLongValue(ini_name.c_str(), "StartUpTime");
    weapon_parameters.bink = (int16_t)ini_config.GetLongValue(ini_name.c_str(), "Bink");
    weapon_parameters.movement_acc =
      (float)ini_config.GetDoubleValue(ini_name.c_str(), "MovementAcc");
    weapon_parameters.bullet_spread =
      (float)ini_config.GetDoubleValue(ini_name.c_str(), "BulletSpread");
    weapon_parameters.recoil = ini_config.GetLongValue(ini_name.c_str(), "Recoil");
    weapon_parameters.push = (float)ini_config.GetDoubleValue(ini_name.c_str(), "Push");
    weapon_parameters.inherited_velocity =
      (float)ini_config.GetDoubleValue(ini_name.c_str(), "InheritedVelocity");
    weapon_parameters.modifier_head =
      (float)ini_config.GetDoubleValue(ini_name.c_str(), "ModifierHead");
    weapon_parameters.modifier_chest =
      (float)ini_config.GetDoubleValue(ini_name.c_str(), "ModifierChest");
    weapon_parameters.modifier_legs =
      (float)ini_config.GetDoubleValue(ini_name.c_str(), "ModifierLegs");
    weapon_parameters.no_collision =
      static_cast<std::uint8_t>(ini_config.GetLongValue(ini_name.c_str(), "NoCollision"));

    switch (weapon_type) {
        case WeaponType::ClusterGrenade:
            weapon_parameters.bullet_style = BulletType::ClusterGrenade;
            break;
        case WeaponType::Cluster:
            weapon_parameters.bullet_style = BulletType::Cluster;
            break;
        case WeaponType::ThrownKnife:
            weapon_parameters.bullet_style = BulletType::ThrownKnife;
            break;
        default:
            weapon_parameters.bullet_style =
              ConvertToBulletStyleEnum(ini_config.GetLongValue(ini_name.c_str(), "BulletStyle"));
    }

    weapon_parameters.clip_reload = GetClipReload(weapon_type);

    switch (weapon_parameters.bullet_style) {
        case BulletType::FragGrenade:
        case BulletType::ClusterGrenade:
            weapon_parameters.timeout = GRENADE_TIMEOUT;
            break;
        case BulletType::Flame:
            weapon_parameters.timeout = FLAMER_TIMEOUT;
            break;
        case BulletType::Fist:
        case BulletType::Blade:
            weapon_parameters.timeout = MELEE_TIMEOUT;
            break;
        case BulletType::M2Bullet:
            weapon_parameters.timeout = M2BULLET_TIMEOUT;
            break;
        default:
            weapon_parameters.timeout = BULLET_TIMEOUT;
    }

    return weapon_parameters;
}

WeaponParameters LoadFromINIFile(const std::string& ini_file_path,
                                 WeaponType weapon_type,
                                 const IFileReader& file_reader = FileReader())
{
    spdlog::debug("ini_file_path: {}", ini_file_path);
    auto file_data = file_reader.Read(ini_file_path);
    if (!file_data.has_value()) {
        spdlog::critical("Animation file not found {}", ini_file_path);
        std::string message = "Could not open file: " + ini_file_path;
        throw std::runtime_error(message.c_str());
    }

    CSimpleIniA ini_config;
    SI_Error rc = ini_config.LoadData(*file_data);
    if (rc < 0) {
        spdlog::critical("Error: INI File could not be loaded: {}", ini_file_path);
    };
    auto parameters = LoadFromINI(ini_config, weapon_type);
    parameters.realistic = ini_file_path == Config::WEAPONS_REALISTIC_INI_FILE_PATH;
    return parameters;
}

const WeaponParameters& GetParameters(WeaponType weapon_type, bool realistic)
{
    // clang-format off
    static std::unordered_map<WeaponType, const WeaponParameters> weapon_parameters_map{
        { WeaponType::DesertEagles, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::DesertEagles) },
        { WeaponType::MP5, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::MP5) },
        { WeaponType::Ak74, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Ak74) },
        { WeaponType::SteyrAUG, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::SteyrAUG) },
        { WeaponType::Spas12, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Spas12) },
        { WeaponType::Ruger77, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Ruger77) },
        { WeaponType::M79, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::M79) },
        { WeaponType::Barrett, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Barrett) },
        { WeaponType::Minimi, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Minimi) },
        { WeaponType::Minigun, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Minigun) },
        { WeaponType::USSOCOM, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::USSOCOM) },
        { WeaponType::Knife, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Knife) },
        { WeaponType::Chainsaw, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Chainsaw) },
        { WeaponType::LAW, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::LAW) },
        { WeaponType::FlameBow, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::FlameBow) },
        { WeaponType::Bow, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Bow) },
        { WeaponType::Flamer, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Flamer) },
        { WeaponType::M2, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::M2) },
        { WeaponType::NoWeapon, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::NoWeapon) },
        { WeaponType::FragGrenade, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::FragGrenade) },
        { WeaponType::ClusterGrenade, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::ClusterGrenade) },
        { WeaponType::Cluster, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::Cluster) },
        { WeaponType::ThrownKnife, LoadFromINIFile(Config::WEAPONS_INI_FILE_PATH, WeaponType::ThrownKnife) },
    };

    static std::unordered_map<WeaponType, const WeaponParameters> weapon_parameters_realistic_map{
        { WeaponType::DesertEagles, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::DesertEagles) },
        { WeaponType::MP5, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::MP5) },
        { WeaponType::Ak74, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Ak74) },
        { WeaponType::SteyrAUG, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::SteyrAUG) },
        { WeaponType::Spas12, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Spas12) },
        { WeaponType::Ruger77, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Ruger77) },
        { WeaponType::M79, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::M79) },
        { WeaponType::Barrett, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Barrett) },
        { WeaponType::Minimi, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Minimi) },
        { WeaponType::Minigun, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Minigun) },
        { WeaponType::USSOCOM, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::USSOCOM) },
        { WeaponType::Knife, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Knife) },
        { WeaponType::Chainsaw, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Chainsaw) },
        { WeaponType::LAW, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::LAW) },
        { WeaponType::FlameBow, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::FlameBow) },
        { WeaponType::Bow, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Bow) },
        { WeaponType::Flamer, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Flamer) },
        { WeaponType::M2, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::M2) },
        { WeaponType::NoWeapon, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::NoWeapon) },
        { WeaponType::FragGrenade, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::FragGrenade) },
        { WeaponType::ClusterGrenade, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::ClusterGrenade) },
        { WeaponType::Cluster, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::Cluster) },
        { WeaponType::ThrownKnife, LoadFromINIFile(Config::WEAPONS_REALISTIC_INI_FILE_PATH, WeaponType::ThrownKnife) },
    };
    // clang-format on

    if (realistic) {
        return weapon_parameters_realistic_map.at(weapon_type);
    }
    return weapon_parameters_map.at(weapon_type);
}

std::uint64_t GetConfigurationHash(bool realistic)
{
    const auto compute = [](bool mode) {
        std::uint64_t hash = 14695981039346656037ULL;
        const auto append = [&](std::uint32_t value) {
            for (unsigned int byte = 0; byte < 4; ++byte) {
                hash ^= (value >> (byte * 8U)) & 0xffU;
                hash *= 1099511628211ULL;
            }
        };
        for (int index = 0; index <= static_cast<int>(WeaponType::ThrownKnife); ++index) {
            const auto& parameters = GetParameters(static_cast<WeaponType>(index), mode);
            for (const std::uint32_t value : std::array{
                   static_cast<std::uint32_t>(index),
                   static_cast<std::uint32_t>(parameters.ammo),
                   static_cast<std::uint32_t>(static_cast<std::uint16_t>(parameters.bink)),
                   static_cast<std::uint32_t>(parameters.recoil),
                   static_cast<std::uint32_t>(parameters.fire_interval),
                   static_cast<std::uint32_t>(parameters.bullet_style),
                   static_cast<std::uint32_t>(parameters.no_collision),
                   static_cast<std::uint32_t>(parameters.start_up_time),
                   static_cast<std::uint32_t>(parameters.reload_time),
                   static_cast<std::uint32_t>(parameters.clip_reload),
                   static_cast<std::uint32_t>(parameters.timeout) }) {
                append(value);
            }
            for (const float value : { parameters.movement_acc,
                                       parameters.hit_multiply,
                                       parameters.bullet_spread,
                                       parameters.modifier_legs,
                                       parameters.modifier_chest,
                                       parameters.modifier_head,
                                       parameters.inherited_velocity,
                                       parameters.push,
                                       parameters.speed }) {
                append(std::bit_cast<std::uint32_t>(value));
            }
        }
        return hash == 0 ? std::uint64_t{ 1 } : hash;
    };
    static const auto NORMAL_HASH = compute(false);
    static const auto REALISTIC_HASH = compute(true);
    return realistic ? REALISTIC_HASH : NORMAL_HASH;
}

std::string GetNameForWeaponType(WeaponType weapon_type)
{
    switch (weapon_type) {
        case WeaponType::DesertEagles:
            return "Desert Eagles";
        case WeaponType::MP5:
            return "HK MP5";
        case WeaponType::Ak74:
            return "Ak-74";
        case WeaponType::SteyrAUG:
            return "Steyr AUG";
        case WeaponType::Spas12:
            return "Spas-12";
        case WeaponType::Ruger77:
            return "Ruger 77";
        case WeaponType::M79:
            return "M79";
        case WeaponType::Barrett:
            return "Barrett M82A1";
        case WeaponType::Minimi:
            return "FN Minimi";
        case WeaponType::Minigun:
            return "XM214 Minigun";
        case WeaponType::USSOCOM:
            return "USSOCOM";
        case WeaponType::Knife:
            return "Combat Knife";
        case WeaponType::Chainsaw:
            return "Chainsaw";
        case WeaponType::LAW:
            return "LAW";
        case WeaponType::FlameBow:
            return "Flame Bow";
        case WeaponType::Bow:
            return "Bow";
        case WeaponType::Flamer:
            return "Flamer";
        case WeaponType::M2:
            return "M2 MG";
        case WeaponType::NoWeapon:
            return "Hands";
        case WeaponType::FragGrenade:
        case WeaponType::ClusterGrenade:
        case WeaponType::Cluster:
            return "Frag Grenade";
        case WeaponType::ThrownKnife:
            return "Combat Knife";
    }
}

std::string GetININameForWeaponType(WeaponType weapon_type)
{
    switch (weapon_type) {
        case WeaponType::DesertEagles:
            return "Desert Eagles";
        case WeaponType::MP5:
            return "HK MP5";
        case WeaponType::Ak74:
            return "Ak-74";
        case WeaponType::SteyrAUG:
            return "Steyr AUG";
        case WeaponType::Spas12:
            return "Spas-12";
        case WeaponType::Ruger77:
            return "Ruger 77";
        case WeaponType::M79:
            return "M79";
        case WeaponType::Barrett:
            return "Barret M82A1";
        case WeaponType::Minimi:
            return "FN Minimi";
        case WeaponType::Minigun:
            return "XM214 Minigun";
        case WeaponType::USSOCOM:
            return "USSOCOM";
        case WeaponType::Knife:
            return "Combat Knife";
        case WeaponType::Chainsaw:
            return "Chainsaw";
        case WeaponType::LAW:
            return "M72 LAW";
        case WeaponType::FlameBow:
            return "Flamed Arrows";
        case WeaponType::Bow:
            return "Rambo Bow";
        case WeaponType::Flamer:
            return "Flamer";
        case WeaponType::M2:
            return "Stationary Gun";
        case WeaponType::NoWeapon:
            return "Punch";
        case WeaponType::FragGrenade:
        case WeaponType::ClusterGrenade:
        case WeaponType::Cluster:
            return "Grenade";
        case WeaponType::ThrownKnife:
            return "Combat Knife";
    }
}

bool GetClipReload(WeaponType weapon_type)
{
    switch (weapon_type) {
        case WeaponType::DesertEagles:
        case WeaponType::MP5:
        case WeaponType::Ak74:
        case WeaponType::SteyrAUG:
        case WeaponType::M79:
        case WeaponType::Barrett:
        case WeaponType::Minimi:
        case WeaponType::USSOCOM:
        case WeaponType::LAW:
            return true;
        case WeaponType::Spas12:
        case WeaponType::Ruger77:
        case WeaponType::Minigun:
        case WeaponType::Knife:
        case WeaponType::Chainsaw:
        case WeaponType::FlameBow:
        case WeaponType::Bow:
        case WeaponType::Flamer:
        case WeaponType::M2:
        case WeaponType::NoWeapon:
        case WeaponType::FragGrenade:
        case WeaponType::ClusterGrenade:
        case WeaponType::Cluster:
        case WeaponType::ThrownKnife:
            return false;
    }
}
} // namespace Soldank::WeaponParametersFactory
