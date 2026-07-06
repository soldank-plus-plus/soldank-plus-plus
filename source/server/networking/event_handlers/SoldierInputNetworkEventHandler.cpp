module;

#include "core/math/Glm.hpp"

#include <cstdint>
#include <memory>

export module Networking.EventHandlers.SoldierInputNetworkEventHandler;

import Networking.IGameServer;

import Application.ServerState;

import Shared.Core.IWorld;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkPackets;
import Shared.Networking.ProtocolConversions;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.State.Control;

import Extern.Spdlog;

export namespace Soldank
{
class SoldierInputNetworkEventHandler : public NetworkEventHandlerBase<SoldierInputPacket>
{
public:
    SoldierInputNetworkEventHandler(const std::shared_ptr<IWorld>& world,
                                    const std::shared_ptr<ServerState>& server_state,
                                    const std::shared_ptr<IGameServer>& game_server)
        : world_(world)
        , server_state_(server_state)
        , game_server_(game_server)
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
        PlayerInputCommand player_input_command =
          ProtocolConversions::ToPlayerInputCommand(
            static_cast<std::uint8_t>(soldier_id), soldier_input_packet);
        const Control& player_control = player_input_command.control;

        // TODO: validate arguments
        // Spdlog::info("{} Soldier pos from client: {} {}",
        //              input_sequence_id - 1,
        //              soldier_position.x,
        //              soldier_position.y);
        if (input_sequence_id <= server_state_->last_processed_input_id.at(soldier_id)) {
            Spdlog::warn("*************** LATE PACKET ***************************");
            return NetworkEventHandlerResult::Failure;
        }

        server_state_->last_processed_input_id.at(soldier_id) = input_sequence_id;

        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::MoveLeft, player_control.left);
        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::MoveRight, player_control.right);
        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::Jump, player_control.up);
        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::Crouch, player_control.down);
        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::ChangeWeapon, player_control.change);
        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::ThrowGrenade, player_control.throw_grenade);
        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::DropWeapon, player_control.drop);
        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::Prone, player_control.prone);

        world_->GetStateManager()->ChangeSoldierMouseMapPosition(
          soldier_id,
          player_input_command.mouse_map_position); // TODO: smooth camera handling, probably need
                                                    // to send mouse aim instead of cursor pos in
                                                    // packets
        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::UseJets, player_control.jets);
        world_->GetStateManager()->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::Fire, player_control.fire);

        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
    std::shared_ptr<ServerState> server_state_;
    std::shared_ptr<IGameServer> game_server_;
};
} // namespace Soldank
