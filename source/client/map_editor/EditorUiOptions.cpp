module;

#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <string_view>
#include <utility>

export module MapEditor.EditorUiOptions;

import MapEditorState;

import Shared.Core.Map.PMSEnums;

export namespace Soldank::EditorUiOptions
{
struct JetFuelOption
{
    std::string_view label;
    std::string_view id;
    int value;
};

std::span<const std::pair<std::string_view, ToolType>> GetToolOptions()
{
    static constexpr std::array<std::pair<std::string_view, ToolType>, 11> OPTIONS = {
        { { "Transform tool", ToolType::Transform },
          { "Polygon tool", ToolType::Polygon },
          { "Vertex selection tool", ToolType::VertexSelection },
          { "Selection tool", ToolType::Selection },
          { "Vertex color tool", ToolType::VertexColor },
          { "Color tool", ToolType::Color },
          { "Texture tool", ToolType::Texture },
          { "Scenery tool", ToolType::Scenery },
          { "Waypoint tool", ToolType::Waypoint },
          { "Spawnpoint tool", ToolType::Spawnpoint },
          { "Color picker tool", ToolType::ColorPicker } }
    };
    return OPTIONS;
}

std::span<const ToolType> GetDisabledToolTypes()
{
    static constexpr std::array<ToolType, 3> OPTIONS = {
        ToolType::VertexColor,
        ToolType::Texture,
        ToolType::Waypoint,
    };
    return OPTIONS;
}

std::span<const std::pair<std::string_view, PMSSpawnPointType>> GetSpawnPointOptions()
{
    static constexpr std::array<std::pair<std::string_view, PMSSpawnPointType>, 17> OPTIONS = {
        { { "Player Spawn", PMSSpawnPointType::General },
          { "Alpha Team", PMSSpawnPointType::Alpha },
          { "Bravo Team", PMSSpawnPointType::Bravo },
          { "Charlie Team", PMSSpawnPointType::Charlie },
          { "Delta Team", PMSSpawnPointType::Delta },
          { "Alpha Flag", PMSSpawnPointType::AlphaFlag },
          { "Bravo Flag", PMSSpawnPointType::BravoFlag },
          { "Pointmatch Flag", PMSSpawnPointType::YellowFlag },
          { "Grenade Kit", PMSSpawnPointType::Grenades },
          { "Medical Kit", PMSSpawnPointType::Medkits },
          { "Cluster Grenade Kit", PMSSpawnPointType::Clusters },
          { "Vest Kit", PMSSpawnPointType::Vest },
          { "Flamer Kit", PMSSpawnPointType::Flamer },
          { "Berserker Kit", PMSSpawnPointType::Berserker },
          { "Predator Kit", PMSSpawnPointType::Predator },
          { "Rambo Bow", PMSSpawnPointType::RamboBow },
          { "Stationary Gun", PMSSpawnPointType::StatGun } }
    };
    return OPTIONS;
}

std::span<const std::pair<std::string_view, PMSPolygonType>> GetPolygonTypeOptions()
{
    static constexpr std::array<std::pair<std::string_view, PMSPolygonType>, 24> OPTIONS = {
        { { "Normal", PMSPolygonType::Normal },
          { "All Bullets Collide", PMSPolygonType::OnlyBulletsCollide },
          { "All Players Collide", PMSPolygonType::OnlyPlayersCollide },
          { "Doesn't Collide", PMSPolygonType::NoCollide },
          { "Icy", PMSPolygonType::Ice },
          { "Deadly", PMSPolygonType::Deadly },
          { "Bloody Deadly", PMSPolygonType::BloodyDeadly },
          { "Hurting", PMSPolygonType::Hurts },
          { "Regenerative", PMSPolygonType::Regenerates },
          { "Lava", PMSPolygonType::Lava },
          { "Only Alpha Bullets Collide", PMSPolygonType::AlphaBullets },
          { "Only Alpha Players Collide", PMSPolygonType::AlphaPlayers },
          { "Only Bravo Bullets Collide", PMSPolygonType::BravoBullets },
          { "Only Bravo Players Collide", PMSPolygonType::BravoPlayers },
          { "Only Charlie Bullets Collide", PMSPolygonType::CharlieBullets },
          { "Only Charlie Players Collide", PMSPolygonType::CharliePlayers },
          { "Only Delta Bullets Collide", PMSPolygonType::DeltaBullets },
          { "Only Delta Players Collide", PMSPolygonType::DeltaPlayers },
          { "Bouncy", PMSPolygonType::Bouncy },
          { "Explosive", PMSPolygonType::Explosive },
          { "Hurting Flaggers", PMSPolygonType::HurtFlaggers },
          { "Only Flaggers Collide", PMSPolygonType::FlaggerCollides },
          { "Only Non Flaggers Collide", PMSPolygonType::NonFlaggerCollides },
          { "Only Flag Collide", PMSPolygonType::FlagCollides } }
    };
    return OPTIONS;
}

std::span<const std::pair<std::string_view, PMSWeatherType>> GetWeatherOptions()
{
    static constexpr std::array<std::pair<std::string_view, PMSWeatherType>, 4> OPTIONS = {
        { { "None", PMSWeatherType::None },
          { "Rain", PMSWeatherType::Rain },
          { "Sandstorm", PMSWeatherType::Sandstorm },
          { "Snow", PMSWeatherType::Snow } }
    };
    return OPTIONS;
}

std::span<const std::pair<std::string_view, PMSStepType>> GetStepOptions()
{
    static constexpr std::array<std::pair<std::string_view, PMSStepType>, 3> OPTIONS = {
        { { "Hard", PMSStepType::HardGround },
          { "Soft", PMSStepType::SoftGround },
          { "None", PMSStepType::None } }
    };
    return OPTIONS;
}

std::span<const JetFuelOption> GetJetFuelOptions()
{
    static constexpr std::array<JetFuelOption, 8> OPTIONS = {
        { { "None", "None", 0 },
          { "Minimal", "Minimal", 12 },
          { "Very Low", "VeryLow", 45 },
          { "Low", "Low", 95 },
          { "Normal", "Normal", 190 },
          { "High", "High", 320 },
          { "Extreme", "Extreme", 800 },
          { "Infinite", "Infinite", 25999 } }
    };
    return OPTIONS;
}

std::span<const std::string_view> GetSupportedImageExtensions()
{
    static constexpr std::array<std::string_view, 5> EXTENSIONS = {
        ".bmp", ".jpg", ".jpeg", ".gif", ".png",
    };
    return EXTENSIONS;
}

int AlphaToOpacityPercent(float alpha)
{
    return std::clamp(static_cast<int>(std::lround(alpha * 100.0F)), 0, 100);
}

float OpacityPercentToAlpha(int opacity_percent)
{
    return static_cast<float>(std::clamp(opacity_percent, 0, 100)) / 100.0F;
}
} // namespace Soldank::EditorUiOptions
