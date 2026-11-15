module;

#include <cstddef>
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

export module Shared.Networking.GameModeProtocol;

import Shared.Networking.NetworkPackets;
import Shared.Core.GameModes.GameModeRuntime;
import Shared.Core.GameModes.WorldGameModeAdapter;
import Shared.Core.State.StateManager;
import Shared.Core.Types.ItemType;

export namespace Soldank
{
MatchStatePacket MakeMatchStatePacket(const StateManager& state)
{
    MatchStatePacket packet;
    const auto* mode = state.GetGameMode();
    if (mode == nullptr) {
        return packet;
    }
    const auto& settings = mode->Settings();
    std::ranges::copy(settings.id, packet.mode_id.begin());
    std::ranges::copy(settings.display_name.substr(0, 63), packet.display_name.begin());
    std::ranges::copy(state.GetLoadedMapPath().substr(0, 255), packet.map_path.begin());
    packet.server_tick = state.GetGameTick();
    packet.team_count = settings.team_count;
    packet.survival = settings.rounds.enabled;
    packet.friendly_fire = settings.friendly_fire;
    packet.score_limit = settings.score_limit.value_or(0);
    packet.time_limit = settings.time_limit.value_or(0);
    packet.state = mode->GetState();
    packet.definition_fingerprint = mode->GetDefinition().GetFingerprint();
    state.ForEachItem([&](const auto& item) {
        for (const auto& spec : settings.objectives) {
            if (item.style == spec.item_type && packet.objective_count < packet.objectives.size()) {
                packet.objectives.at(packet.objective_count++) = {
                    .item_id = item.id,
                    .item_type = static_cast<std::uint8_t>(item.style),
                    .carrier = item.holding_soldier_id,
                    .home = item.in_base,
                };
            }
        }
    });
    return packet;
}
bool IsValidMatchStatePacket(const MatchStatePacket& packet)
{
    if (packet.version != GAME_MODE_PROTOCOL_VERSION || packet.mode_id.front() == '\0' ||
        packet.mode_id.back() != '\0' || packet.display_name.back() != '\0' ||
        packet.map_path.back() != '\0' || packet.objective_count > packet.objectives.size() ||
        (packet.team_count != 0 && packet.team_count != 2 && packet.team_count != 4) ||
        packet.state.phase > MatchPhase::Finished || packet.score_limit < 0 ||
        packet.time_limit > 1000000000U) {
        return false;
    }
    const std::string_view id(packet.mode_id.data());
    if (!std::ranges::all_of(id, [](char character) {
            return (character >= 'a' && character <= 'z') ||
                   (character >= '0' && character <= '9') || character == '_';
        })) {
        return false;
    }
    for (const auto& player : packet.state.players) {
        if (player.points < 0 || player.kills < 0 || player.deaths < 0 || player.captures < 0) {
            return false;
        }
    }
    for (const auto score : packet.state.teams) {
        if (score < 0) {
            return false;
        }
    }
    for (std::size_t i = 0; i < packet.objective_count; ++i) {
        const auto& objective = packet.objectives.at(i);
        if (objective.item_id >= 128 || objective.carrier < -1 || objective.carrier >= 32 ||
            (objective.item_type != static_cast<std::uint8_t>(ItemType::AlphaFlag) &&
             objective.item_type != static_cast<std::uint8_t>(ItemType::BravoFlag) &&
             objective.item_type != static_cast<std::uint8_t>(ItemType::PointmatchFlag) &&
             objective.item_type != static_cast<std::uint8_t>(ItemType::Bow))) {
            return false;
        }
    }
    return true;
}
} // namespace Soldank
