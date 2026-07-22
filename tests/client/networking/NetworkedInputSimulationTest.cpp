#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

import Networking.InputApplicationTimeline;
import Networking.ReconciliationTimeline;
import Runtime.ServerCommandQueues;
import Sessions.PlayerSessionManager;

import Shared.Core.State.Control;
import Shared.Core.Utility.SerialNumber;
import Shared.Networking.NetworkPackets;
import Shared.Networking.ProtocolConversions;

using namespace Soldank;

namespace
{
enum class InputScenario
{
    Idle,
    ContinuousMovement,
    RapidDirectionChanges,
    JumpAndLanding,
    Jets,
    StanceTransitions,
    TerrainEdges,
};

struct DeterministicMovementState
{
    int horizontal_position = 0;
    int vertical_position = 0;
    int horizontal_velocity = 0;
    int vertical_velocity = 0;
    int stance = 0;
    int jet_energy = 120;
    bool on_ground = true;

    bool operator==(const DeterministicMovementState&) const = default;
};

void SimulateMovementTick(DeterministicMovementState& state, const Control& control)
{
    const int horizontal_input = static_cast<int>(control.right) - static_cast<int>(control.left);
    if (horizontal_input != 0) {
        state.horizontal_velocity = std::clamp(state.horizontal_velocity + horizontal_input, -4, 4);
    } else if (state.horizontal_velocity > 0) {
        state.horizontal_velocity--;
    } else if (state.horizontal_velocity < 0) {
        state.horizontal_velocity++;
    }

    state.stance = control.prone ? 2 : (control.down ? 1 : 0);
    if (control.up && !control.was_jumping && state.on_ground) {
        state.vertical_velocity = -6;
        state.on_ground = false;
    }
    if (control.jets && state.jet_energy > 0) {
        state.vertical_velocity = std::max(state.vertical_velocity - 1, -7);
        state.jet_energy--;
        state.on_ground = false;
    } else if (!control.jets && state.jet_energy < 120) {
        state.jet_energy++;
    }

    if (!state.on_ground) {
        state.vertical_velocity++;
    }
    state.horizontal_position += state.horizontal_velocity;
    state.vertical_position += state.vertical_velocity;
    int terrain_phase = state.horizontal_position % 80;
    if (terrain_phase < 0) {
        terrain_phase += 80;
    }
    const int ground_height =
      terrain_phase < 40 ? -(terrain_phase / 4) : -((80 - terrain_phase) / 4);
    if (state.on_ground || state.vertical_position >= ground_height) {
        state.vertical_position = ground_height;
        state.vertical_velocity = 0;
        state.on_ground = true;
    }
}

bool IsJumpPressed(InputScenario scenario, std::uint32_t input_tick)
{
    return scenario == InputScenario::JumpAndLanding && input_tick % 45U == 5U;
}

Control MakeControl(InputScenario scenario, std::uint32_t input_tick)
{
    Control control{};
    switch (scenario) {
        case InputScenario::Idle:
            break;
        case InputScenario::ContinuousMovement:
            control.right = true;
            break;
        case InputScenario::RapidDirectionChanges:
            control.right = input_tick % 16U < 8U;
            control.left = !control.right;
            break;
        case InputScenario::JumpAndLanding:
            control.right = true;
            control.up = IsJumpPressed(scenario, input_tick);
            control.was_jumping = input_tick > 0 && IsJumpPressed(scenario, input_tick - 1U);
            break;
        case InputScenario::Jets:
            control.right = input_tick % 60U < 30U;
            control.left = !control.right;
            control.jets = input_tick % 50U >= 10U && input_tick % 50U < 24U;
            break;
        case InputScenario::StanceTransitions:
            control.right = input_tick % 40U < 20U;
            control.down = input_tick % 60U >= 20U && input_tick % 60U < 30U;
            control.prone = input_tick % 60U >= 30U && input_tick % 60U < 40U;
            break;
        case InputScenario::TerrainEdges:
            control.right = true;
            control.up = input_tick % 70U == 35U;
            control.was_jumping = input_tick > 0 && (input_tick - 1U) % 70U == 35U;
            break;
    }
    control.was_running_left = input_tick > 0 && scenario == InputScenario::RapidDirectionChanges &&
                               (input_tick - 1U) % 16U >= 8U;
    return control;
}

struct LatencyPhase
{
    std::uint64_t start_step;
    std::uint16_t round_trip_time_milliseconds;
};

struct AuthoritativeSnapshot
{
    std::uint32_t server_tick;
    DeterministicMovementState state;
};

struct PredictedSnapshot
{
    std::uint32_t apply_server_tick;
    DeterministicMovementState state;
};

template<typename Payload>
struct InFlightMessage
{
    std::uint64_t delivery_step;
    Payload payload;
};

struct SimulationMetrics
{
    std::size_t accurate_snapshots = 0;
    std::size_t missing_snapshots = 0;
    std::size_t corrections = 0;
    std::size_t resynchronization_wait_snapshots = 0;
    std::size_t resynchronizations = 0;
    std::size_t ignored_old_snapshots = 0;
    std::size_t authoritative_past_replays = 0;
    std::size_t selected_late_inputs = 0;
    std::size_t superseded_inputs = 0;
    std::size_t consecutive_accurate_snapshots = 0;
};

class NetworkedInputSimulation
{
public:
    explicit NetworkedInputSimulation(std::vector<LatencyPhase> latency_phases,
                                      bool enable_delivery_faults = false,
                                      std::uint64_t delivery_fault_end_step = 0,
                                      std::uint32_t initial_client_tick = 0,
                                      std::uint32_t initial_server_tick = 100,
                                      std::uint32_t initial_input_sequence_id = 1)
        : latency_phases_(std::move(latency_phases))
        , enable_delivery_faults_(enable_delivery_faults)
        , delivery_fault_end_step_(delivery_fault_end_step)
        , client_tick_(initial_client_tick)
        , server_tick_(initial_server_tick)
        , next_input_sequence_id_(initial_input_sequence_id)
    {
        if (initial_input_sequence_id != 1) {
            player_sessions_.MarkInputReceived(PLAYER_ID, initial_input_sequence_id - 1U);
        }
    }

