module;

#include <GLFW/glfw3.h>

#include <cstdint>
#include <memory>
#include <utility>

export module Gameplay.PlayerController;

import Extern.Glm;

import Application.Input.ApplicationInputController;
import Application.Input.PlatformInput;
import Application.Input.PlayerInput;
import Application.Window;
import ClientState;

import Shared.Core.Entities.Soldier;
import Shared.Core.IWorld;
import Shared.Core.State.Control;
import Shared.Core.State.StateManager;

export namespace Soldank
{
class PlayerController
{
public:
    PlayerController(IWorld& world,
                     Window& window,
                     std::shared_ptr<ClientState> client_state,
                     ApplicationInputController& input_controller)
        : world_(world)
        , window_(window)
        , client_state_(std::move(client_state))
        , input_controller_(input_controller)
    {
    }

    void Update(std::uint8_t soldier_id)
    {
        const PlatformInput& input = window_.GetPlatformInput();
        glm::vec2 mouse_position = { input.GetX(), input.GetY() };
        client_state_->input.mouse_screen_position = mouse_position;

        float ratio_x = client_state_->input.window_width / client_state_->camera.view.GetWidth();
        float ratio_y = client_state_->input.window_height / client_state_->camera.view.GetHeight();
        mouse_position.x /= ratio_x;
        mouse_position.y /= ratio_y;
        client_state_->camera.previous_position = client_state_->camera.position;

        const Soldier& soldier = world_.GetStateManager()->GetSoldier(soldier_id);
        if (!soldier.active) {
            client_state_->camera.position = { 0.0F, 0.0F };
            return;
        }

        if (!soldier.dead_meat) {
            UpdateControlActions(soldier_id, input);
        }

        world_.UpdateWeaponChoices(soldier_id,
                                   client_state_->primary_weapon_type_choice,
                                   client_state_->secondary_weapon_type_choice);
        UpdateCamera(soldier_id, mouse_position);
        world_.GetStateManager()->ChangeSoldierMouseMapPosition(
          soldier_id, input_controller_.GetMouseMapPosition());
    }

private:
    void UpdateControlActions(std::uint8_t soldier_id, const PlatformInput& input)
    {
        const std::shared_ptr<StateManager>& state_manager = world_.GetStateManager();
        state_manager->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::MoveLeft, input.Key(GLFW_KEY_A));
        state_manager->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::MoveRight, input.Key(GLFW_KEY_D));
        state_manager->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::Jump, input.Key(GLFW_KEY_W));
        state_manager->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::Crouch, input.Key(GLFW_KEY_S));
        state_manager->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::ChangeWeapon, input.Key(GLFW_KEY_Q));
        state_manager->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::ThrowGrenade, input.Key(GLFW_KEY_E));
        state_manager->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::DropWeapon, input.Key(GLFW_KEY_F));
        state_manager->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::Prone, input.Key(GLFW_KEY_X));
        state_manager->ChangeSoldierControlActionState(
          soldier_id, ControlActionType::ThrowFlag, input.Key(GLFW_KEY_W) && input.Key(GLFW_KEY_S));
        state_manager->ChangeSoldierControlActionState(soldier_id,
                                                       ControlActionType::UseJets,
                                                       input.Button(GLFW_MOUSE_BUTTON_RIGHT) ||
                                                         input.Key(GLFW_KEY_K));
        state_manager->ChangeSoldierControlActionState(soldier_id,
                                                       ControlActionType::Fire,
                                                       input.Button(GLFW_MOUSE_BUTTON_LEFT) ||
                                                         input.Key(GLFW_KEY_I));

        state_manager->SoldierControlApply(
          soldier_id, [this](const Soldier& soldier, Control& control) {
              PlayerInput::UpdatePlayerSoldierControlCollisions(soldier, control, client_state_);
          });
    }

    void UpdateCamera(std::uint8_t soldier_id, glm::vec2 mouse_position)
    {
        client_state_->camera.previous_position = client_state_->camera.position;
        float width = client_state_->camera.view.GetWidth();
        float height = client_state_->camera.view.GetHeight();

        if (client_state_->camera.smooth) {
            glm::vec2 mouse_offset;
            mouse_offset.x =
              (mouse_position.x - width / 2.0F) / 7.0F *
              ((2.0F * 640.0F / width - 1.0F) + (width - 640.0F) / width * 0.0F / 6.8F);
            mouse_offset.y = (mouse_position.y - height / 2.0F) / 7.0F;

            glm::vec2 camera_position = client_state_->camera.position;
            const glm::vec2& soldier_position = world_.GetSoldier(soldier_id).particle.position;
            glm::vec2 soldier_camera_position = { soldier_position.x, -soldier_position.y };
            camera_position += (soldier_camera_position - camera_position) * 0.14F;
            camera_position += mouse_offset;
            client_state_->camera.position = camera_position;
        } else {
            const glm::vec2& soldier_position = world_.GetSoldier(soldier_id).particle.position;
            client_state_->camera.position.x = soldier_position.x + mouse_position.x - width / 2.0F;
            client_state_->camera.position.y =
              -soldier_position.y + mouse_position.y - height / 2.0F;
        }
    }

    IWorld& world_;
    Window& window_;
    std::shared_ptr<ClientState> client_state_;
    ApplicationInputController& input_controller_;
};
} // namespace Soldank
