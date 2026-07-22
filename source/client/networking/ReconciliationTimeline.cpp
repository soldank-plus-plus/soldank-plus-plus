module;

#include <algorithm>
#include <cstdint>
#include <list>
#include <vector>

export module Networking.ReconciliationTimeline;

import Shared.Networking.NetworkPackets;
import Shared.Core.Utility.SerialNumber;

export namespace Soldank
{
std::vector<SoldierInputPacket> SelectReplayInputs(
  const std::list<SoldierInputPacket>& prediction_inputs,
  std::uint32_t authoritative_server_tick)
{
    std::vector<SoldierInputPacket> replay_inputs;
    for (const auto& input : prediction_inputs) {
        if (IsSerialNumberNewer(input.apply_server_tick, authoritative_server_tick)) {
            replay_inputs.push_back(input);
        }
    }

    std::ranges::sort(replay_inputs,
                      [authoritative_server_tick](const auto& first, const auto& second) {
                          if (first.apply_server_tick != second.apply_server_tick) {
                              return SerialNumberForwardDistance(authoritative_server_tick,
                                                                 first.apply_server_tick) <
                                     SerialNumberForwardDistance(authoritative_server_tick,
                                                                 second.apply_server_tick);
                          }
                          return IsSerialNumberOlder(first.client_tick, second.client_tick);
                      });
    return replay_inputs;
}
} // namespace Soldank