    SimulationMetrics Run(std::uint64_t steps, InputScenario scenario)
    {
        for (std::uint64_t step = 0; step < steps; step++) {
            current_step_ = step;
            DeliverInputs();
            SimulateServerTick();
            DeliverSnapshots();
            SimulateClientTick(scenario);
        }
        return metrics_;
    }

private:
    struct LatencyTicks
    {
        std::uint32_t outgoing;
        std::uint32_t incoming;
    };

    std::uint16_t CurrentRoundTripTime() const
    {
        std::uint16_t round_trip_time = latency_phases_.front().round_trip_time_milliseconds;
        for (const auto& phase : latency_phases_) {
            if (phase.start_step > current_step_) {
                break;
            }
            round_trip_time = phase.round_trip_time_milliseconds;
        }
        return round_trip_time;
    }

    LatencyTicks CurrentLatencyTicks() const
    {
        constexpr std::uint32_t TICKS_PER_SECOND = 60;
        constexpr std::uint32_t MILLISECONDS_PER_SECOND = 1000;
        const std::uint32_t round_trip_ticks =
          (static_cast<std::uint32_t>(CurrentRoundTripTime()) * TICKS_PER_SECOND +
           MILLISECONDS_PER_SECOND - 1U) /
          MILLISECONDS_PER_SECOND;
        return { .outgoing = round_trip_ticks / 2U,
                 .incoming = round_trip_ticks - round_trip_ticks / 2U };
    }

    bool DeliveryFaultsEnabled() const
    {
        return enable_delivery_faults_ && current_step_ < delivery_fault_end_step_;
    }

    void QueueInputPacket(const SoldierInputPacket& packet)
    {
        if (DeliveryFaultsEnabled() && packet.input_sequence_id % 17U == 0U) {
            return;
        }

        std::uint64_t delivery_step = current_step_ + CurrentLatencyTicks().outgoing;
        if (DeliveryFaultsEnabled() && packet.input_sequence_id % 19U == 0U) {
            delivery_step += 3U;
        }
        in_flight_inputs_.push_back({ delivery_step, packet });
        if (DeliveryFaultsEnabled() && packet.input_sequence_id % 23U == 0U) {
            in_flight_inputs_.push_back({ delivery_step + 1U, packet });
        }
    }

