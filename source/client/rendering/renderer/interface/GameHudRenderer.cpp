module;

#include <format>
#include <string>

export module GameHudRenderer;

import ClientState;
import TextRenderer;

import Shared.Core.State.StateManager;

export namespace Soldank
{
class GameHudRenderer
{
public:
    GameHudRenderer()
        : text_renderer_("play-regular.ttf", 48)
    {
    }

    void Render(const StateManager& game_state_manager, const ClientState& client_state)
    {
        if (client_state.network.weapon_config_mismatch) {
            text_renderer_.Render(
              "Weapon settings differ from this server.",
              50.0F,
              240.0F,
              0.5F,
              { 1.0F, 0.3F, 0.2F },
              { client_state.input.window_width, client_state.input.window_height });
            text_renderer_.Render(
              "Install its weapon files, then reconnect.",
              50.0F,
              270.0F,
              0.5F,
              { 1.0F, 0.3F, 0.2F },
              { client_state.input.window_width, client_state.input.window_height });
        }
        game_state_manager.ForEachSoldier([&](const auto& soldier) {
            if (client_state.client_soldier_id.has_value() &&
                *client_state.client_soldier_id == soldier.id) {
                text_renderer_.Render(
                  "Health: " + std::to_string((int)soldier.health),
                  50.0,
                  100.0,
                  1.0,
                  { 1.0, 1.0, 1.0 },
                  { client_state.input.window_width, client_state.input.window_height });
                const auto& weapon = soldier.GetActiveWeapon();
                std::string weapon_text = weapon.GetWeaponParameters().name + ": " +
                                          std::to_string(weapon.GetAmmoCount()) + "/" +
                                          std::to_string(weapon.GetCapacity());
                if (weapon.IsMelee()) {
                    weapon_text = weapon.GetWeaponParameters().name;
                }
                if (soldier.mounted_weapon >= 0) {
                    weapon_text = "M2";
                    game_state_manager.ForEachItem([&](const auto& item) {
                        if (item.id == soldier.mounted_weapon) {
                            weapon_text += " Heat: " + std::to_string(item.interest) + "/19";
                        }
                    });
                } else if (weapon.IsReloading()) {
                    weapon_text += " Reloading";
                } else if (weapon.GetStartUpTimeCount() <
                             weapon.GetWeaponParameters().start_up_time &&
                           weapon.GetStartUpTimeCount() > 0) {
                    weapon_text += " Starting";
                }
                text_renderer_.Render(
                  weapon_text,
                  50.0,
                  150.0,
                  0.7,
                  { 1.0, 1.0, 1.0 },
                  { client_state.input.window_width, client_state.input.window_height });
                text_renderer_.Render(
                  "Grenades: " + std::to_string(soldier.weapons.at(2).GetAmmoCount()),
                  50.0,
                  190.0,
                  0.7,
                  { 1.0, 1.0, 1.0 },
                  { client_state.input.window_width, client_state.input.window_height });
                text_renderer_.Render(
                  "Jets: " + std::to_string((int)soldier.jets_count),
                  50.0,
                  50.0,
                  1.0,
                  { 1.0, 1.0, 1.0 },
                  { client_state.input.window_width, client_state.input.window_height });
            }
        });

        if (client_state.client_soldier_id.has_value()) {
            game_state_manager.ForEachSoldier([&](const auto& soldier) {
                if (*client_state.client_soldier_id == soldier.id && soldier.dead_meat) {
                    text_renderer_.Render(
                      "Respawn timer: " +
                        std::format("{:.2f}", (float)soldier.ticks_to_respawn / 60.0F),
                      400.0,
                      100.0,
                      1.0,
                      { 1.0, 1.0, 1.0 },
                      { client_state.input.window_width, client_state.input.window_height });
                }
            });
        }

        if (game_state_manager.IsGamePaused()) {
            text_renderer_.Render(
              "Game paused",
              400.0,
              700.0,
              1.0,
              { 0.6, 0.7, 0.4 },
              { client_state.input.window_width, client_state.input.window_height });
        }
    }

private:
    TextRenderer text_renderer_;
};
} // namespace Soldank
