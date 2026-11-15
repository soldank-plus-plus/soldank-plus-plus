module;

#include <algorithm>
#include <cstddef>
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>
#include <cmath>

export module Shared.Core.GameModes.WorldGameModeAdapter;

import Extern.Glm;
import Shared.Core.GameModes.GameModeRuntime;
import Shared.Core.State.StateManager;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.TeamType;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Math.Calc;

export namespace Soldank
{
class WorldGameModeAdapter
{
public:
    ModeWorldView View(const StateManager& state) const
    {
        ModeWorldView view{ .tick = state.GetGameTick(), .objectives = {} };
        state.ForEachSoldier([&](const Soldier& soldier) {
            view.players.at(soldier.id) = {
                .active = true,
                .alive = soldier.IsAlive(),
                .team = soldier.team,
                .generation = state.GetParticipantGeneration(soldier.id),
                .bow = IsBow(soldier.GetActiveWeapon().GetWeaponParameters().kind),
                .carrying_flag = soldier.is_holding_flags,
            };
        });
        for (std::size_t index = 0; index < objectives_.size(); ++index) {
            const auto& binding = objectives_.at(index);
            if (!binding.item) {
                continue;
            }
            state.ForEachItem([&](const Item& item) {
                if (item.id == *binding.item) {
                    ObjectiveView objective{
                        .definition_index = index,
                        .item_id = item.id,
                        .carrier = std::nullopt,
                    };
                    if (item.holding_soldier_id >= 0) {
                        objective.carrier = static_cast<std::uint8_t>(item.holding_soldier_id);
                    }
                    objective.home =
                      Calc::Distance(item.skeleton->GetPos(1), binding.home) <
                      state.GetGameMode()->Settings().objectives.at(index).home_radius;
                    view.objectives.push_back(objective);
                }
            });
        }
        return view;
    }
    void BeginTick(StateManager& state)
    {
        auto* mode = state.GetGameMode();
        if ((mode == nullptr) || !state.HasAuthoritativeProjectiles() || mode->Settings().sandbox) {
            return;
        }
        if (epoch_ != mode->GetState().epoch) {
            Initialize(state);
        }
        auto const view = View(state);
        mode->ObserveRoster(view);
        SynchronizeRoster(state, view);
        if (mode->RestartDue(view.tick)) {
            RestartRound(state);
        }
        RecoverObjectives(state);
        if (mode->Settings().regenerate_bow && view.tick % 3U == 0 &&
            mode->GetState().phase == MatchPhase::Running) {
            state.TransformSoldiers([&](Soldier& soldier) {
                if (soldier.IsAlive() &&
                    IsBow(soldier.GetActiveWeapon().GetWeaponParameters().kind)) {
                    soldier.health = std::min(state.GetMaximumHealth(), soldier.health + 1.0F);
                }
            });
        }
    }
    void EndTick(StateManager& state)
    {
        auto* mode = state.GetGameMode();
        if ((mode == nullptr) || !state.HasAuthoritativeProjectiles() || mode->Settings().sandbox) {
            return;
        }
        auto view = View(state);
        if (mode->Settings().captures && mode->GetState().phase == MatchPhase::Running) {
            EvaluateCaptures(state, view);
        }
        view = View(state);
        for (const auto& effect : mode->EndTick(view)) {
            if (effect.kind == ModeActionKind::ResetObjective) {
                ResetObjective(state, effect.target);
            }
        }
    }
    static bool CanSimulate(const StateManager& state, std::uint8_t player)
    {
        const auto* mode = state.GetGameMode();
        if ((mode == nullptr) || !state.HasAuthoritativeProjectiles() || mode->Settings().sandbox) {
            return true;
        }
        return mode->GetState().phase == MatchPhase::Running && mode->IsRoundParticipant(player);
    }
    static bool CanRespawn(const StateManager& state, std::uint8_t player)
    {
        const auto* mode = state.GetGameMode();
        return CanSimulate(state, player) &&
               ((mode == nullptr) || mode->Settings().respawn.kind != RespawnKind::RoundOnly);
    }

private:
    void EvaluateCaptures(StateManager& state, const ModeWorldView& view)
    {
        auto& mode = *state.GetGameMode();
        for (const auto& objective : view.objectives) {
            if (!objective.carrier) {
                continue;
            }
            const auto& spec = mode.Settings().objectives.at(objective.definition_index);
            const auto carrier = *objective.carrier;
            const auto team = view.players.at(carrier).team;
            if (!view.players.at(carrier).alive || team == spec.team) {
                continue;
            }
            const auto item_id = objectives_.at(objective.definition_index).item;
            if (!item_id) {
                continue;
            }
            const auto& item = state.GetItem(*item_id);
            for (const auto& home : view.objectives) {
                const auto& home_spec = mode.Settings().objectives.at(home.definition_index);
                if (home_spec.team == team && home.home && !home.carrier &&
                    Calc::Distance(item.skeleton->GetPos(1),
                                   objectives_.at(home.definition_index).home) <
                      home_spec.capture_radius) {
                    mode.OnCapture({ .player = carrier, .objective = objective.definition_index },
                                   view);
                    ResetObjective(state, objective.definition_index);
                    break;
                }
            }
        }
    }
    void SynchronizeRoster(StateManager& state, const ModeWorldView& view)
    {
        const auto* mode = state.GetGameMode();
        state.TransformSoldiers([&](Soldier& soldier) {
            soldier.ApplyTeamShirtColor();
            const auto& previous = roster_.at(soldier.id);
            const auto& current = view.players.at(soldier.id);
            const bool changed_team =
              previous.active &&
              (previous.team != current.team || previous.generation != current.generation);
            const bool waiting_for_round =
              mode->Settings().rounds.enabled && !mode->IsRoundParticipant(soldier.id);
            if (changed_team || waiting_for_round) {
                state.ReleaseSoldierFlags(soldier.id);
                soldier.dead_meat = true;
                soldier.death_committed = true;
                soldier.health = 0;
                soldier.ticks_to_respawn = static_cast<int>(mode->RespawnDelay(view));
            }
        });
        roster_ = view.players;
    }
    void RestartRound(StateManager& state)
    {
        auto* mode = state.GetGameMode();
        if (mode->Settings().rounds.clear_weapons) {
            state.TransformItems([](Item& item) {
                if (IsItemTypeWeapon(item.style) && item.style != ItemType::Bow) {
                    item.active = false;
                }
            });
        }
        for (std::size_t i = 0; i < objectives_.size(); ++i) {
            if (mode->Settings().objectives.at(i).team != TeamType::None) {
                ResetObjective(state, i);
            }
        }
        state.TransformSoldiers([&](Soldier& soldier) {
            state.ReleaseSoldierFlags(soldier.id);
            if (!soldier.IsSpectator()) {
                state.SpawnSoldier(soldier.id, std::nullopt);
            }
        });
        mode->RestartRound(View(state));
    }
    struct ObjectiveBinding
    {
        glm::vec2 home{};
        std::optional<std::uint8_t> item;
        std::optional<std::uint32_t> dropped_at;
    };
    static PMSSpawnPointType SpawnType(ItemType type)
    {
        switch (type) {
            case ItemType::AlphaFlag:
                return PMSSpawnPointType::AlphaFlag;
            case ItemType::BravoFlag:
                return PMSSpawnPointType::BravoFlag;
            case ItemType::PointmatchFlag:
                return PMSSpawnPointType::YellowFlag;
            case ItemType::Bow:
                return PMSSpawnPointType::RamboBow;
            default:
                throw std::invalid_argument("Unsupported objective item type");
        }
    }
    void Initialize(StateManager& state)
    {
        auto const& mode = *state.GetGameMode();
        objectives_.clear();
        for (const auto& spec : mode.Settings().objectives) {
            std::vector<glm::vec2> homes;
            for (const auto& spawn : state.GetConstMap().GetSpawnPoints()) {
                if (spawn.active != 0 && spawn.type == SpawnType(spec.item_type)) {
                    homes.emplace_back(spawn.x, spawn.y);
                }
            }
            if (homes.empty()) {
                throw std::invalid_argument("Missing objective spawn: " + spec.id);
            }
            const auto selected =
              state.GetRandomInt(0, static_cast<unsigned int>(homes.size() - 1));
            objectives_.push_back(
              { .home = homes.at(selected), .item = std::nullopt, .dropped_at = std::nullopt });
        }
        for (std::size_t i = 0; i < objectives_.size(); ++i) {
            ResetObjective(state, i);
        }
        epoch_ = mode.GetState().epoch;
        roster_ = {};
    }
    void ResetObjective(StateManager& state, std::size_t index)
    {
        auto& binding = objectives_.at(index);
        binding.dropped_at.reset();
        const auto& spec = state.GetGameMode()->Settings().objectives.at(index);
        if (binding.item && state.GetItem(*binding.item).active &&
            state.GetItem(*binding.item).style != spec.item_type) {
            binding.item.reset();
        }
        if (binding.item) {
            state.TransformItems([&](Item& item) {
                if (item.id == *binding.item) {
                    if (item.holding_soldier_id >= 0) {
                        state.GetSoldier(static_cast<std::uint8_t>(item.holding_soldier_id))
                          .is_holding_flags = false;
                    }
                    item.active = false;
                }
            });
        }
        const auto& item =
          state.CreateItem(binding.home, 255, spec.item_type, nullptr, binding.item);
        binding.item = item.id;
        state.TransformItems([&](Item& mutable_item) {
            if (mutable_item.id == item.id) {
                mutable_item.return_requested = false;
                mutable_item.time_out = static_cast<std::int32_t>(spec.return_ticks);
                mutable_item.in_base = true;
                mutable_item.static_type = spec.fixed;
                mutable_item.collide_with_bullets = !spec.fixed;
            }
        });
    }
    void RecoverObjectives(StateManager& state)
    {
        const auto& specs = state.GetGameMode()->Settings().objectives;
        for (std::size_t index = 0; index < objectives_.size(); ++index) {
            const auto& spec = specs.at(index);
            bool held_bow = false;
            if (spec.item_type == ItemType::Bow) {
                state.ForEachSoldier([&](const Soldier& soldier) {
                    held_bow =
                      held_bow || (soldier.IsAlive() &&
                                   IsBow(soldier.GetActiveWeapon().GetWeaponParameters().kind));
                });
            }
            bool found = false;
            state.TransformItems([&](Item& item) {
                if (item.style == spec.item_type) {
                    if (held_bow || found) {
                        item.active = false;
                    } else {
                        found = true;
                        objectives_.at(index).item = item.id;
                        if (spec.fixed) {
                            item.static_type = true;
                            item.time_out = 1500;
                        }
                    }
                }
            });
            if (!found && !held_bow) {
                ResetObjective(state, index);
            } else if (found) {
                UpdateReturnTimer(state, index);
            }
        }
    }
    static bool IsInvalidPosition(const glm::vec2& position, const glm::vec2& home)
    {
        return !std::isfinite(position.x) || !std::isfinite(position.y) ||
               Calc::Distance(position, home) > 100000.0F;
    }
    void UpdateReturnTimer(StateManager& state, std::size_t index)
    {
        const auto& spec = state.GetGameMode()->Settings().objectives.at(index);
        const auto item_id = objectives_.at(index).item;
        if (!item_id) {
            return;
        }
        const auto& item = state.GetItem(*item_id);
        auto& dropped_at = objectives_.at(index).dropped_at;
        const auto position = item.skeleton->GetPos(1);
        const bool home = spec.team != TeamType::None &&
                          Calc::Distance(position, objectives_.at(index).home) < spec.home_radius;
        if (item.holding_soldier_id >= 0 || home || spec.fixed) {
            dropped_at.reset();
        } else if (!dropped_at) {
            dropped_at = state.GetGameTick();
        }
        if (item.return_requested || IsInvalidPosition(position, objectives_.at(index).home) ||
            (dropped_at && state.GetGameTick() - *dropped_at >= spec.return_ticks)) {
            ResetObjective(state, index);
        }
    }
    std::uint32_t epoch_{};
    std::vector<ObjectiveBinding> objectives_;
    std::array<ModeParticipant, MODE_PLAYERS> roster_{};
};
} // namespace Soldank