    void QueueAuthoritativeSnapshot(const AuthoritativeSnapshot& snapshot)
    {
        if (DeliveryFaultsEnabled() && snapshot.server_tick % 13U == 0U) {
            return;
        }

        std::uint64_t delivery_step = current_step_ + CurrentLatencyTicks().incoming;
        if (DeliveryFaultsEnabled() && snapshot.server_tick % 29U == 0U) {
            delivery_step += 2U;
        }
        in_flight_snapshots_.push_back({ delivery_step, snapshot });
        if (DeliveryFaultsEnabled() && snapshot.server_tick % 31U == 0U) {
            in_flight_snapshots_.push_back({ delivery_step + 1U, snapshot });
        }
    }

    void DeliverInputs()
    {
        std::erase_if(in_flight_inputs_, [&](const auto& message) {
            if (message.delivery_step > current_step_) {
                return false;
            }
            const SoldierInputPacket& packet = message.payload;
            if (player_sessions_.ShouldAcceptInput(PLAYER_ID, packet.input_sequence_id)) {
                player_sessions_.MarkInputReceived(PLAYER_ID, packet.input_sequence_id);
                command_queues_.StorePendingPlayerInput(
                  ProtocolConversions::ToPlayerInputCommand(PLAYER_ID, packet));
            }
            return true;
        });
    }

    void SimulateServerTick()
    {
        const auto selected_inputs = command_queues_.SelectPlayerInputsForSimulation(server_tick_);
#ifndef NDEBUG
        const auto debug_stats = command_queues_.GetInputDebugStats();
        metrics_.superseded_inputs += debug_stats.superseded_input_count;
        command_queues_.ResetInputDebugStats();
#endif
        if (!selected_inputs.empty()) {
            const auto& selected_input = selected_inputs.front();
            if (IsSerialNumberOlder(selected_input.apply_server_tick, server_tick_)) {
                metrics_.selected_late_inputs++;
            }
            server_control_ = selected_input.control;
            player_sessions_.MarkInputApplied(PLAYER_ID, selected_input.input_sequence_id);
        }
        SimulateMovementTick(server_state_, server_control_);
        QueueAuthoritativeSnapshot({ .server_tick = server_tick_, .state = server_state_ });
        server_tick_++;
    }

    void DeliverSnapshots()
    {
        std::erase_if(in_flight_snapshots_, [&](const auto& message) {
            if (message.delivery_step > current_step_) {
                return false;
            }
            ProcessAuthoritativeSnapshot(message.payload);
            return true;
        });
    }

    void ProcessAuthoritativeSnapshot(const AuthoritativeSnapshot& snapshot)
    {
        if (latest_authoritative_tick_.has_value() &&
            !IsSerialNumberNewer(snapshot.server_tick, *latest_authoritative_tick_)) {
            metrics_.ignored_old_snapshots++;
            return;
        }
        latest_authoritative_tick_ = snapshot.server_tick;
        server_tick_offset_ = SerialNumberSignedDistance(client_tick_, snapshot.server_tick);

        if (reconciliation_resume_server_tick_.has_value() &&
            IsSerialNumberOlder(snapshot.server_tick, *reconciliation_resume_server_tick_)) {
            metrics_.resynchronization_wait_snapshots++;
            PrunePredictionHistory(snapshot.server_tick);
            return;
        }
        if (reconciliation_resume_server_tick_.has_value()) {
            reconciliation_resume_server_tick_.reset();
        }

        const auto predicted_snapshot = std::find_if(
          predicted_snapshots_.begin(), predicted_snapshots_.end(), [&](const auto& prediction) {
              return prediction.apply_server_tick == snapshot.server_tick;
          });
        if (predicted_snapshot == predicted_snapshots_.end()) {
            metrics_.missing_snapshots++;
            metrics_.consecutive_accurate_snapshots = 0;
            RestoreAndReplay(snapshot);
        } else if (predicted_snapshot->state != snapshot.state) {
            metrics_.corrections++;
            metrics_.consecutive_accurate_snapshots = 0;
            RestoreAndReplay(snapshot);
        } else {
            metrics_.accurate_snapshots++;
            metrics_.consecutive_accurate_snapshots++;
        }
        PrunePredictionHistory(snapshot.server_tick);
    }

