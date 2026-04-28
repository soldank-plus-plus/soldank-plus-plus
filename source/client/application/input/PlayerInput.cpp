module;

#include <memory>

export module Application.Input.PlayerInput;

import ClientState;

import Shared.Core.Animations;
import Shared.Core.Entities.Soldier;
import Shared.Core.State.Control;

export namespace Soldank::PlayerInput
{
void UpdatePlayerSoldierControlCollisions(const Soldier& soldier,
                                          Control& soldier_control,
                                          const std::shared_ptr<ClientState>& client_state)
{
    bool was_holding_left = soldier_control.left;
    bool was_holding_right = soldier_control.right;

    if (soldier_control.left && soldier_control.right) {
        if (client_state->player_was_jumping) {
            // If jumping, keep going in the old direction
            if (client_state->player_was_running_left) {
                soldier_control.right = false;
            } else {
                soldier_control.left = false;
            }
        } else {
            // If not jumping, instead go in the new direction
            if (client_state->player_was_running_left) {
                soldier_control.left = false;
            } else {
                soldier_control.right = false;
            }
        }

        if (soldier.legs_animation->GetType() == AnimationType::JumpSide) {
            soldier_control.left = true;
            soldier_control.right = true;
        }
    } else {
        client_state->player_was_running_left = soldier_control.left;
        client_state->player_was_jumping = soldier_control.up;
    }

    client_state->player_was_holding_left = was_holding_left;
    client_state->player_was_holding_right = was_holding_right;
}
} // namespace Soldank::PlayerInput
