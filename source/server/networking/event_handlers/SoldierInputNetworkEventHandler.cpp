#include "networking/event_handlers/SoldierInputNetworkEventHandler.hpp"

#include "spdlog/spdlog.h"

#include <chrono>

namespace Soldank
{
SoldierInputNetworkEventHandler::SoldierInputNetworkEventHandler(
  const std::shared_ptr<IWorld>& world,
  const std::shared_ptr<ServerState>& server_state,
  const std::shared_ptr<IGameServer>& game_server)
    : world_(world)
    , server_state_(server_state)
    , game_server_(game_server)
{
}

NetworkEventHandlerResult SoldierInputNetworkEventHandler::HandleNetworkMessageImpl(
  unsigned int sender_connection_id,
  SoldierInputPacket soldier_input_packet)
{
    unsigned int input_sequence_id = soldier_input_packet.input_sequence_id;
    unsigned int soldier_id = game_server_->GetSoldierIdFromConnectionId(sender_connection_id);
    glm::vec2 soldier_position = { soldier_input_packet.position_x,
                                   soldier_input_packet.position_y };
    glm::vec2 mouse_position = { soldier_input_packet.mouse_position_x,
                                 soldier_input_packet.mouse_position_y };
    const Control& player_control = soldier_input_packet.control;

    // TODO: validate arguments
    // spdlog::info("{} Soldier pos from client: {} {}",
    //              input_sequence_id - 1,
    //              soldier_position.x,
    //              soldier_position.y);
    if (input_sequence_id <= server_state_->last_processed_input_id.at(soldier_id)) {
        spdlog::warn("*************** LATE PACKET ***************************");
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

    world_->GetStateManager()->ChangeSoldierMousePosition(
      soldier_id,
      { mouse_position.x, mouse_position.y }); // TODO: smooth camera handling, probably need to
                                               // send mouse aim instead of cursor pos in packets
    world_->GetStateManager()->ChangeSoldierControlActionState(
      soldier_id, ControlActionType::UseJets, player_control.jets);
    world_->GetStateManager()->ChangeSoldierControlActionState(
      soldier_id, ControlActionType::Fire, player_control.fire);

    return NetworkEventHandlerResult::Success;
}
} // namespace Soldank
