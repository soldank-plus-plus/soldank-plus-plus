module;

#include "core/math/Glm.hpp"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <ranges>
#include <string_view>
#include <array>
#include <vector>
#include <string>
#include <memory>

export module Shared.Core.Animations:AnimationData;

import Shared.Core.Utility.Getline;
import Shared.Core.Data.FileReader;
import Shared.Core.Data.IFileReader;


export namespace Soldank
{
enum class AnimationType
{
    Stand = 0,
    Run,
    RunBack,
    Jump,
    JumpSide,
    Fall,
    Crouch,
    CrouchRun,
    Reload,
    Throw,
    Recoil,
    SmallRecoil,
    Shotgun,
    ClipOut,
    ClipIn,
    SlideBack,
    Change,
    ThrowWeapon,
    WeaponNone,
    Punch,
    ReloadBow,
    Barret,
    Roll,
    RollBack,
    CrouchRunBack,
    Cigar,
    Match,
    Smoke,
    Wipe,
    Groin,
    Piss,
    Mercy,
    Mercy2,
    TakeOff,
    Prone,
    Victory,
    Aim,
    HandsUpAim,
    ProneMove,
    GetUp,
    AimRecoil,
    HandsUpRecoil,
    Melee,
    Own,
};

constexpr const std::string_view ANIMATION_BASE_DIRECTORY_PATH = "anims/";
constexpr const std::string_view ANIMATION_STAND_FILE = "stoi.poa";
constexpr const std::string_view ANIMATION_RUN_FILE = "biega.poa";
constexpr const std::string_view ANIMATION_RUN_BACK_FILE = "biegatyl.poa";
constexpr const std::string_view ANIMATION_JUMP_FILE = "skok.poa";
constexpr const std::string_view ANIMATION_JUMP_SIDE_FILE = "skokwbok.poa";
constexpr const std::string_view ANIMATION_FALL_FILE = "spada.poa";
constexpr const std::string_view ANIMATION_CROUCH_FILE = "kuca.poa";
constexpr const std::string_view ANIMATION_CROUCH_RUN_FILE = "kucaidzie.poa";
constexpr const std::string_view ANIMATION_RELOAD_FILE = "laduje.poa";
constexpr const std::string_view ANIMATION_THROW_FILE = "rzuca.poa";
constexpr const std::string_view ANIMATION_RECOIL_FILE = "odrzut.poa";
constexpr const std::string_view ANIMATION_SMALL_RECOIL_FILE = "odrzut2.poa";
constexpr const std::string_view ANIMATION_SHOTGUN_FILE = "shotgun.poa";
constexpr const std::string_view ANIMATION_CLIP_OUT_FILE = "clipout.poa";
constexpr const std::string_view ANIMATION_CLIP_IN_FILE = "clipin.poa";
constexpr const std::string_view ANIMATION_SLIDE_BACK_FILE = "slideback.poa";
constexpr const std::string_view ANIMATION_CHANGE_FILE = "change.poa";
constexpr const std::string_view ANIMATION_THROW_WEAPON_FILE = "wyrzuca.poa";
constexpr const std::string_view ANIMATION_WEAPON_NONE_FILE = "bezbroni.poa";
constexpr const std::string_view ANIMATION_PUNCH_FILE = "bije.poa";
constexpr const std::string_view ANIMATION_RELOAD_BOW_FILE = "strzala.poa";
constexpr const std::string_view ANIMATION_BARRET_FILE = "barret.poa";
constexpr const std::string_view ANIMATION_ROLL_FILE = "skokdolobrot.poa";
constexpr const std::string_view ANIMATION_ROLL_BACK_FILE = "skokdolobrottyl.poa";
constexpr const std::string_view ANIMATION_CROUCH_RUN_BACK_FILE = "kucaidzietyl.poa";
constexpr const std::string_view ANIMATION_CIGAR_FILE = "cigar.poa";
constexpr const std::string_view ANIMATION_MATCH_FILE = "match.poa";
constexpr const std::string_view ANIMATION_SMOKE_FILE = "smoke.poa";
constexpr const std::string_view ANIMATION_WIPE_FILE = "wipe.poa";
constexpr const std::string_view ANIMATION_GROIN_FILE = "krocze.poa";
constexpr const std::string_view ANIMATION_PISS_FILE = "szcza.poa";
constexpr const std::string_view ANIMATION_MERCY_FILE = "samo.poa";
constexpr const std::string_view ANIMATION_MERCY2_FILE = "samo2.poa";
constexpr const std::string_view ANIMATION_TAKE_OFF_FILE = "takeoff.poa";
constexpr const std::string_view ANIMATION_PRONE_FILE = "lezy.poa";
constexpr const std::string_view ANIMATION_VICTORY_FILE = "cieszy.poa";
constexpr const std::string_view ANIMATION_AIM_FILE = "celuje.poa";
constexpr const std::string_view ANIMATION_HANDS_UP_AIM_FILE = "gora.poa";
constexpr const std::string_view ANIMATION_PRONE_MOVE_FILE = "lezyidzie.poa";
constexpr const std::string_view ANIMATION_GET_UP_FILE = "wstaje.poa";
constexpr const std::string_view ANIMATION_AIM_RECOIL_FILE = "celujeodrzut.poa";
constexpr const std::string_view ANIMATION_HANDS_UP_RECOIL_FILE = "goraodrzut.poa";
constexpr const std::string_view ANIMATION_MELEE_FILE = "kolba.poa";
constexpr const std::string_view ANIMATION_OWN_FILE = "rucha.poa";

struct AnimationFrame
{
    std::vector<glm::vec2> positions;
};

class AnimationData
{
public:
    AnimationData(AnimationType animation_type,
                  bool looped,
                  int speed,
                  const std::vector<AnimationFrame>& frames)
        : animation_type_(animation_type)
        , looped_(looped)
        , speed_(speed)
        , frames_(frames) {};

