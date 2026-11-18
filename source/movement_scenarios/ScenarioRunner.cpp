module;
#include <cmath>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <vector>
export module MovementScenarios.Runner;
export import MovementScenarios.Definition;
import Shared.Core.World;
import Shared.Core.Map.Map;
import Shared.Core.GameModes.BuiltinGameModes;
import Shared.Core.Simulation.WorldTick;
import Shared.Core.Simulation.SimulationCommands;
export namespace Soldank::MovementScenarios
{
struct Sample
{
    unsigned int tick{};
    Input input;
    glm::vec2 aim{}, position{}, displacement{}, velocity{};
    bool grounded{};
    int direction{}, fuel{}, body_type{}, legs_type{};
    unsigned int body_frame{}, legs_frame{};
    int body_speed{}, legs_speed{};
};
class ScenarioRunner
{
public:
    explicit ScenarioRunner(const ScenarioDefinition& definition) { Reset(definition); }
    void Reset(const ScenarioDefinition& definition)
    {
        Validate(definition);
        for (const auto* asset :
             { "anims/stoi.poa", "objects/gostek.po", "weapons.ini", "weapons_realistic.ini" }) {
            if (!std::filesystem::is_regular_file(asset)) {
                throw std::runtime_error(std::string("Missing scenario asset: ") + asset +
                                         ". Run from the target output directory.");
            }
        }
        definition_ = definition;
        trace_.clear();
        failed_ = false;
        held_ = {};
        aim_ = definition.aim;
        relative_aim_ = definition.relative_aim;
        world_tick_ = 0;
        setup_ticks_ = 0;
        world_ = std::make_unique<World>();
        Map map;
        map.CreateEmptyMap();
        map.SetTextureName(definition.texture);
        map.SetBackgroundTopColor(definition.background_top);
        map.SetBackgroundBottomColor(definition.background_bottom);
        map.SetJetCount(definition.jet_capacity);
        for (const auto& polygon : definition.polygons) {
            map.AddNewPolygon(polygon);
        }
        offset_ = map.NormalizeCoordinatesForRuntime();
        map.GenerateSectors();
        auto& state = *world_->GetStateManager();
        state.OverrideMap(map);
        state.ConfigureGameMode(MakeSandbox());
        world_->CreateSoldier(definition.soldier_id);
        world_->UpdateWeaponChoices(
          definition.soldier_id, definition.primary, definition.secondary);
        state.TransformSoldier(definition.soldier_id, [&](auto& soldier) {
            soldier.appearance = definition.appearance;
            soldier.SetTeam(definition.team);
        });
        world_->SpawnSoldier(definition.soldier_id, definition.spawn + offset_);
        state.TransformSoldier(definition.soldier_id, [&](auto& soldier) {
            soldier.jets_count = definition.fuel;
            soldier.jets_count_prev = definition.fuel;
        });
        unsigned int grounded = 0;
        while (grounded < definition.settle_ticks && setup_ticks_ < definition.settle_limit) {
            Advance();
            ++setup_ticks_;
            grounded = world_->GetSoldier(definition_.soldier_id).on_ground ? grounded + 1 : 0;
        }
        if (grounded < definition.settle_ticks) {
            throw std::runtime_error("Settling timed out: " + ScenarioIdToString(definition.id));
        }
        state.TransformSoldier(definition.soldier_id, [&](auto& soldier) {
            soldier.particle.SetVelocity(definition.initial_velocity);
        });
        state.TransformSoldier(definition.soldier_id, [&](auto& soldier) {
            soldier.jets_count = definition.fuel;
            soldier.jets_count_prev = definition.fuel;
        });
        held_ = definition.initial_input;
        for (unsigned int tick = 0; tick < definition.preparation_ticks; ++tick) {
            ApplyChanges(definition.preparation_changes, tick);
            Advance();
        }
        const auto& initial = world_->GetSoldier(definition_.soldier_id);
        origin_ = initial.particle.position;
        starting_velocity_ = initial.particle.GetVelocity();
        starting_fuel_ = initial.jets_count;
        starting_direction_ = initial.direction;
    }
    bool Step()
    {
        if (Completed()) {
            return false;
        }
        if (failed_) {
            throw std::runtime_error("Failed run must be reset: " +
                                     ScenarioIdToString(definition_.id));
        }
        failed_ = true;
        const auto tick = static_cast<unsigned int>(trace_.size());
        ApplyChanges(definition_.changes, tick);
        for (const auto& change : definition_.aim_changes) {
            if (change.tick == tick) {
                aim_ = change.aim;
                relative_aim_ = change.relative;
            }
        }
        const auto aim = Advance();
        const auto& soldier = world_->GetSoldier(definition_.soldier_id);
        Sample sample{ .tick = tick,
                       .input = held_,
                       .aim = aim,
                       .position = soldier.particle.position,
                       .displacement = soldier.particle.position - origin_,
                       .velocity = soldier.particle.GetVelocity(),
                       .grounded = soldier.on_ground,
                       .direction = soldier.direction,
                       .fuel = soldier.jets_count,
                       .body_type = static_cast<int>(soldier.body_animation->GetType()),
                       .legs_type = static_cast<int>(soldier.legs_animation->GetType()),
                       .body_frame = soldier.body_animation->GetFrame(),
                       .legs_frame = soldier.legs_animation->GetFrame(),
                       .body_speed = soldier.body_animation->GetSpeed(),
                       .legs_speed = soldier.legs_animation->GetSpeed() };
        if (!std::isfinite(sample.position.x) || !std::isfinite(sample.position.y) ||
            soldier.dead_meat) {
            throw std::runtime_error("Invalid soldier state: " +
                                     ScenarioIdToString(definition_.id));
        }
        const auto bounds = world_->GetStateManager()->GetConstMap().GetBoundaries();
        const auto margin = definition_.inspection_margin;
        if (sample.position.x < bounds[2] - margin || sample.position.x > bounds[3] + margin ||
            sample.position.y < bounds[0] - margin || sample.position.y > bounds[1] + margin) {
            throw std::runtime_error("Soldier left inspection bounds: " +
                                     ScenarioIdToString(definition_.id));
        }
        trace_.push_back(sample);
        failed_ = false;
        return true;
    }
    void RunUntil(unsigned int tick)
    {
        if (tick >= definition_.duration_ticks || tick < trace_.size()) {
            throw std::invalid_argument("RunUntil requires an unexecuted tick in range");
        }
        while (trace_.size() <= tick) {
            Step();
        }
    }
    void RunToCompletion()
    {
        while (Step()) {
        }
    }
    bool Completed() const { return trace_.size() == definition_.duration_ticks; }
    const auto& GetStateManager() const { return world_->GetStateManager(); }
    const auto& GetTrace() const { return trace_; }
    const auto& GetDefinition() const { return definition_; }
    glm::vec2 GetOffset() const { return offset_; }
    glm::vec2 GetOrigin() const { return origin_; }
    unsigned int GetSetupTicks() const { return setup_ticks_; }
    glm::vec2 GetStartingVelocity() const { return starting_velocity_; }
    int GetStartingFuel() const { return starting_fuel_; }
    int GetStartingDirection() const { return starting_direction_; }

private:
    void ApplyChanges(const std::vector<InputChange>& changes, unsigned int tick)
    {
        for (const auto& change : changes) {
            if (change.tick != tick) {
                continue;
            }
            switch (change.action) {
                case ControlActionType::MoveLeft:
                    held_.left = change.held;
                    break;
                case ControlActionType::MoveRight:
                    held_.right = change.held;
                    break;
                case ControlActionType::Jump:
                    held_.jump = change.held;
                    break;
                case ControlActionType::UseJets:
                    held_.jets = change.held;
                    break;
                case ControlActionType::Crouch:
                    held_.crouch = change.held;
                    break;
                case ControlActionType::Prone:
                    held_.prone = change.held;
                    break;
                default:
                    break;
            }
        }
    }
    glm::vec2 Advance()
    {
        const auto& soldier = world_->GetSoldier(definition_.soldier_id);
        auto control = soldier.control;
        control.left = held_.left;
        control.right = held_.right;
        control.up = held_.jump;
        control.jets = held_.jets;
        control.down = held_.crouch;
        control.prone = held_.prone;
        control.fire = false;
        control.change = false;
        control.throw_grenade = false;
        control.drop = false;
        control.reload = false;
        control.flag_throw = false;
        const auto aim = relative_aim_ ? soldier.particle.position + aim_ : aim_ + offset_;
        const std::vector<PlayerInputCommand> inputs{ { .soldier_id = definition_.soldier_id,
                                                        .input_sequence_id = world_tick_,
                                                        .client_tick = world_tick_,
                                                        .apply_server_tick = world_tick_,
                                                        .control = control,
                                                        .mouse_map_position = aim } };
        const std::vector<SimulationCommand> commands;
        static_cast<void>(
          world_->Tick({ .tick = world_tick_++, .player_inputs = inputs, .commands = commands }));
        return aim;
    }
    ScenarioDefinition definition_;
    std::unique_ptr<World> world_;
    std::vector<Sample> trace_;
    Input held_;
    bool failed_{};
    glm::vec2 aim_{};
    bool relative_aim_{};
    glm::vec2 offset_{}, origin_{}, starting_velocity_{};
    int starting_fuel_{}, starting_direction_{};
    unsigned int world_tick_{}, setup_ticks_{};
};
} // namespace Soldank::MovementScenarios
