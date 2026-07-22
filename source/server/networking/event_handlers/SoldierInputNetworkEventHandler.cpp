module;

#include <cstdint>
#include <memory>

export module Networking.EventHandlers.SoldierInputNetworkEventHandler;

import Networking.IGameServer;

import Runtime.ServerCommandQueues;
import Sessions.PlayerSessionManager;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkPackets;
import Shared.Networking.ProtocolConversions;
import Shared.Core.Simulation.SimulationCommands;

import Extern.Spdlog;

export namespace Soldank
{
class SoldierInputNetworkEventHandler : public NetworkEventHandlerBase<SoldierInputPacket>
{
public:
    SoldierInputNetworkEventHandler(const std::shared_ptr<IGameServer>& game_server,
                                    PlayerSessionManager& player_session_manager,
                                    ServerCommandQueues& command_queues)
        : game_server_(game_server)
        , player_session_manager_(player_session_manager)
        , command_queues_(command_queues)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::SoldierInput; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(
      unsigned int sender_connection_id,
      SoldierInputPacket soldier_input_packet) override
    {
        unsigned int input_sequence_id = soldier_input_packet.input_sequence_id;
        unsigned int soldier_id = game_server_->GetSoldierIdFromConnectionId(sender_connection_id);
        PlayerInputCommand player_input_command = ProtocolConversions::ToPlayerInputCommand(
          static_cast<std::uint8_t>(soldier_id), soldier_input_packet);

        // TODO: validate arguments
        // Spdlog::info("{} Soldier pos from client: {} {}",
        //              input_sequence_id - 1,
        //              soldier_position.x,
        //              soldier_position.y);
        if (!player_session_manager_.ShouldAcceptInput(static_cast<std::uint8_t>(soldier_id),
                                                       input_sequence_id)) {
            Spdlog::warn("*************** LATE PACKET ***************************");
            return NetworkEventHandlerResult::Failure;
        }

        player_session_manager_.MarkInputReceived(static_cast<std::uint8_t>(soldier_id),
                                                  input_sequence_id);
        command_queues_.StorePendingPlayerInput(player_input_command);

        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IGameServer> game_server_;
    PlayerSessionManager& player_session_manager_;
    ServerCommandQueues& command_queues_;
};
} // namespace Soldank