    void RestoreAndReplay(const AuthoritativeSnapshot& snapshot)
    {
        client_state_ = snapshot.state;
        predicted_snapshots_.remove_if([&](const auto& prediction) {
            return IsSerialNumberNewer(prediction.apply_server_tick, snapshot.server_tick);
        });

        const auto replay_inputs = SelectReplayInputs(prediction_inputs_, snapshot.server_tick);
        for (const auto& input : replay_inputs) {
            if (!IsSerialNumberNewer(input.apply_server_tick, snapshot.server_tick)) {
                metrics_.authoritative_past_replays++;
            }
            SimulateMovementTick(client_state_, input.control);
            predicted_snapshots_.push_back(
              { .apply_server_tick = input.apply_server_tick, .state = client_state_ });
        }
    }

    void PrunePredictionHistory(std::uint32_t authoritative_server_tick)
    {
        constexpr std::uint32_t HISTORY_GRACE_TICKS = 4;
        const std::uint32_t oldest_tick_to_keep = authoritative_server_tick - HISTORY_GRACE_TICKS;
        prediction_inputs_.remove_if([&](const auto& input) {
            return IsSerialNumberOlder(input.apply_server_tick, oldest_tick_to_keep);
        });
        predicted_snapshots_.remove_if([&](const auto& snapshot) {
            return IsSerialNumberOlder(snapshot.apply_server_tick, oldest_tick_to_keep);
        });
    }

    void SimulateClientTick(InputScenario scenario)
    {
        target_input_delay_ticks_ =
          std::max(target_input_delay_ticks_, CalculateInputDelayTicks(CurrentRoundTripTime()));
        const auto apply_server_tick =
          timeline_.Schedule(client_tick_, server_tick_offset_, target_input_delay_ticks_);
        if (apply_server_tick.has_value()) {
            if (timeline_.WasLastScheduleResynchronized()) {
                prediction_inputs_.clear();
                predicted_snapshots_.clear();
                reconciliation_resume_server_tick_ = *apply_server_tick;
                metrics_.resynchronizations++;
            }

            const Control control = MakeControl(scenario, input_tick_);
            SoldierInputPacket packet{
                .input_sequence_id = next_input_sequence_id_,
                .client_tick = client_tick_,
                .apply_server_tick = *apply_server_tick,
                .position_x = 0.0F,
                .position_y = 0.0F,
                .mouse_map_position_x = 0.0F,
                .mouse_map_position_y = 0.0F,
                .control = control,
            };
            next_input_sequence_id_++;
            input_tick_++;
            prediction_inputs_.push_back(packet);
            QueueInputPacket(packet);

            SimulateMovementTick(client_state_, control);
            predicted_snapshots_.push_back(
              { .apply_server_tick = *apply_server_tick, .state = client_state_ });
        }
        client_tick_++;
    }

    static constexpr std::uint8_t PLAYER_ID = 1;

    std::vector<LatencyPhase> latency_phases_;
    bool enable_delivery_faults_;
    std::uint64_t delivery_fault_end_step_;
    std::uint64_t current_step_ = 0;
    std::uint32_t client_tick_;
    std::uint32_t server_tick_;
    std::uint32_t next_input_sequence_id_;
    std::uint32_t input_tick_ = 0;
    std::uint32_t target_input_delay_ticks_ = 5;
    std::optional<std::int64_t> server_tick_offset_;
    std::optional<std::uint32_t> reconciliation_resume_server_tick_;
    std::optional<std::uint32_t> latest_authoritative_tick_;

    InputApplicationTimeline timeline_;
    ServerCommandQueues command_queues_;
    PlayerSessionManager player_sessions_;
    Control server_control_{};
    DeterministicMovementState client_state_;
    DeterministicMovementState server_state_;
    std::list<SoldierInputPacket> prediction_inputs_;
    std::list<PredictedSnapshot> predicted_snapshots_;
    std::vector<InFlightMessage<SoldierInputPacket>> in_flight_inputs_;
    std::vector<InFlightMessage<AuthoritativeSnapshot>> in_flight_snapshots_;
    SimulationMetrics metrics_;
};

std::string_view ScenarioName(InputScenario scenario)
{
    switch (scenario) {
        case InputScenario::Idle:
            return "idle";
        case InputScenario::ContinuousMovement:
            return "continuous movement";
        case InputScenario::RapidDirectionChanges:
            return "rapid direction changes";
        case InputScenario::JumpAndLanding:
            return "jump and landing";
        case InputScenario::Jets:
            return "jets";
        case InputScenario::StanceTransitions:
            return "stance transitions";
        case InputScenario::TerrainEdges:
            return "terrain edges";
    }
    return "unknown";
}
} // namespace

