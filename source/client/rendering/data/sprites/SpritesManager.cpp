module;

#include "core/math/Glm.hpp"

#include "rendering/data/sprites/SpriteTypes.hpp"

#include <memory>
#include <utility>
#include <variant>
#include <vector>
#include <optional>
#include <algorithm>
#include <string>
#include <filesystem>
#include <optional>
#include <variant>
#include <unordered_map>

#include "core/utility/Expected.hpp"

export module SpritesManager;

import Texture;
import SoldierPartData;

import Shared.Core.Entities.Bullet;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.WeaponType;

import Extern.Spdlog;

export namespace Soldank::Sprites
{
class SpriteManager
{
private:
    using TSoldierPartSpriteData = std::unique_ptr<SoldierPartData>;
    using TSoldierPartSpriteKey = std::variant<SoldierPartSpriteType,
                                               SoldierPartPrimaryWeaponSpriteType,
                                               SoldierPartSecondaryWeaponSpriteType,
                                               SoldierPartTertiaryWeaponSpriteType>;
    using TSoldierPartListItem = std::pair<TSoldierPartSpriteKey, TSoldierPartSpriteData>;
    using TSoldierPartList = std::vector<TSoldierPartListItem>;

    using TSpriteKey = std::variant<SoldierPartSpriteType, WeaponSpriteType, ObjectSpriteType>;

public:
    SpriteManager()
    {
        Spdlog::info("Init Sprites");
        std::vector<std::pair<TSpriteKey, std::string>> all_sprite_file_paths{
            { SoldierPartSpriteType::Stopa, "gostek-gfx/stopa.png" },
            { SoldierPartSpriteType::Stopa2, "gostek-gfx/stopa2.png" },
            { SoldierPartSpriteType::Noga, "gostek-gfx/noga.png" },
            { SoldierPartSpriteType::Noga2, "gostek-gfx/noga2.png" },
            { SoldierPartSpriteType::Udo, "gostek-gfx/udo.png" },
            { SoldierPartSpriteType::Udo2, "gostek-gfx/udo2.png" },
            { SoldierPartSpriteType::Biodro, "gostek-gfx/biodro.png" },
            { SoldierPartSpriteType::Biodro2, "gostek-gfx/biodro2.png" },
            { SoldierPartSpriteType::Klata, "gostek-gfx/klata.png" },
            { SoldierPartSpriteType::Klata2, "gostek-gfx/klata2.png" },
            { SoldierPartSpriteType::Morda, "gostek-gfx/morda.png" },
            { SoldierPartSpriteType::Morda2, "gostek-gfx/morda2.png" },
            { SoldierPartSpriteType::Ramie, "gostek-gfx/ramie.png" },
            { SoldierPartSpriteType::Ramie2, "gostek-gfx/ramie2.png" },
            { SoldierPartSpriteType::Reka, "gostek-gfx/reka.png" },
            { SoldierPartSpriteType::Reka2, "gostek-gfx/reka2.png" },
            { SoldierPartSpriteType::Dlon, "gostek-gfx/dlon.png" },
            { SoldierPartSpriteType::Dlon2, "gostek-gfx/dlon2.png" },
            { SoldierPartSpriteType::Lancuch, "gostek-gfx/lancuch.png" },
            { SoldierPartSpriteType::Helm, "gostek-gfx/helm.png" },
            { SoldierPartSpriteType::Helm2, "gostek-gfx/helm2.png" },
            { SoldierPartSpriteType::Badge, "gostek-gfx/badge.png" },
            { SoldierPartSpriteType::Badge2, "gostek-gfx/badge2.png" },
            { SoldierPartSpriteType::Cygaro, "gostek-gfx/cygaro.png" },
            { SoldierPartSpriteType::Cygaro2, "gostek-gfx/cygaro2.png" },
            { SoldierPartSpriteType::Metal, "gostek-gfx/metal.png" },
            { SoldierPartSpriteType::Metal2, "gostek-gfx/metal2.png" },
            { SoldierPartSpriteType::Zloto, "gostek-gfx/zloto.png" },
            { SoldierPartSpriteType::Zloto2, "gostek-gfx/zloto2.png" },
            { SoldierPartSpriteType::Zlotylancuch, "gostek-gfx/zlotylancuch.png" },
            { SoldierPartSpriteType::Kap, "gostek-gfx/kap.png" },
            { SoldierPartSpriteType::Kap2, "gostek-gfx/kap2.png" },
            { SoldierPartSpriteType::Dred, "gostek-gfx/dred.png" },
            { SoldierPartSpriteType::Hair1, "gostek-gfx/hair1.png" },
            { SoldierPartSpriteType::Hair12, "gostek-gfx/hair12.png" },
            { SoldierPartSpriteType::Hair2, "gostek-gfx/hair2.png" },
            { SoldierPartSpriteType::Hair22, "gostek-gfx/hair22.png" },
            { SoldierPartSpriteType::Hair3, "gostek-gfx/hair3.png" },
            { SoldierPartSpriteType::Hair32, "gostek-gfx/hair32.png" },
            { SoldierPartSpriteType::Hair4, "gostek-gfx/hair4.png" },
            { SoldierPartSpriteType::Hair42, "gostek-gfx/hair42.png" },
            { SoldierPartSpriteType::Kamizelka, "gostek-gfx/kamizelka.png" },
            { SoldierPartSpriteType::Kamizelka2, "gostek-gfx/kamizelka2.png" },
            { SoldierPartSpriteType::RannyKlata, "gostek-gfx/ranny/klata.png" },
            { SoldierPartSpriteType::RannyKlata2, "gostek-gfx/ranny/klata2.png" },
            { SoldierPartSpriteType::RannyMorda, "gostek-gfx/ranny/morda.png" },
            { SoldierPartSpriteType::RannyMorda2, "gostek-gfx/ranny/morda2.png" },
            { SoldierPartSpriteType::RannyNoga, "gostek-gfx/ranny/noga.png" },
            { SoldierPartSpriteType::RannyNoga2, "gostek-gfx/ranny/noga2.png" },
            { SoldierPartSpriteType::RannyRamie, "gostek-gfx/ranny/ramie.png" },
            { SoldierPartSpriteType::RannyRamie2, "gostek-gfx/ranny/ramie2.png" },
            { SoldierPartSpriteType::RannyReka, "gostek-gfx/ranny/reka.png" },
            { SoldierPartSpriteType::RannyReka2, "gostek-gfx/ranny/reka2.png" },
            { SoldierPartSpriteType::RannyUdo, "gostek-gfx/ranny/udo.png" },
            { SoldierPartSpriteType::RannyUdo2, "gostek-gfx/ranny/udo2.png" },
            { SoldierPartSpriteType::RannyBiodro, "gostek-gfx/ranny/biodro.png" },
            { SoldierPartSpriteType::RannyBiodro2, "gostek-gfx/ranny/biodro2.png" },
            { SoldierPartSpriteType::Lecistopa, "gostek-gfx/lecistopa.png" },
            { SoldierPartSpriteType::Lecistopa2, "gostek-gfx/lecistopa2.png" },

            // Soldier Team2 list. Keep it next to the non-team2 list.

            { SoldierPartSpriteType::Team2Stopa, "gostek-gfx/team2/stopa.png" },
            { SoldierPartSpriteType::Team2Stopa2, "gostek-gfx/team2/stopa2.png" },
            { SoldierPartSpriteType::Team2Noga, "gostek-gfx/team2/noga.png" },
            { SoldierPartSpriteType::Team2Noga2, "gostek-gfx/team2/noga2.png" },
            { SoldierPartSpriteType::Team2Udo, "gostek-gfx/team2/udo.png" },
            { SoldierPartSpriteType::Team2Udo2, "gostek-gfx/team2/udo2.png" },
            { SoldierPartSpriteType::Team2Biodro, "gostek-gfx/team2/biodro.png" },
            { SoldierPartSpriteType::Team2Biodro2, "gostek-gfx/team2/biodro2.png" },
            { SoldierPartSpriteType::Team2Klata, "gostek-gfx/team2/klata.png" },
            { SoldierPartSpriteType::Team2Klata2, "gostek-gfx/team2/klata2.png" },
            { SoldierPartSpriteType::Team2Morda, "gostek-gfx/team2/morda.png" },
            { SoldierPartSpriteType::Team2Morda2, "gostek-gfx/team2/morda2.png" },
            { SoldierPartSpriteType::Team2Ramie, "gostek-gfx/team2/ramie.png" },
            { SoldierPartSpriteType::Team2Ramie2, "gostek-gfx/team2/ramie2.png" },
            { SoldierPartSpriteType::Team2Reka, "gostek-gfx/team2/reka.png" },
            { SoldierPartSpriteType::Team2Reka2, "gostek-gfx/team2/reka2.png" },
            { SoldierPartSpriteType::Team2Dlon, "gostek-gfx/team2/dlon.png" },
            { SoldierPartSpriteType::Team2Dlon2, "gostek-gfx/team2/dlon2.png" },
            { SoldierPartSpriteType::Team2Lancuch, "gostek-gfx/team2/lancuch.png" },
            { SoldierPartSpriteType::Team2Helm, "gostek-gfx/team2/helm.png" },
            { SoldierPartSpriteType::Team2Helm2, "gostek-gfx/team2/helm2.png" },
            { SoldierPartSpriteType::Team2Badge, "gostek-gfx/team2/badge.png" },
            { SoldierPartSpriteType::Team2Badge2, "gostek-gfx/team2/badge2.png" },
            { SoldierPartSpriteType::Team2Cygaro, "gostek-gfx/team2/cygaro.png" },
            { SoldierPartSpriteType::Team2Cygaro2, "gostek-gfx/team2/cygaro2.png" },
            { SoldierPartSpriteType::Team2Metal, "gostek-gfx/team2/metal.png" },
            { SoldierPartSpriteType::Team2Metal2, "gostek-gfx/team2/metal2.png" },
            { SoldierPartSpriteType::Team2Zloto, "gostek-gfx/team2/zloto.png" },
            { SoldierPartSpriteType::Team2Zloto2, "gostek-gfx/team2/zloto2.png" },
            { SoldierPartSpriteType::Team2Zlotylancuch, "gostek-gfx/team2/zlotylancuch.png" },
            { SoldierPartSpriteType::Team2Kap, "gostek-gfx/team2/kap.png" },
            { SoldierPartSpriteType::Team2Kap2, "gostek-gfx/team2/kap2.png" },
            { SoldierPartSpriteType::Team2Dred, "gostek-gfx/team2/dred.png" },
            { SoldierPartSpriteType::Team2Hair1, "gostek-gfx/team2/hair1.png" },
            { SoldierPartSpriteType::Team2Hair12, "gostek-gfx/team2/hair12.png" },
            { SoldierPartSpriteType::Team2Hair2, "gostek-gfx/team2/hair2.png" },
            { SoldierPartSpriteType::Team2Hair22, "gostek-gfx/team2/hair22.png" },
            { SoldierPartSpriteType::Team2Hair3, "gostek-gfx/team2/hair3.png" },
            { SoldierPartSpriteType::Team2Hair32, "gostek-gfx/team2/hair32.png" },
            { SoldierPartSpriteType::Team2Hair4, "gostek-gfx/team2/hair4.png" },
            { SoldierPartSpriteType::Team2Hair42, "gostek-gfx/team2/hair42.png" },
            { SoldierPartSpriteType::Team2Kamizelka, "gostek-gfx/team2/kamizelka.png" },
            { SoldierPartSpriteType::Team2Kamizelka2, "gostek-gfx/team2/kamizelka2.png" },
            { SoldierPartSpriteType::Team2RannyKlata, "gostek-gfx/team2/ranny/klata.png" },
            { SoldierPartSpriteType::Team2RannyKlata2, "gostek-gfx/team2/ranny/klata2.png" },
            { SoldierPartSpriteType::Team2RannyMorda, "gostek-gfx/team2/ranny/morda.png" },
            { SoldierPartSpriteType::Team2RannyMorda2, "gostek-gfx/team2/ranny/morda2.png" },
            { SoldierPartSpriteType::Team2RannyNoga, "gostek-gfx/team2/ranny/noga.png" },
            { SoldierPartSpriteType::Team2RannyNoga2, "gostek-gfx/team2/ranny/noga2.png" },
            { SoldierPartSpriteType::Team2RannyRamie, "gostek-gfx/team2/ranny/ramie.png" },
            { SoldierPartSpriteType::Team2RannyRamie2, "gostek-gfx/team2/ranny/ramie2.png" },
            { SoldierPartSpriteType::Team2RannyReka, "gostek-gfx/team2/ranny/reka.png" },
            { SoldierPartSpriteType::Team2RannyReka2, "gostek-gfx/team2/ranny/reka2.png" },
            { SoldierPartSpriteType::Team2RannyUdo, "gostek-gfx/team2/ranny/udo.png" },
            { SoldierPartSpriteType::Team2RannyUdo2, "gostek-gfx/team2/ranny/udo2.png" },
            { SoldierPartSpriteType::Team2RannyBiodro, "gostek-gfx/team2/ranny/biodro.png" },
            { SoldierPartSpriteType::Team2RannyBiodro2, "gostek-gfx/team2/ranny/biodro2.png" },
            { SoldierPartSpriteType::Team2Lecistopa, "gostek-gfx/team2/lecistopa.png" },
            { SoldierPartSpriteType::Team2Lecistopa2, "gostek-gfx/team2/lecistopa2.png" },
            { WeaponSpriteType::Shell, "weapons-gfx/shell.png" },
            { WeaponSpriteType::Bullet, "weapons-gfx/bullet.png" },
            { WeaponSpriteType::Smudge, "weapons-gfx/smudge.png" },
            { WeaponSpriteType::Missile, "weapons-gfx/missile.png" },
            { WeaponSpriteType::Cluster, "weapons-gfx/cluster.png" },
            { WeaponSpriteType::ClusterGrenade, "weapons-gfx/cluster-grenade.png" },
            { WeaponSpriteType::FragGrenade, "weapons-gfx/frag-grenade.png" },
            { WeaponSpriteType::Ak74, "weapons-gfx/ak74.png" },
            { WeaponSpriteType::Ak742, "weapons-gfx/ak74-2.png" },
            { WeaponSpriteType::Ak74Clip, "weapons-gfx/ak74-clip.png" },
            { WeaponSpriteType::Ak74Clip2, "weapons-gfx/ak74-clip2.png" },
            { WeaponSpriteType::Ak74Shell, "weapons-gfx/ak74-shell.png" },
            { WeaponSpriteType::Ak74Bullet, "weapons-gfx/ak74-bullet.png" },
            { WeaponSpriteType::Ak74Fire, "weapons-gfx/ak74-fire.png" },
            { WeaponSpriteType::Minimi, "weapons-gfx/m249.png" },
            { WeaponSpriteType::Minimi2, "weapons-gfx/m249-2.png" },
            { WeaponSpriteType::MinimiClip, "weapons-gfx/m249-clip.png" },
            { WeaponSpriteType::MinimiClip2, "weapons-gfx/m249-clip2.png" },
            { WeaponSpriteType::MinimiShell, "weapons-gfx/m249-shell.png" },
            { WeaponSpriteType::MinimiBullet, "weapons-gfx/m249-bullet.png" },
            { WeaponSpriteType::MinimiFire, "weapons-gfx/m249-fire.png" },
            { WeaponSpriteType::Ruger, "weapons-gfx/ruger77.png" },
            { WeaponSpriteType::Ruger2, "weapons-gfx/ruger77-2.png" },
            { WeaponSpriteType::RugerShell, "weapons-gfx/ruger77-shell.png" },
            { WeaponSpriteType::RugerBullet, "weapons-gfx/ruger77-bullet.png" },
            { WeaponSpriteType::RugerFire, "weapons-gfx/ruger77-fire.png" },
            { WeaponSpriteType::Mp5, "weapons-gfx/mp5.png" },
            { WeaponSpriteType::Mp52, "weapons-gfx/mp5-2.png" },
            { WeaponSpriteType::Mp5Clip, "weapons-gfx/mp5-clip.png" },
            { WeaponSpriteType::Mp5Clip2, "weapons-gfx/mp5-clip2.png" },
            { WeaponSpriteType::Mp5Shell, "weapons-gfx/mp5-shell.png" },
            { WeaponSpriteType::Mp5Bullet, "weapons-gfx/mp5-bullet.png" },
            { WeaponSpriteType::Mp5Fire, "weapons-gfx/mp5-fire.png" },
            { WeaponSpriteType::Spas, "weapons-gfx/spas12.png" },
            { WeaponSpriteType::Spas2, "weapons-gfx/spas12-2.png" },
            { WeaponSpriteType::SpasShell, "weapons-gfx/spas12-shell.png" },
            { WeaponSpriteType::SpasBullet, "weapons-gfx/spas12-bullet.png" },
            { WeaponSpriteType::SpasFire, "weapons-gfx/spas12-fire.png" },
            { WeaponSpriteType::M79, "weapons-gfx/m79.png" },
            { WeaponSpriteType::M792, "weapons-gfx/m79-2.png" },
            { WeaponSpriteType::M79Clip, "weapons-gfx/m79-clip.png" },
            { WeaponSpriteType::M79Clip2, "weapons-gfx/m79-clip2.png" },
            { WeaponSpriteType::M79Shell, "weapons-gfx/m79-shell.png" },
            { WeaponSpriteType::M79Bullet, "weapons-gfx/m79-bullet.png" },
            { WeaponSpriteType::M79Fire, "weapons-gfx/m79-fire.png" },
            { WeaponSpriteType::Deagles, "weapons-gfx/deserteagle.png" },
            { WeaponSpriteType::Deagles2, "weapons-gfx/deserteagle-2.png" },
            { WeaponSpriteType::NDeagles, "weapons-gfx/n-deserteagle.png" },
            { WeaponSpriteType::NDeagles2, "weapons-gfx/n-deserteagle-2.png" },
            { WeaponSpriteType::DeaglesClip, "weapons-gfx/deserteagle-clip.png" },
            { WeaponSpriteType::DeaglesClip2, "weapons-gfx/deserteagle-clip2.png" },
            { WeaponSpriteType::DeaglesShell, "weapons-gfx/eagles-shell.png" },
            { WeaponSpriteType::DeaglesBullet, "weapons-gfx/eagles-bullet.png" },
            { WeaponSpriteType::DeaglesFire, "weapons-gfx/eagles-fire.png" },
            { WeaponSpriteType::Steyr, "weapons-gfx/steyraug.png" },
            { WeaponSpriteType::Steyr2, "weapons-gfx/steyraug-2.png" },
            { WeaponSpriteType::SteyrClip, "weapons-gfx/steyraug-clip.png" },
            { WeaponSpriteType::SteyrClip2, "weapons-gfx/steyraug-clip2.png" },
            { WeaponSpriteType::SteyrShell, "weapons-gfx/steyraug-shell.png" },
            { WeaponSpriteType::SteyrBullet, "weapons-gfx/steyraug-bullet.png" },
            { WeaponSpriteType::SteyrFire, "weapons-gfx/steyraug-fire.png" },
            { WeaponSpriteType::Barrett, "weapons-gfx/barretm82.png" },
            { WeaponSpriteType::Barrett2, "weapons-gfx/barretm82-2.png" },
            { WeaponSpriteType::BarrettClip, "weapons-gfx/barretm82-clip.png" },
            { WeaponSpriteType::BarrettClip2, "weapons-gfx/barretm82-clip2.png" },
            { WeaponSpriteType::BarrettShell, "weapons-gfx/barretm82-shell.png" },
            { WeaponSpriteType::BarrettBullet, "weapons-gfx/barretm82-bullet.png" },
            { WeaponSpriteType::BarrettFire, "weapons-gfx/barret-fire.png" },
            { WeaponSpriteType::Minigun, "weapons-gfx/minigun.png" },
            { WeaponSpriteType::Minigun2, "weapons-gfx/minigun-2.png" },
            { WeaponSpriteType::MinigunClip, "weapons-gfx/minigun-clip.png" },
            { WeaponSpriteType::MinigunShell, "weapons-gfx/minigun-shell.png" },
            { WeaponSpriteType::MinigunBullet, "weapons-gfx/minigun-bullet.png" },
            { WeaponSpriteType::MinigunFire, "weapons-gfx/minigun-fire.png" },
            { WeaponSpriteType::Socom, "weapons-gfx/colt1911.png" },
            { WeaponSpriteType::Socom2, "weapons-gfx/colt1911-2.png" },
            { WeaponSpriteType::NSocom, "weapons-gfx/n-colt1911.png" },
            { WeaponSpriteType::NSocom2, "weapons-gfx/n-colt1911-2.png" },
            { WeaponSpriteType::SocomClip, "weapons-gfx/colt1911-clip.png" },
            { WeaponSpriteType::SocomClip2, "weapons-gfx/colt1911-clip2.png" },
            { WeaponSpriteType::ColtShell, "weapons-gfx/colt-shell.png" },
            { WeaponSpriteType::ColtBullet, "weapons-gfx/colt-bullet.png" },
            { WeaponSpriteType::SocomFire, "weapons-gfx/colt1911-fire.png" },
            { WeaponSpriteType::Bow, "weapons-gfx/bow.png" },
            { WeaponSpriteType::Bow2, "weapons-gfx/bow-2.png" },
            { WeaponSpriteType::BowS, "weapons-gfx/bow-s.png" },
            { WeaponSpriteType::BowA, "weapons-gfx/bow-a.png" },
            { WeaponSpriteType::NBow, "weapons-gfx/n-bow.png" },
            { WeaponSpriteType::NBow2, "weapons-gfx/n-bow-2.png" },
            { WeaponSpriteType::Arrow, "weapons-gfx/arrow.png" },
            { WeaponSpriteType::BowFire, "weapons-gfx/bow-fire.png" },
            { WeaponSpriteType::Flamer, "weapons-gfx/flamer.png" },
            { WeaponSpriteType::Flamer2, "weapons-gfx/flamer-2.png" },
            { WeaponSpriteType::FlamerFire, "weapons-gfx/flamer-fire.png" },
            { WeaponSpriteType::Knife, "weapons-gfx/knife.png" },
            { WeaponSpriteType::Knife2, "weapons-gfx/knife2.png" },
            { WeaponSpriteType::Chainsaw, "weapons-gfx/chainsaw.png" },
            { WeaponSpriteType::Chainsaw2, "weapons-gfx/chainsaw2.png" },
            { WeaponSpriteType::ChainsawFire, "weapons-gfx/chainsaw-fire.png" },
            { WeaponSpriteType::Law, "weapons-gfx/law.png" },
            { WeaponSpriteType::Law2, "weapons-gfx/law-2.png" },
            { WeaponSpriteType::LawFire, "weapons-gfx/law-fire.png" },
            { WeaponSpriteType::M2, "weapons-gfx/m2.png" },
            { WeaponSpriteType::M22, "weapons-gfx/m2-2.png" },
            { WeaponSpriteType::M2Stat, "weapons-gfx/m2-stat.png" },

            { ObjectSpriteType::Flag, "textures/objects/flag.bmp" },
            { ObjectSpriteType::Infflag, "textures/objects/infflag.bmp" },
            { ObjectSpriteType::Medikit, "textures/objects/medikit.png" },
            { ObjectSpriteType::Grenadekit, "textures/objects/grenadekit.png" },
            { ObjectSpriteType::Flamerkit, "textures/objects/flamerkit.png" },
            { ObjectSpriteType::Predatorkit, "textures/objects/predatorkit.png" },
            { ObjectSpriteType::Vestkit, "textures/objects/vestkit.png" },
            { ObjectSpriteType::Berserkerkit, "textures/objects/berserkerkit.png" },
            { ObjectSpriteType::Clusterkit, "textures/objects/clusterkit.png" },
            { ObjectSpriteType::Ilum, "objects-gfx/ilum.png" },
            { ObjectSpriteType::FlagHandle, "objects-gfx/flag.png" },

            // Preserve order of these parachute textures.

            { ObjectSpriteType::ParaRope, "gostek-gfx/para-rope.png" },
            { ObjectSpriteType::Para, "gostek-gfx/para.png" },
            { ObjectSpriteType::Para2, "gostek-gfx/para2.png" },
        };

        std::ranges::for_each(
          std::as_const(all_sprite_file_paths), [&](const auto& type_and_file_path) {
              auto texture_data_or_error = Texture::Load(type_and_file_path.second.c_str());
              auto texture_data =
                Texture::TextureData{ .opengl_id = 0, .width = 0, .height = 0 };
              if (texture_data_or_error.has_value()) {
                  texture_data = *texture_data_or_error;
              } else {
                  switch (texture_data_or_error.error()) {
                      case Texture::LoadError::TextureNotFound: {
                          Spdlog::critical("Sprite file not found: {}", type_and_file_path.second);
                          texture_data = Texture::CreateSinglePixel(0, 0, 0, 0);
                      }
                  }
              }

              all_sprites_.insert({ type_and_file_path.first, texture_data });
          });

        // clang-format off
    soldier_part_type_to_data_.emplace_back(SoldierPartSecondaryWeaponSpriteType::Deagles, nullptr);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Mp5, WeaponSpriteType::Mp5, WeaponSpriteType::Mp52, {5, 10}, {0.300, 0.300}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Ak74, WeaponSpriteType::Ak74, WeaponSpriteType::Ak742, {5, 10}, {0.300, 0.250}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Steyr, WeaponSpriteType::Steyr, WeaponSpriteType::Steyr2, {5, 10}, {0.300, 0.500}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Spas, WeaponSpriteType::Spas, WeaponSpriteType::Spas2, {5, 10}, {0.300, 0.300}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Ruger, WeaponSpriteType::Ruger, WeaponSpriteType::Ruger2, {5, 10}, {0.300, 0.300}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::M79, WeaponSpriteType::M79, WeaponSpriteType::M792, {5, 10}, {0.300, 0.350}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Barrett, WeaponSpriteType::Barrett, WeaponSpriteType::Barrett2, {5, 10}, {0.300, 0.350}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Minimi, WeaponSpriteType::Minimi, WeaponSpriteType::Minimi2, {5, 10}, {0.300, 0.350}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Minigun, WeaponSpriteType::Minigun, WeaponSpriteType::Minigun2, {5, 10}, {0.200, 0.500}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    soldier_part_type_to_data_.emplace_back(SoldierPartSecondaryWeaponSpriteType::Socom, nullptr);
    soldier_part_type_to_data_.emplace_back(SoldierPartSecondaryWeaponSpriteType::Knife, nullptr);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Chainsaw, WeaponSpriteType::Chainsaw, WeaponSpriteType::Chainsaw2, {5, 10}, {0.250, 0.500}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Law, WeaponSpriteType::Law, WeaponSpriteType::Law2, {5, 10}, {0.300, 0.450}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    soldier_part_type_to_data_.emplace_back(SoldierPartSecondaryWeaponSpriteType::Flamebow, nullptr);
    soldier_part_type_to_data_.emplace_back(SoldierPartSecondaryWeaponSpriteType::Bow, nullptr);
    AddSprite(SoldierPartSecondaryWeaponSpriteType::Flamer, WeaponSpriteType::Flamer, WeaponSpriteType::Flamer2, {5, 10}, {0.300, 0.300}, false, false, 0.0F, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Udo, SoldierPartSpriteType::Udo, SoldierPartSpriteType::Udo2, {6, 3}, {0.200, 0.500}, true, true, 5.0, SoldierSpriteColor::Pants, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyUdo, SoldierPartSpriteType::Udo, SoldierPartSpriteType::Udo2, {6, 3}, {0.200, 0.500}, false, true, 5.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Stopa, SoldierPartSpriteType::Stopa, SoldierPartSpriteType::Stopa2, {2, 18}, {0.350, 0.350}, true, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Lecistopa, SoldierPartSpriteType::Lecistopa, SoldierPartSpriteType::Lecistopa2, {2, 18}, {0.350, 0.350}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Noga, SoldierPartSpriteType::Noga, SoldierPartSpriteType::Noga2, {3, 2}, {0.150, 0.550}, true, true, 0.0, SoldierSpriteColor::Pants, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyNoga, SoldierPartSpriteType::Noga, SoldierPartSpriteType::Noga2, {3, 2}, {0.150, 0.550}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Ramie, SoldierPartSpriteType::Ramie, SoldierPartSpriteType::Ramie2, {11, 14}, {0.000, 0.500}, true, true, 0.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyRamie, SoldierPartSpriteType::Ramie, SoldierPartSpriteType::Ramie2, {11, 14}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Reka, SoldierPartSpriteType::Reka, std::nullopt, {14, 15}, {0.000, 0.500}, true, true, 5.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyReka, SoldierPartSpriteType::Reka, SoldierPartSpriteType::Reka2, {14, 15}, {0.000, 0.500}, false, true, 5.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Dlon, SoldierPartSpriteType::Dlon, SoldierPartSpriteType::Dlon2, {15, 19}, {0.000, 0.400}, true, true, 0.0, SoldierSpriteColor::Skin, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Helm, SoldierPartSpriteType::Helm, SoldierPartSpriteType::Helm2, {15, 19}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Kap, SoldierPartSpriteType::Kap, SoldierPartSpriteType::Kap2, {15, 19}, {0.100, 0.400}, false, true, 0.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Udo, SoldierPartSpriteType::Udo, SoldierPartSpriteType::Udo2, {5, 4}, {0.200, 0.650}, true, true, 5.0, SoldierSpriteColor::Pants, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyUdo, SoldierPartSpriteType::Udo, SoldierPartSpriteType::Udo2, {5, 4}, {0.200, 0.650}, false, true, 5.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Stopa, SoldierPartSpriteType::Stopa, SoldierPartSpriteType::Stopa2, {1, 17}, {0.350, 0.350}, true, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Lecistopa, SoldierPartSpriteType::Lecistopa, SoldierPartSpriteType::Lecistopa2, {1, 17}, {0.350, 0.350}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Noga, SoldierPartSpriteType::Noga, SoldierPartSpriteType::Noga2, {4, 1}, {0.150, 0.550}, true, true, 0.0, SoldierSpriteColor::Pants, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyNoga, SoldierPartSpriteType::Noga, SoldierPartSpriteType::Noga2, {4, 1}, {0.150, 0.550}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Klata, SoldierPartSpriteType::Klata, SoldierPartSpriteType::Klata2, {10, 11}, {0.100, 0.300}, true, true, 0.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Kamizelka, SoldierPartSpriteType::Kamizelka, SoldierPartSpriteType::Kamizelka2, {10, 11}, {0.100, 0.300}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyKlata, SoldierPartSpriteType::Klata, SoldierPartSpriteType::Klata2, {10, 11}, {0.100, 0.300}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Biodro, SoldierPartSpriteType::Biodro, SoldierPartSpriteType::Biodro2, {5, 6}, {0.250, 0.600}, true, true, 0.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyBiodro, SoldierPartSpriteType::Biodro, SoldierPartSpriteType::Biodro2, {5, 6}, {0.250, 0.600}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Morda, SoldierPartSpriteType::Morda, SoldierPartSpriteType::Morda2, {9, 12}, {0.000, 0.500}, true, true, 0.0, SoldierSpriteColor::Skin, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyMorda, SoldierPartSpriteType::Morda, SoldierPartSpriteType::Morda2, {9, 12}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::Headblood, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Morda, SoldierPartSpriteType::Morda, SoldierPartSpriteType::Morda2, {9, 12}, {0.500, 0.500}, false, true, 0.0, SoldierSpriteColor::Skin, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyMorda, SoldierPartSpriteType::Morda, SoldierPartSpriteType::Morda2, {9, 12}, {0.500, 0.500}, false, true, 0.0, SoldierSpriteColor::Headblood, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Hair3, SoldierPartSpriteType::Hair3, SoldierPartSpriteType::Hair32, {9, 12}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::Hair, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Helm, SoldierPartSpriteType::Helm, SoldierPartSpriteType::Helm2, {9, 12}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Kap, SoldierPartSpriteType::Kap, SoldierPartSpriteType::Kap2, {9, 12}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Badge, SoldierPartSpriteType::Badge, SoldierPartSpriteType::Badge2, {9, 12}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Hair1, SoldierPartSpriteType::Hair1, SoldierPartSpriteType::Hair12, {9, 12}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::Hair, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Dred, SoldierPartSpriteType::Dred, std::nullopt, {23, 24}, {0.000, 1.220}, false, true, 0.0, SoldierSpriteColor::Hair, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Dred, SoldierPartSpriteType::Dred, std::nullopt, {23, 24}, {0.100, 0.500}, false, true, 0.0, SoldierSpriteColor::Hair, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Dred, SoldierPartSpriteType::Dred, std::nullopt, {23, 24}, {0.040, -0.300}, false, true, 0.0, SoldierSpriteColor::Hair, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Dred, SoldierPartSpriteType::Dred, std::nullopt, {23, 24}, {0.000, -0.900}, false, true, 0.0, SoldierSpriteColor::Hair, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Dred, SoldierPartSpriteType::Dred, std::nullopt, {23, 24}, {-0.200, -1.350}, false, true, 0.0, SoldierSpriteColor::Hair, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Hair2, SoldierPartSpriteType::Hair2, SoldierPartSpriteType::Hair22, {9, 12}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::Hair, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Hair4, SoldierPartSpriteType::Hair4, SoldierPartSpriteType::Hair42, {9, 12}, {0.000, 0.500}, false, true, 0.0, SoldierSpriteColor::Hair, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Cygaro, SoldierPartSpriteType::Cygaro, SoldierPartSpriteType::Cygaro2, {9, 12}, {-0.125, 0.400}, false, true, 0.0, SoldierSpriteColor::Cygar, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Lancuch, SoldierPartSpriteType::Lancuch, std::nullopt, {10, 22}, {0.100, 0.500}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Lancuch, SoldierPartSpriteType::Lancuch, std::nullopt, {11, 22}, {0.100, 0.500}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Metal, SoldierPartSpriteType::Metal, SoldierPartSpriteType::Metal2, {22, 21}, {0.500, 0.700}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Zlotylancuch, SoldierPartSpriteType::Zlotylancuch, std::nullopt, {10, 22}, {0.100, 0.500}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Zlotylancuch, SoldierPartSpriteType::Zlotylancuch, std::nullopt, {11, 22}, {0.100, 0.500}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::Zloto, SoldierPartSpriteType::Zloto, SoldierPartSpriteType::Zloto2, {22, 21}, {0.500, 0.500}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::FragGrenade1, WeaponSpriteType::FragGrenade, std::nullopt, { 5,  6}, { 0.500,  0.100}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::FragGrenade2, WeaponSpriteType::FragGrenade, std::nullopt, { 5,  6}, { 0.500,  0.100}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::FragGrenade3, WeaponSpriteType::FragGrenade, std::nullopt, { 5,  6}, { 0.500,  0.100}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::FragGrenade4, WeaponSpriteType::FragGrenade, std::nullopt, { 5,  6}, { 0.500,  0.100}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::FragGrenade5, WeaponSpriteType::FragGrenade, std::nullopt, { 5,  6}, { 0.500,  0.100}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::ClusterGrenade1, WeaponSpriteType::ClusterGrenade, std::nullopt, { 5,  6}, { 0.500,  0.300}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::ClusterGrenade2, WeaponSpriteType::ClusterGrenade, std::nullopt, { 5,  6}, { 0.500,  0.300}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::ClusterGrenade3, WeaponSpriteType::ClusterGrenade, std::nullopt, { 5,  6}, { 0.500,  0.300}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::ClusterGrenade4, WeaponSpriteType::ClusterGrenade, std::nullopt, { 5,  6}, { 0.500,  0.300}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartTertiaryWeaponSpriteType::ClusterGrenade5, WeaponSpriteType::ClusterGrenade, std::nullopt, { 5,  6}, { 0.500,  0.300}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Nades);
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Deagles, WeaponSpriteType::Deagles, WeaponSpriteType::Deagles2, {16, 15}, { 0.100,  0.800}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::DeaglesClip, WeaponSpriteType::DeaglesClip, WeaponSpriteType::DeaglesClip2, {16, 15}, { 0.100,  0.800}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::DeaglesFire, WeaponSpriteType::DeaglesFire, std::nullopt, {16, 15}, {-0.500,  1.000}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Mp5, WeaponSpriteType::Mp5, WeaponSpriteType::Mp52, {16, 15}, { 0.150,  0.600}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Mp5Clip, WeaponSpriteType::Mp5Clip, WeaponSpriteType::Mp5Clip2, {16, 15}, { 0.150,  0.600}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Mp5Fire, WeaponSpriteType::Mp5Fire, std::nullopt, {16, 15}, { -0.65,  0.850}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Ak74, WeaponSpriteType::Ak74, WeaponSpriteType::Ak742, {16, 15}, { 0.150,  0.500}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Ak74Clip, WeaponSpriteType::Ak74Clip, WeaponSpriteType::Ak74Clip2, {16, 15}, { 0.150,  0.500}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Ak74Fire, WeaponSpriteType::Ak74Fire, std::nullopt, {16, 15}, { -0.37,  0.800}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Steyr, WeaponSpriteType::Steyr, WeaponSpriteType::Steyr2, {16, 15}, { 0.200,  0.600}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::SteyrClip, WeaponSpriteType::SteyrClip, WeaponSpriteType::SteyrClip2, {16, 15}, { 0.200,  0.600}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::SteyrFire, WeaponSpriteType::SteyrFire, std::nullopt, {16, 15}, { -0.24,  0.750}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Spas, WeaponSpriteType::Spas, WeaponSpriteType::Spas2, {16, 15}, { 0.100,  0.600}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    soldier_part_type_to_data_.emplace_back(SoldierPartPrimaryWeaponSpriteType::SpasClip, nullptr);
    AddSprite(SoldierPartPrimaryWeaponSpriteType::SpasFire, WeaponSpriteType::SpasFire, std::nullopt, {16, 15}, {-0.200,  0.900}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Ruger, WeaponSpriteType::Ruger, WeaponSpriteType::Ruger2, {16, 15}, { 0.100,  0.700}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    soldier_part_type_to_data_.emplace_back(SoldierPartPrimaryWeaponSpriteType::RugerClip, nullptr);
    AddSprite(SoldierPartPrimaryWeaponSpriteType::RugerFire, WeaponSpriteType::RugerFire, std::nullopt, {16, 15}, { -0.35,  0.850}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::M79, WeaponSpriteType::M79, WeaponSpriteType::M792, {16, 15}, { 0.100,  0.700}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::M79Clip, WeaponSpriteType::M79Clip, WeaponSpriteType::M79Clip2, {16, 15}, { 0.100,  0.700}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::M79Fire, WeaponSpriteType::M79Fire, std::nullopt, {16, 15}, {-0.400,  0.800}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Barrett, WeaponSpriteType::Barrett, WeaponSpriteType::Barrett2, {16, 15}, { 0.150,  0.700}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::BarrettClip, WeaponSpriteType::BarrettClip, WeaponSpriteType::BarrettClip2, {16, 15}, { 0.150,  0.700}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::BarrettFire, WeaponSpriteType::BarrettFire, std::nullopt, {16, 15}, { -0.15,  0.800}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Minimi, WeaponSpriteType::Minimi, WeaponSpriteType::Minimi2, {16, 15}, { 0.150,  0.600}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::MinimiClip, WeaponSpriteType::MinimiClip, WeaponSpriteType::MinimiClip2, {16, 15}, { 0.150,  0.600}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::MinimiFire, WeaponSpriteType::MinimiFire, std::nullopt, {16, 15}, {-0.200,  0.900}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::MinigunClip, WeaponSpriteType::MinigunClip, std::nullopt, { 8,  7}, { 0.500,  0.100}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Minigun, WeaponSpriteType::Minigun, WeaponSpriteType::Minigun2, {16, 15}, { 0.050,  0.500}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::MinigunFire, WeaponSpriteType::MinigunFire, std::nullopt, {16, 15}, {-0.200,  0.450}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Socom, WeaponSpriteType::Socom, WeaponSpriteType::Socom2, {16, 15}, { 0.200,  0.550}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::SocomClip, WeaponSpriteType::SocomClip, WeaponSpriteType::SocomClip2, {16, 15}, { 0.200,  0.550}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::SocomFire, WeaponSpriteType::SocomFire, std::nullopt, {16, 15}, { -0.24,  0.850}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Knife, WeaponSpriteType::Knife, WeaponSpriteType::Knife2, {16, 20}, {-0.100,  0.600}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    soldier_part_type_to_data_.emplace_back(SoldierPartPrimaryWeaponSpriteType::KnifeClip, nullptr);
    soldier_part_type_to_data_.emplace_back(SoldierPartPrimaryWeaponSpriteType::KnifeFire, nullptr);
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Chainsaw, WeaponSpriteType::Chainsaw, WeaponSpriteType::Chainsaw2, {16, 15}, { 0.100,  0.500}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    soldier_part_type_to_data_.emplace_back(SoldierPartPrimaryWeaponSpriteType::ChainsawClip, nullptr);
    AddSprite(SoldierPartPrimaryWeaponSpriteType::ChainsawFire, WeaponSpriteType::ChainsawFire, std::nullopt, {16, 15}, { 0.000,  0.000}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Law, WeaponSpriteType::Law, WeaponSpriteType::Law2, {16, 15}, { 0.100,  0.600}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    soldier_part_type_to_data_.emplace_back(SoldierPartPrimaryWeaponSpriteType::LawClip, nullptr);
    AddSprite(SoldierPartPrimaryWeaponSpriteType::LawFire, WeaponSpriteType::LawFire, std::nullopt, {16, 15}, {-0.100,  0.550}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Bow, WeaponSpriteType::Bow, WeaponSpriteType::Bow2, {16, 15}, {-0.400,  0.550}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::BowArrow, WeaponSpriteType::BowA, std::nullopt, {16, 15}, { 0.000,  0.550}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::BowString, WeaponSpriteType::BowS, std::nullopt, {16, 15}, {-0.400,  0.550}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::BowReload, WeaponSpriteType::Bow, WeaponSpriteType::Bow2, {16, 15}, {-0.400,  0.550}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::BowArrowReload, WeaponSpriteType::Arrow, std::nullopt, {16, 20}, { 0.000,  0.550}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::BowStringReload, WeaponSpriteType::BowS, std::nullopt, {16, 15}, {-0.400,  0.550}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::BowFire, WeaponSpriteType::BowFire, std::nullopt, {16, 15}, { 0.000,  0.000}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartPrimaryWeaponSpriteType::Flamer, WeaponSpriteType::Flamer, WeaponSpriteType::Flamer2, {16, 15}, { 0.200,  0.700}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    soldier_part_type_to_data_.emplace_back(SoldierPartPrimaryWeaponSpriteType::FlamerClip, nullptr);
    AddSprite(SoldierPartPrimaryWeaponSpriteType::FlamerFire, WeaponSpriteType::FlamerFire, std::nullopt, {16, 15}, { 0.000,  0.000}, false, false, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Base );
    AddSprite(SoldierPartSpriteType::Ramie, SoldierPartSpriteType::Ramie, SoldierPartSpriteType::Ramie2, {10, 13}, {0.000, 0.600}, true, true, 0.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyRamie, SoldierPartSpriteType::Ramie, SoldierPartSpriteType::Ramie2, {10, 13}, {-0.100, 0.500}, false, true, 0.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Reka, SoldierPartSpriteType::Reka, std::nullopt, {13, 16}, {0.000, 0.600}, true, true, 5.0, SoldierSpriteColor::Main, SoldierSpriteAlpha::Base);
    AddSprite(SoldierPartSpriteType::RannyReka, SoldierPartSpriteType::Reka, SoldierPartSpriteType::Reka2, {13, 16}, {0.000, 0.600}, false, true, 5.0, SoldierSpriteColor::None, SoldierSpriteAlpha::Blood);
    AddSprite(SoldierPartSpriteType::Dlon, SoldierPartSpriteType::Dlon, SoldierPartSpriteType::Dlon2, {16, 20}, {0.000, 0.500}, true, true, 0.0, SoldierSpriteColor::Skin, SoldierSpriteAlpha::Base);
        // clang-format on
        Spdlog::info("Loaded {} soldier part sprites", soldier_part_type_to_data_.size());
    }

    ~SpriteManager()
    {
        std::ranges::for_each(std::as_const(all_sprites_), [&](const auto& type_and_texture_data) {
            Texture::Delete(type_and_texture_data.second.opengl_id);
        });
    }

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL textures
    SpriteManager(const SpriteManager&) = delete;
    SpriteManager& operator=(SpriteManager other) = delete;
    SpriteManager(SpriteManager&&) = delete;
    SpriteManager& operator=(SpriteManager&& other) = delete;

    const SoldierPartData* GetSoldierPartData(unsigned int id) const
    {
        return soldier_part_type_to_data_[id].second.get();
    }

    std::variant<SoldierPartSpriteType,
                 SoldierPartPrimaryWeaponSpriteType,
                 SoldierPartSecondaryWeaponSpriteType,
                 SoldierPartTertiaryWeaponSpriteType>
    GetSoldierPartDataType(unsigned int id) const
    {
        return soldier_part_type_to_data_[id].first;
    }

    unsigned int GetSoldierPartCount() const { return soldier_part_type_to_data_.size(); }

    Texture::TextureData GetBulletTexture(WeaponSpriteType weapon_sprite_type) const
    {
        return all_sprites_.at(weapon_sprite_type);
    }

    Texture::TextureData GetBulletTexture(const Bullet& bullet) const
    {
        auto bullet_style = bullet.style;
        auto weapon_kind = bullet.weapon;

        switch (bullet_style) {
            case BulletType::Bullet: {
                switch (weapon_kind) {
                    case Soldank::WeaponType::DesertEagles:
                        return all_sprites_.at(WeaponSpriteType::DeaglesBullet);
                    case Soldank::WeaponType::MP5:
                        return all_sprites_.at(WeaponSpriteType::Mp5Bullet);
                    case Soldank::WeaponType::Ak74:
                        return all_sprites_.at(WeaponSpriteType::Ak74Bullet);
                    case Soldank::WeaponType::SteyrAUG:
                        return all_sprites_.at(WeaponSpriteType::SteyrBullet);
                    case Soldank::WeaponType::Spas12:
                        return all_sprites_.at(WeaponSpriteType::SpasBullet);
                    case Soldank::WeaponType::Ruger77:
                        return all_sprites_.at(WeaponSpriteType::RugerBullet);
                    case Soldank::WeaponType::M79:
                        return all_sprites_.at(WeaponSpriteType::M79Bullet);
                    case Soldank::WeaponType::Barrett:
                        return all_sprites_.at(WeaponSpriteType::BarrettBullet);
                    case Soldank::WeaponType::Minimi:
                        return all_sprites_.at(WeaponSpriteType::MinimiBullet);
                    case Soldank::WeaponType::Minigun:
                        return all_sprites_.at(WeaponSpriteType::MinigunBullet);
                    case Soldank::WeaponType::USSOCOM:
                        return all_sprites_.at(WeaponSpriteType::ColtBullet);
                    case Soldank::WeaponType::Knife:
                        return all_sprites_.at(WeaponSpriteType::Knife); // TODO
                    case Soldank::WeaponType::Chainsaw:
                        return all_sprites_.at(WeaponSpriteType::Chainsaw); // TODO
                    case Soldank::WeaponType::LAW:
                        return all_sprites_.at(WeaponSpriteType::Law); // TODO
                    case Soldank::WeaponType::FlameBow:
                        return all_sprites_.at(WeaponSpriteType::Arrow); // TODO
                    case Soldank::WeaponType::Bow:
                        return all_sprites_.at(WeaponSpriteType::Arrow); // TODO
                    case Soldank::WeaponType::Flamer:
                        return all_sprites_.at(WeaponSpriteType::Flamer); // TODO
                    case Soldank::WeaponType::M2:
                        return all_sprites_.at(WeaponSpriteType::M2); // TODO
                    case Soldank::WeaponType::NoWeapon:
                        return all_sprites_.at(WeaponSpriteType::Bullet); // TODO
                    case Soldank::WeaponType::FragGrenade:
                        return all_sprites_.at(WeaponSpriteType::FragGrenade); // TODO
                    case Soldank::WeaponType::ClusterGrenade:
                        return all_sprites_.at(WeaponSpriteType::ClusterGrenade); // TODO
                    case Soldank::WeaponType::Cluster:
                        return all_sprites_.at(WeaponSpriteType::Cluster); // TODO
                    case Soldank::WeaponType::ThrownKnife:
                        return all_sprites_.at(WeaponSpriteType::Knife); // TODO
                }
            }
            case BulletType::FragGrenade:
            case BulletType::GaugeBullet:
            case BulletType::M79Grenade:
            case BulletType::Flame:
                // TODO: I'm not sure which sprite to return here
                //       I think this one is handled with sparks
                return all_sprites_.at(WeaponSpriteType::FlamerFire);
            case BulletType::Fist:
            case BulletType::Arrow:
                // TODO: I'm not sure which sprite to return here
                return all_sprites_.at(WeaponSpriteType::Arrow);
            case BulletType::FlameArrow:
                // TODO: I'm not sure which sprite to return here
                return all_sprites_.at(WeaponSpriteType::Arrow);
            case BulletType::ClusterGrenade:
            case BulletType::Cluster:
            case BulletType::Blade:
                // TODO: I'm not sure which sprite to return here
                return all_sprites_.at(WeaponSpriteType::Bullet);
            case BulletType::LAWMissile:
                // TODO: I'm not sure which sprite to return here
                return all_sprites_.at(WeaponSpriteType::Bullet);
            case BulletType::ThrownKnife:
                return all_sprites_.at(WeaponSpriteType::Knife);
            case BulletType::M2Bullet:
                // TODO: I'm not sure which sprite to return here
                return all_sprites_.at(WeaponSpriteType::Bullet);
        }
    }

    Texture::TextureData GetItemTexture(ItemType item_type, bool flipped = false) const
    {
        if (flipped) {
            switch (item_type) {
                case ItemType::USSOCOM:
                    return all_sprites_.at(WeaponSpriteType::NSocom2);
                case ItemType::DesertEagles:
                    return all_sprites_.at(WeaponSpriteType::NDeagles2);
                case ItemType::MP5:
                    return all_sprites_.at(WeaponSpriteType::Mp52);
                case ItemType::Ak74:
                    return all_sprites_.at(WeaponSpriteType::Ak742);
                case ItemType::SteyrAUG:
                    return all_sprites_.at(WeaponSpriteType::Steyr2);
                case ItemType::Spas12:
                    return all_sprites_.at(WeaponSpriteType::Spas2);
                case ItemType::Ruger77:
                    return all_sprites_.at(WeaponSpriteType::Ruger2);
                case ItemType::M79:
                    return all_sprites_.at(WeaponSpriteType::M792);
                case ItemType::Barrett:
                    return all_sprites_.at(WeaponSpriteType::Barrett2);
                case ItemType::Minimi:
                    return all_sprites_.at(WeaponSpriteType::Minimi2);
                case ItemType::Minigun:
                    return all_sprites_.at(WeaponSpriteType::Minigun2);
                case ItemType::Bow:
                    return all_sprites_.at(WeaponSpriteType::NBow2);
                case ItemType::Knife:
                    return all_sprites_.at(WeaponSpriteType::Knife2);
                case ItemType::Chainsaw:
                    return all_sprites_.at(WeaponSpriteType::Chainsaw2);
                case ItemType::LAW:
                    return all_sprites_.at(WeaponSpriteType::Law2);
                default:
                    break;
            }
        }

        switch (item_type) {
            case ItemType::AlphaFlag:
            case ItemType::BravoFlag:
            case ItemType::PointmatchFlag:
                return all_sprites_.at(ObjectSpriteType::Flag);
            case ItemType::USSOCOM:
                return all_sprites_.at(WeaponSpriteType::NSocom);
            case ItemType::DesertEagles:
                return all_sprites_.at(WeaponSpriteType::NDeagles);
            case ItemType::MP5:
                return all_sprites_.at(WeaponSpriteType::Mp5);
            case ItemType::Ak74:
                return all_sprites_.at(WeaponSpriteType::Ak74);
            case ItemType::SteyrAUG:
                return all_sprites_.at(WeaponSpriteType::Steyr);
            case ItemType::Spas12:
                return all_sprites_.at(WeaponSpriteType::Spas);
            case ItemType::Ruger77:
                return all_sprites_.at(WeaponSpriteType::Ruger);
            case ItemType::M79:
                return all_sprites_.at(WeaponSpriteType::M79);
            case ItemType::Barrett:
                return all_sprites_.at(WeaponSpriteType::Barrett);
            case ItemType::Minimi:
                return all_sprites_.at(WeaponSpriteType::Minimi);
            case ItemType::Minigun:
                return all_sprites_.at(WeaponSpriteType::Minigun);
            case ItemType::Bow:
                return all_sprites_.at(WeaponSpriteType::NBow);
            case ItemType::MedicalKit:
                return all_sprites_.at(ObjectSpriteType::Medikit);
            case ItemType::GrenadeKit:
                return all_sprites_.at(ObjectSpriteType::Grenadekit);
            case ItemType::FlamerKit:
                return all_sprites_.at(ObjectSpriteType::Flamerkit);
            case ItemType::PredatorKit:
                return all_sprites_.at(ObjectSpriteType::Predatorkit);
            case ItemType::VestKit:
                return all_sprites_.at(ObjectSpriteType::Vestkit);
            case ItemType::BerserkKit:
                return all_sprites_.at(ObjectSpriteType::Berserkerkit);
            case ItemType::ClusterKit:
                return all_sprites_.at(ObjectSpriteType::Clusterkit);
            case ItemType::Parachute:
            case ItemType::Knife:
                return all_sprites_.at(WeaponSpriteType::Knife);
            case ItemType::Chainsaw:
                return all_sprites_.at(WeaponSpriteType::Chainsaw);
            case ItemType::LAW:
                return all_sprites_.at(WeaponSpriteType::Law);
            case ItemType::M2:
                break;
        }
    }

    Texture::TextureData GetObjectSprite(ObjectSpriteType object_sprite_type) const
    {
        switch (object_sprite_type) {
            case ObjectSpriteType::Flag:
                return all_sprites_.at(ObjectSpriteType::Flag);
            case ObjectSpriteType::Infflag:
                return all_sprites_.at(ObjectSpriteType::Infflag);
            case ObjectSpriteType::Medikit:
                return all_sprites_.at(ObjectSpriteType::Medikit);
            case ObjectSpriteType::Grenadekit:
                return all_sprites_.at(ObjectSpriteType::Grenadekit);
            case ObjectSpriteType::Flamerkit:
                return all_sprites_.at(ObjectSpriteType::Flamerkit);
            case ObjectSpriteType::Predatorkit:
                return all_sprites_.at(ObjectSpriteType::Predatorkit);
            case ObjectSpriteType::Vestkit:
                return all_sprites_.at(ObjectSpriteType::Vestkit);
            case ObjectSpriteType::Berserkerkit:
                return all_sprites_.at(ObjectSpriteType::Berserkerkit);
            case ObjectSpriteType::Clusterkit:
                return all_sprites_.at(ObjectSpriteType::Clusterkit);
            case ObjectSpriteType::Ilum:
                return all_sprites_.at(ObjectSpriteType::Ilum);
            case ObjectSpriteType::FlagHandle:
                return all_sprites_.at(ObjectSpriteType::FlagHandle);
            case ObjectSpriteType::ParaRope:
                return all_sprites_.at(ObjectSpriteType::ParaRope);
            case ObjectSpriteType::Para:
                return all_sprites_.at(ObjectSpriteType::Para);
            case ObjectSpriteType::Para2:
                return all_sprites_.at(ObjectSpriteType::Para2);
                break;
        }
    }

private:
    void AddSprite(TSoldierPartSpriteKey soldier_part_sprite_key,
                   TSpriteKey sprite_key,
                   std::optional<TSpriteKey> flipped_sprite_key,
                   glm::uvec2 point,
                   glm::vec2 center,
                   bool visible,
                   bool team,
                   float flexibility,
                   SoldierSpriteColor color,
                   SoldierSpriteAlpha alpha)
    {
        std::optional<Texture::TextureData> flipped_sprite_texture_data = std::nullopt;
        if (flipped_sprite_key) {
            flipped_sprite_texture_data = all_sprites_[*flipped_sprite_key];
        }
        soldier_part_type_to_data_.emplace_back(
          soldier_part_sprite_key,
          std::make_unique<SoldierPartData>(all_sprites_[sprite_key],
                                            point,
                                            center,
                                            visible,
                                            flipped_sprite_texture_data,
                                            team,
                                            flexibility,
                                            color,
                                            alpha));
    }

    TSoldierPartList soldier_part_type_to_data_;

    std::unordered_map<TSpriteKey, Texture::TextureData> all_sprites_;
};
} // namespace Soldank::Sprites