    AnimationType GetAnimationType() const { return animation_type_; }
    bool GetLooped() const { return looped_; }
    int GetSpeed() const { return speed_; }
    const std::vector<AnimationFrame>& GetFrames() const { return frames_; }

private:
    AnimationType animation_type_;
    bool looped_;
    int speed_;
    std::vector<AnimationFrame> frames_;
};

class AnimationDataManager
{
public:
    // TODO: add error handling for when animation could not be read from file
    // TODO: add error handling for when animation of type animation_type had already been loaded
    // before
    void LoadAnimationData(AnimationType animation_type,
                           const std::string& file_path,
                           bool looped,
                           int speed,
                           const IFileReader& file_reader = FileReader())
    {
        spdlog::info("LoadAnimationData");

        auto file_data = file_reader.Read(file_path);
        if (!file_data.has_value()) {
            spdlog::critical("Animation file not found {}", file_path);
            std::string message = "Could not open file: " + file_path;
            throw std::runtime_error(message.c_str());
        }
        std::stringstream data_buffer{ *file_data };
        spdlog::info("Animation: {}", file_path);

        std::string line;
        std::vector<glm::vec2> positions;
        std::vector<AnimationFrame> frames;

        GetlineSafe(data_buffer, line);
        while (!data_buffer.eof() && (line != "ENDFILE")) {
            if (line == "NEXTFRAME") {
                frames.push_back({ positions });
                positions.clear();
            } else {
                unsigned int point = std::stoul(line);
                if (point != positions.size() + 1) {
                    spdlog::error(
                      "Error in Animation loading: {} != {}", point, positions.size() + 1);
                }

                GetlineSafe(data_buffer, line);
                float x = std::stof(line);
                GetlineSafe(data_buffer, line); // y is not used for some reason
                GetlineSafe(data_buffer, line);
                float z = std::stof(line);

                positions.emplace_back(-3.0F * x / 1.1F, -3.0 * z);
            }

            GetlineSafe(data_buffer, line);
        }

        frames.push_back({ positions });

        animation_datas_.push_back(
          std::make_shared<AnimationData>(animation_type, looped, speed, frames));
    }