TEST(NetworkedInputSimulationTest, DeterministicMovementMatchesAtSupportedStaticLatencies)
{
    constexpr std::array SCENARIOS{
        InputScenario::Idle,
        InputScenario::ContinuousMovement,
        InputScenario::RapidDirectionChanges,
        InputScenario::JumpAndLanding,
        InputScenario::Jets,
        InputScenario::StanceTransitions,
        InputScenario::TerrainEdges,
    };
    constexpr std::array<std::uint16_t, 3> ROUND_TRIP_TIMES{ 0, 50, 150 };

    for (const auto scenario : SCENARIOS) {
        for (const auto round_trip_time : ROUND_TRIP_TIMES) {
            SCOPED_TRACE(std::string(ScenarioName(scenario)) + " at RTT " +
                         std::to_string(round_trip_time));
            NetworkedInputSimulation simulation({ { 0, round_trip_time } });
            const SimulationMetrics metrics = simulation.Run(900, scenario);

            EXPECT_EQ(metrics.corrections, 0U);
            EXPECT_EQ(metrics.selected_late_inputs, 0U);
            EXPECT_EQ(metrics.superseded_inputs, 0U);
            EXPECT_EQ(metrics.authoritative_past_replays, 0U);
            EXPECT_GT(metrics.consecutive_accurate_snapshots, 500U);
        }
    }
}

TEST(NetworkedInputSimulationTest, RuntimeLatencyIncreasesUseBoundedResynchronization)
{
    NetworkedInputSimulation simulation({ { 0, 0 }, { 500, 77 }, { 1000, 280 }, { 1500, 445 } });

    const SimulationMetrics metrics = simulation.Run(2400, InputScenario::JumpAndLanding);

    EXPECT_GE(metrics.resynchronizations, 2U);
    EXPECT_LE(metrics.resynchronizations, 3U);
    EXPECT_LE(metrics.corrections, metrics.resynchronizations);
    EXPECT_EQ(metrics.authoritative_past_replays, 0U);
    EXPECT_EQ(metrics.selected_late_inputs, 0U);
    EXPECT_EQ(metrics.superseded_inputs, 0U);
    EXPECT_GT(metrics.resynchronization_wait_snapshots, 0U);
    EXPECT_GT(metrics.consecutive_accurate_snapshots, 500U);
}

TEST(NetworkedInputSimulationTest, LossReorderingDuplicationAndBatchingConverge)
{
    NetworkedInputSimulation simulation({ { 0, 150 } }, true, 1000);

    const SimulationMetrics metrics = simulation.Run(1500, InputScenario::RapidDirectionChanges);

    EXPECT_GT(metrics.corrections, 0U);
    EXPECT_GT(metrics.ignored_old_snapshots, 0U);
    EXPECT_EQ(metrics.authoritative_past_replays, 0U);
    EXPECT_GT(metrics.consecutive_accurate_snapshots, 300U);
}

TEST(NetworkedInputSimulationTest, ClientAndServerTimelinesCrossUint32Wraparound)
{
    constexpr std::uint32_t MAX_VALUE = std::numeric_limits<std::uint32_t>::max();
    NetworkedInputSimulation simulation(
      { { 0, 50 } }, false, 0, MAX_VALUE - 300U, MAX_VALUE - 200U, MAX_VALUE - 250U);

    const SimulationMetrics metrics = simulation.Run(700, InputScenario::Jets);

    EXPECT_EQ(metrics.corrections, 0U);
    EXPECT_EQ(metrics.selected_late_inputs, 0U);
    EXPECT_EQ(metrics.authoritative_past_replays, 0U);
    EXPECT_GT(metrics.consecutive_accurate_snapshots, 300U);
}
