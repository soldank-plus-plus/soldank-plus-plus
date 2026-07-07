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
        if (!client_state.draw_game_interface) {
            return;
        }

        game_state_manager.ForEachSoldier([&](const auto& soldier) {
            if (client_state.client_soldier_id.has_value() &&
                *client_state.client_soldier_id == soldier.id) {
                text_renderer_.Render("Health: " + std::to_string((int)soldier.health),
                                      50.0,
                                      100.0,
                                      1.0,
                                      { 1.0, 1.0, 1.0 },
                                      { client_state.window_width, client_state.window_height });
                text_renderer_.Render("Jets: " + std::to_string((int)soldier.jets_count),
                                      50.0,
                                      50.0,
                                      1.0,
                                      { 1.0, 1.0, 1.0 },
                                      { client_state.window_width, client_state.window_height });
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
                      { client_state.window_width, client_state.window_height });
                }
            });
        }

        if (game_state_manager.IsGamePaused()) {
            text_renderer_.Render("Game paused",
                                  400.0,
                                  700.0,
                                  1.0,
                                  { 0.6, 0.7, 0.4 },
                                  { client_state.window_width, client_state.window_height });
        }
    }

private:
    TextRenderer text_renderer_;
};
} // namespace Soldank