    void LoadAllAnimationDatas(const IFileReader& file_reader = FileReader())
    {
        const auto get_file_path = [](std::string_view file_name) {
            return std::string{ ANIMATION_BASE_DIRECTORY_PATH } + std::string{ file_name };
        };
        // clang-format off
    LoadAnimationData(AnimationType::Stand,          get_file_path(ANIMATION_STAND_FILE),           true,  3, file_reader);
    LoadAnimationData(AnimationType::Run,            get_file_path(ANIMATION_RUN_FILE),             true,  1, file_reader);
    LoadAnimationData(AnimationType::RunBack,        get_file_path(ANIMATION_RUN_BACK_FILE),        true,  1, file_reader);
    LoadAnimationData(AnimationType::Jump,           get_file_path(ANIMATION_JUMP_FILE),            false, 1, file_reader);
    LoadAnimationData(AnimationType::JumpSide,       get_file_path(ANIMATION_JUMP_SIDE_FILE),       false, 1, file_reader);
    LoadAnimationData(AnimationType::Fall,           get_file_path(ANIMATION_FALL_FILE),            false, 1, file_reader);
    LoadAnimationData(AnimationType::Crouch,         get_file_path(ANIMATION_CROUCH_FILE),          false, 1, file_reader);
    LoadAnimationData(AnimationType::CrouchRun,      get_file_path(ANIMATION_CROUCH_RUN_FILE),      true,  2, file_reader);
    LoadAnimationData(AnimationType::Reload,         get_file_path(ANIMATION_RELOAD_FILE),          false, 2, file_reader);
    LoadAnimationData(AnimationType::Throw,          get_file_path(ANIMATION_THROW_FILE),           false, 1, file_reader);
    LoadAnimationData(AnimationType::Recoil,         get_file_path(ANIMATION_RECOIL_FILE),          false, 1, file_reader);
    LoadAnimationData(AnimationType::SmallRecoil,    get_file_path(ANIMATION_SMALL_RECOIL_FILE),    false, 1, file_reader);
    LoadAnimationData(AnimationType::Shotgun,        get_file_path(ANIMATION_SHOTGUN_FILE),         false, 1, file_reader);
    LoadAnimationData(AnimationType::ClipOut,        get_file_path(ANIMATION_CLIP_OUT_FILE),        false, 3, file_reader);
    LoadAnimationData(AnimationType::ClipIn,         get_file_path(ANIMATION_CLIP_IN_FILE),         false, 3, file_reader);
    LoadAnimationData(AnimationType::SlideBack,      get_file_path(ANIMATION_SLIDE_BACK_FILE),      false, 2, file_reader);
    LoadAnimationData(AnimationType::Change,         get_file_path(ANIMATION_CHANGE_FILE),          false, 1, file_reader);
    LoadAnimationData(AnimationType::ThrowWeapon,    get_file_path(ANIMATION_THROW_WEAPON_FILE),    false, 1, file_reader);
    LoadAnimationData(AnimationType::WeaponNone,     get_file_path(ANIMATION_WEAPON_NONE_FILE),     false, 3, file_reader);
    LoadAnimationData(AnimationType::Punch,          get_file_path(ANIMATION_PUNCH_FILE),           false, 1, file_reader);
    LoadAnimationData(AnimationType::ReloadBow,      get_file_path(ANIMATION_RELOAD_BOW_FILE),      false, 1, file_reader);
    LoadAnimationData(AnimationType::Barret,         get_file_path(ANIMATION_BARRET_FILE),          false, 9, file_reader);
    LoadAnimationData(AnimationType::Roll,           get_file_path(ANIMATION_ROLL_FILE),            false, 1, file_reader);
    LoadAnimationData(AnimationType::RollBack,       get_file_path(ANIMATION_ROLL_BACK_FILE),       false, 1, file_reader);
    LoadAnimationData(AnimationType::CrouchRunBack,  get_file_path(ANIMATION_CROUCH_RUN_BACK_FILE), true,  2, file_reader);
    LoadAnimationData(AnimationType::Cigar,          get_file_path(ANIMATION_CIGAR_FILE),           false, 3, file_reader);
    LoadAnimationData(AnimationType::Match,          get_file_path(ANIMATION_MATCH_FILE),           false, 3, file_reader);
    LoadAnimationData(AnimationType::Smoke,          get_file_path(ANIMATION_SMOKE_FILE),           false, 4, file_reader);
    LoadAnimationData(AnimationType::Wipe,           get_file_path(ANIMATION_WIPE_FILE),            false, 4, file_reader);
    LoadAnimationData(AnimationType::Groin,          get_file_path(ANIMATION_GROIN_FILE),           false, 2, file_reader);
    LoadAnimationData(AnimationType::Piss,           get_file_path(ANIMATION_PISS_FILE),            false, 8, file_reader);
    LoadAnimationData(AnimationType::Mercy,          get_file_path(ANIMATION_MERCY_FILE),           false, 3, file_reader);
    LoadAnimationData(AnimationType::Mercy2,         get_file_path(ANIMATION_MERCY2_FILE),          false, 3, file_reader);
    LoadAnimationData(AnimationType::TakeOff,        get_file_path(ANIMATION_TAKE_OFF_FILE),        false, 2, file_reader);
    LoadAnimationData(AnimationType::Prone,          get_file_path(ANIMATION_PRONE_FILE),           false, 1, file_reader);
    LoadAnimationData(AnimationType::Victory,        get_file_path(ANIMATION_VICTORY_FILE),         false, 3, file_reader);
    LoadAnimationData(AnimationType::Aim,            get_file_path(ANIMATION_AIM_FILE),             false, 2, file_reader);
    LoadAnimationData(AnimationType::HandsUpAim,     get_file_path(ANIMATION_HANDS_UP_AIM_FILE),    false, 2, file_reader);
    LoadAnimationData(AnimationType::ProneMove,      get_file_path(ANIMATION_PRONE_MOVE_FILE),      true,  2, file_reader);
    LoadAnimationData(AnimationType::GetUp,          get_file_path(ANIMATION_GET_UP_FILE),          false, 1, file_reader);
    LoadAnimationData(AnimationType::AimRecoil,      get_file_path(ANIMATION_AIM_RECOIL_FILE),      false, 1, file_reader);
    LoadAnimationData(AnimationType::HandsUpRecoil,  get_file_path(ANIMATION_HANDS_UP_RECOIL_FILE), false, 1, file_reader);
    LoadAnimationData(AnimationType::Melee,          get_file_path(ANIMATION_MELEE_FILE),           false, 1, file_reader);
    LoadAnimationData(AnimationType::Own,            get_file_path(ANIMATION_OWN_FILE),             false, 3, file_reader);
        // clang-format on
    }

    std::shared_ptr<const AnimationData> Get(AnimationType animation_type) const
    {
        for (const auto& animation_data : animation_datas_) {
            if (animation_data->GetAnimationType() == animation_type) {
                return animation_data;
            }
        }

        // TODO: error handling
        std::unreachable();
    }

private:
    std::vector<std::shared_ptr<const AnimationData>> animation_datas_;
};
} // namespace Soldank
