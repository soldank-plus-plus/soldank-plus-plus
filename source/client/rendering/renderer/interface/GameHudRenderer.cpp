module;

#include <algorithm>
#include <format>
#include <string>

export module GameHudRenderer;

import ClientState;
import TextRenderer;
import HudSpriteRenderer;
import MinimapRenderer;
import Extern.Glm;
import Shared.Core.Types.WeaponType;

import Shared.Core.State.StateManager;
import Shared.Core.GameModes.GameModeRuntime;

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
        if (const auto* mode = game_state_manager.GetGameMode()) {
            RenderMatch(*mode, client_state);
        }
        if (client_state.world_render_options.draw_minimap) {
            minimap_.Render(game_state_manager,
                            { client_state.input.window_width, client_state.input.window_height },
                            client_state.client_soldier_id);
        }
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
                const glm::vec2 viewport(client_state.input.window_width,
                                         client_state.input.window_height);
                sprites_.Render(HudSprite::Health, { 5.0F, 439.0F }, viewport);
                sprites_.Render(HudSprite::HealthBar,
                                { 45.0F, 449.0F },
                                viewport,
                                soldier.health / game_state_manager.GetMaximumHealth());
                sprites_.Render(
                  HudSprite::VestBar, { 45.0F, 459.0F }, viewport, soldier.vest / 100.0F);
                sprites_.Render(HudSprite::Ammo, { 275.0F, 439.0F }, viewport);
                sprites_.Render(HudSprite::Jet, { 480.0F, 439.0F }, viewport);
                const auto jet_capacity = game_state_manager.GetConstMap().GetJetCount();
                sprites_.Render(
                  HudSprite::JetBar,
                  { 520.0F, 449.0F },
                  viewport,
                  jet_capacity > 0 ? static_cast<float>(soldier.jets_count) / jet_capacity : 0.0F);
                const auto& active = soldier.GetActiveWeapon();
                const auto reload_time = active.GetWeaponParameters().reload_time;
                const float ammo_fill =
                  active.IsReloading()
                    ? (reload_time > 0
                         ? 1.0F - static_cast<float>(active.GetReloadTimeCount()) / reload_time
                         : 0.0F)
                    : (active.GetCapacity() > 0
                         ? static_cast<float>(active.GetAmmoCount()) / active.GetCapacity()
                         : 0.0F);
                sprites_.Render(HudSprite::AmmoBar, { 352.0F, 449.0F }, viewport, ammo_fill);
                if (soldier.weapons.size() > 2) {
                    const auto& grenades = soldier.weapons.at(2);
                    for (unsigned int index = 0;
                         index < std::min(6U, static_cast<unsigned int>(grenades.GetAmmoCount()));
                         ++index) {
                        sprites_.Render(grenades.GetState().kind == WeaponType::ClusterGrenade
                                          ? HudSprite::ClusterGrenade
                                          : HudSprite::Grenade,
                                        { 308.0F + index * 10.0F, 462.0F },
                                        viewport);
                    }
                }
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
    void RenderMatch(const GameModeRuntime& mode, const ClientState& client)
    {
        const auto& match = mode.GetState();
        std::string status = mode.Settings().display_name;
        switch (match.phase) {
            case MatchPhase::Waiting:
                status += " - waiting for opponents";
                break;
            case MatchPhase::Running:
                status += " - round " + std::to_string(match.round_number);
                break;
            case MatchPhase::RoundEnding:
                status += " - next round soon";
                break;
            case MatchPhase::Finished:
                status += " - match finished";
                break;
        }
        text_renderer_.Render(status,
                              50.0F,
                              300.0F,
                              0.5F,
                              { 1.0F, 1.0F, 1.0F },
                              { client.input.window_width, client.input.window_height });
        std::string scores;
        if (mode.Settings().team_count > 0) {
            for (std::size_t team = 1; team <= mode.Settings().team_count; ++team) {
                scores += "Team " + std::to_string(team) + ": " +
                          std::to_string(match.teams.at(team)) + "  ";
                if (match.winning_teams.at(team)) {
                    scores += "Winner  ";
                }
            }
        } else if (client.client_soldier_id) {
            const auto id = *client.client_soldier_id;
            const auto& score = match.players.at(id);
            scores = "Points: " + std::to_string(score.points) +
                     " Kills: " + std::to_string(score.kills) +
                     " Deaths: " + std::to_string(score.deaths);
            if (match.winners.at(id)) {
                scores += " Winner";
            }
        }
        text_renderer_.Render(scores,
                              50.0F,
                              330.0F,
                              0.5F,
                              { 1.0F, 1.0F, 1.0F },
                              { client.input.window_width, client.input.window_height });
    }
    MinimapRenderer minimap_;
    HudSpriteRenderer sprites_;
    TextRenderer text_renderer_;
};
} // namespace Soldank
