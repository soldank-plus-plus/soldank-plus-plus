#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

import Extern.Glm;

import ClientState;
import Networking.EventHandlers.SoldierInputNetworkEventHandler;
import Networking.IGameServer;
import Networking.INetworkingClient;
import Networking.InputApplicationTimeline;
import Networking.NetworkClientSession;
import Networking.SoldierStateNetworkEventHandler;
import Replication.ReplicationService;
import Runtime.ServerCommandQueues;
import Runtime.ServerRuntimeServices;
import Sessions.PlayerSessionManager;

import Shared.Core.IWorld;
import Shared.Core.Map.Map;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Simulation.PlayerInputApplication;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Simulation.WorldTick;
import Shared.Core.State.Control;
import Shared.Core.State.StateManager;
import Shared.Core.World;
import Shared.Networking.DeliveryMode;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkMessage;

import Testing.Framework.Shared.MapBuilder;

using namespace Soldank;

namespace
{
struct LatencyPhase
{
    std::uint64_t start_step;
    std::uint16_t round_trip_time_milliseconds;
};

struct InFlightMessage
{
    std::uint64_t delivery_step;
    unsigned int connection_id;
    NetworkMessage message;
};

class DeterministicNetworkBridge;

class DeterministicClientEndpoint final : public INetworkingClient
{
public:
    DeterministicClientEndpoint(DeterministicNetworkBridge& network_bridge,
                                unsigned int connection_id);

    void Update(const std::shared_ptr<NetworkEventDispatcher>& dispatcher) override;
    void SendNetworkMessage(const NetworkMessage& message, DeliveryMode delivery_mode) override;
    void SetLag(int lag_to_add_milliseconds) override;

private:
    DeterministicNetworkBridge& network_bridge_;
    unsigned int connection_id_;
};

class DeterministicNetworkBridge final
    : public IGameServer
    , public IServerNetworkHost
{
public:
    explicit DeterministicNetworkBridge(std::vector<LatencyPhase> latency_phases)
        : latency_phases_(std::move(latency_phases))
    {
    }

    void SetCurrentStep(std::uint64_t current_step) { current_step_ = current_step; }

    std::uint16_t GetCurrentRoundTripTime() const
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

    std::unique_ptr<DeterministicClientEndpoint> CreateClientEndpoint(unsigned int connection_id)
    {
        client_connection_ids_.push_back(connection_id);
        return std::make_unique<DeterministicClientEndpoint>(*this, connection_id);
    }

    void DeliverMessagesToServer(const std::shared_ptr<NetworkEventDispatcher>& dispatcher)
    {
        std::erase_if(messages_to_server_, [&](const auto& in_flight_message) {
            if (in_flight_message.delivery_step > current_step_) {
                return false;
            }

            const auto [result, details] = dispatcher->ProcessNetworkMessage(
              { .connection_id = in_flight_message.connection_id,
                .send_message_to_connection = [](const NetworkMessage& /*message*/) {} },
              in_flight_message.message);
            static_cast<void>(details);
            if (result != NetworkEventDispatchResult::Success) {
                dispatch_failure_count_++;
            }
            return true;
        });
    }

    std::size_t GetDispatchFailureCount() const { return dispatch_failure_count_; }

    void DeliverMessagesToClient(unsigned int connection_id,
                                 const std::shared_ptr<NetworkEventDispatcher>& dispatcher)
    {
        std::erase_if(messages_to_client_, [&](const auto& in_flight_message) {
            if (in_flight_message.connection_id != connection_id ||
                in_flight_message.delivery_step > current_step_) {
                return false;
            }

            const auto [result, details] = dispatcher->ProcessNetworkMessage(
              { .connection_id = 0,
                .send_message_to_connection = [](const NetworkMessage& /*message*/) {} },
              in_flight_message.message);
            static_cast<void>(details);
            if (result != NetworkEventDispatchResult::Success) {
                dispatch_failure_count_++;
            }
            return true;
        });
    }

    void QueueMessageToServer(unsigned int connection_id, const NetworkMessage& message)
    {
        const auto event = message.GetNetworkEvent();
        if (!event.has_value() || *event != NetworkEvent::SoldierInput) {
            return;
        }
        messages_to_server_.push_back({ .delivery_step = current_step_ + GetOutgoingLatencyTicks(),
                                        .connection_id = connection_id,
                                        .message = message });
    }

    void Update() override {}

    void SendNetworkMessage(unsigned int connection_id, const NetworkMessage& message) override
    {
        QueueMessageToClient(connection_id, message);
    }

    void SendNetworkMessageToAll(const NetworkMessage& message) override
    {
        for (const unsigned int connection_id : client_connection_ids_) {
            QueueMessageToClient(connection_id, message);
        }
    }

    unsigned int GetSoldierIdFromConnectionId(unsigned int connection_id) override
    {
        const auto connection =
          std::find(client_connection_ids_.begin(), client_connection_ids_.end(), connection_id);
        return connection != client_connection_ids_.end() ? connection_id : 0;
    }

private:
    std::uint32_t GetRoundTripTimeTicks() const
    {
        constexpr std::uint32_t TICKS_PER_SECOND = 60;
        constexpr std::uint32_t MILLISECONDS_PER_SECOND = 1000;
        return (static_cast<std::uint32_t>(GetCurrentRoundTripTime()) * TICKS_PER_SECOND +
                MILLISECONDS_PER_SECOND - 1U) /
               MILLISECONDS_PER_SECOND;
    }

    std::uint32_t GetOutgoingLatencyTicks() const { return GetRoundTripTimeTicks() / 2U; }

    std::uint32_t GetIncomingLatencyTicks() const
    {
        return GetRoundTripTimeTicks() - GetOutgoingLatencyTicks();
    }

    void QueueMessageToClient(unsigned int connection_id, const NetworkMessage& message)
    {
        messages_to_client_.push_back({ .delivery_step = current_step_ + GetIncomingLatencyTicks(),
                                        .connection_id = connection_id,
                                        .message = message });
    }

    std::vector<LatencyPhase> latency_phases_;
    std::uint64_t current_step_ = 0;
    std::vector<unsigned int> client_connection_ids_;
    std::vector<InFlightMessage> messages_to_server_;
    std::vector<InFlightMessage> messages_to_client_;
    std::size_t dispatch_failure_count_ = 0;
};

DeterministicClientEndpoint::DeterministicClientEndpoint(DeterministicNetworkBridge& network_bridge,
                                                         unsigned int connection_id)
    : network_bridge_(network_bridge)
    , connection_id_(connection_id)
{
}

void DeterministicClientEndpoint::Update(const std::shared_ptr<NetworkEventDispatcher>& dispatcher)
{
    network_bridge_.DeliverMessagesToClient(connection_id_, dispatcher);
}

void DeterministicClientEndpoint::SendNetworkMessage(const NetworkMessage& message,
                                                     DeliveryMode /*delivery_mode*/)
{
    network_bridge_.QueueMessageToServer(connection_id_, message);
}

void DeterministicClientEndpoint::SetLag(int /*lag_to_add_milliseconds*/) {}

Control MakeControl(std::uint64_t step)
{
    Control control{};
    const bool moving_right = step % 240U < 120U;
    control.right = moving_right;
    control.left = !moving_right;
    control.up = step % 90U >= 10U && step % 90U < 15U;
    control.jets = step % 150U >= 55U && step % 150U < 70U;
    control.mouse_aim_x = moving_right ? 500 : -500;
    control.mouse_aim_y = -100;
    control.was_running_left = step > 0 && (step - 1U) % 240U >= 120U;
    control.was_jumping = step > 0 && (step - 1U) % 90U >= 10U && (step - 1U) % 90U < 15U;
    return control;
}

void TickWorldWithNoInput(IWorld& world, std::uint32_t tick)
{
    const std::vector<PlayerInputCommand> player_inputs;
    const std::vector<SimulationCommand> simulation_commands;
    static_cast<void>(world.Tick(
      { .tick = tick, .player_inputs = player_inputs, .commands = simulation_commands }));
    world.GetStateManager()->SetGameTick(tick + 1U);
}

void InitializeWorld(IWorld& world,
                     const Map& map,
                     const std::vector<std::pair<unsigned int, glm::vec2>>& soldier_spawn_positions)
{
    world.GetStateManager()->OverrideMap(map);
    for (const auto& [soldier_id, spawn_position] : soldier_spawn_positions) {
        world.CreateSoldier(soldier_id);
        world.SpawnSoldier(soldier_id, spawn_position);
    }

    for (std::uint32_t tick = 0; tick < 120U; tick++) {
        TickWorldWithNoInput(world, tick);
    }
    world.GetStateManager()->SetGameTick(0);
}

struct ReconciliationDisplacementMetrics
{
    float maximum_steady_state_displacement = 0.0F;
    std::uint64_t maximum_steady_state_displacement_step = 0;
    float maximum_resynchronization_displacement = 0.0F;
    std::size_t recurring_teleport_count = 0;
    std::size_t resynchronization_displacement_count = 0;
    bool measure_next_update_as_resynchronization = false;
};

void UpdateClientAndMeasureReconciliation(NetworkClientSession& client_session,
                                          const std::shared_ptr<IWorld>& client_world,
                                          const std::shared_ptr<ClientState>& client_state,
                                          std::uint8_t player_id,
                                          std::uint64_t step,
                                          bool should_measure,
                                          float maximum_allowed_displacement,
                                          ReconciliationDisplacementMetrics& metrics)
{
    const glm::vec2 position_before_network_update =
      client_world->GetSoldier(player_id).particle.position;
    const bool was_awaiting_timeline_resynchronization =
      client_state->network.reconciliation_resume_server_tick.has_value();
    client_session.UpdateBeforeWorldTick();
    if (!should_measure) {
        return;
    }

    const glm::vec2 position_after_network_update =
      client_world->GetSoldier(player_id).particle.position;
    const float displacement =
      glm::distance(position_before_network_update, position_after_network_update);
    const bool completed_timeline_resynchronization =
      was_awaiting_timeline_resynchronization &&
      !client_state->network.reconciliation_resume_server_tick.has_value();
    const bool is_resynchronization_recovery_update =
      completed_timeline_resynchronization || metrics.measure_next_update_as_resynchronization;
    metrics.measure_next_update_as_resynchronization = completed_timeline_resynchronization;
    if (is_resynchronization_recovery_update) {
        metrics.maximum_resynchronization_displacement =
          std::max(metrics.maximum_resynchronization_displacement, displacement);
        if (displacement > maximum_allowed_displacement) {
            metrics.resynchronization_displacement_count++;
        }
        return;
    }

    if (displacement > metrics.maximum_steady_state_displacement) {
        metrics.maximum_steady_state_displacement = displacement;
        metrics.maximum_steady_state_displacement_step = step;
    }
    if (displacement > maximum_allowed_displacement) {
        metrics.recurring_teleport_count++;
    }
}
} // namespace

TEST(FullClientServerReconciliationTest,
     RuntimeLatencyChangesDoNotCauseRecurringReconciliationTeleports)
{
    constexpr std::uint8_t PLAYER_ID = 1;
    constexpr std::uint64_t STEPS = 2400;
    constexpr std::uint64_t START_MEASURING_AT_STEP = 200;
    constexpr float MAXIMUM_ALLOWED_RECONCILIATION_DISPLACEMENT = 1.0F;

    const auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 0.0F }, { 4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 60.0F }, { -4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->Build();

    auto client_world = std::make_shared<World>();
    auto server_world = std::make_shared<World>();
    const std::vector<std::pair<unsigned int, glm::vec2>> soldier_spawn_positions{
        { PLAYER_ID, { 0.0F, -34.0F } }
    };
    InitializeWorld(*client_world, *map, soldier_spawn_positions);
    InitializeWorld(*server_world, *map, soldier_spawn_positions);

    auto client_state = std::make_shared<ClientState>();
    client_state->client_soldier_id = PLAYER_ID;
    client_state->network.server_reconciliation = true;
    client_state->network.client_side_prediction = true;

    auto network_bridge = std::make_shared<DeterministicNetworkBridge>(
      std::vector<LatencyPhase>{ { 0, 0 }, { 500, 77 }, { 1000, 280 }, { 1500, 445 } });
    auto client_endpoint = network_bridge->CreateClientEndpoint(PLAYER_ID);
    auto game_server = std::static_pointer_cast<IGameServer>(network_bridge);

    PlayerSessionManager player_sessions;
    ServerCommandQueues command_queues;
    auto server_dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierInputNetworkEventHandler>(
          game_server, player_sessions, command_queues),
      });
    auto client_dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierStateNetworkEventHandler>(client_world, client_state),
      });

    NetworkClientSession client_session(
      *client_endpoint, client_dispatcher, *client_world, *client_state);
    ReplicationService replication_service(*network_bridge, player_sessions);

    float maximum_steady_state_reconciliation_displacement = 0.0F;
    float maximum_resynchronization_displacement = 0.0F;
    std::size_t recurring_teleport_count = 0;
    std::size_t resynchronization_displacement_count = 0;
    std::size_t airborne_tick_count = 0;
#ifndef NDEBUG
    std::size_t late_input_count = 0;
#endif

    for (std::uint64_t step = 0; step < STEPS; step++) {
        network_bridge->SetCurrentStep(step);
        client_state->network.target_input_delay_ticks =
          std::max(client_state->network.target_input_delay_ticks,
                   CalculateInputDelayTicks(network_bridge->GetCurrentRoundTripTime()));

        network_bridge->DeliverMessagesToServer(server_dispatcher);

        const std::uint32_t server_tick = server_world->GetStateManager()->GetGameTick();
        auto server_inputs = command_queues.SelectPlayerInputsForSimulation(server_tick);
#ifndef NDEBUG
        late_input_count += command_queues.GetInputDebugStats().late_applied_input_count;
        command_queues.ResetInputDebugStats();
#endif
        const std::vector<SimulationCommand> server_commands;
        static_cast<void>(server_world->Tick(
          { .tick = server_tick, .player_inputs = server_inputs, .commands = server_commands }));
        for (const auto& input : server_inputs) {
            player_sessions.MarkInputApplied(input.soldier_id, input.input_sequence_id);
        }
        replication_service.BroadcastTick(*server_world->GetStateManager());
        server_world->GetStateManager()->SetGameTick(server_tick + 1U);

        const glm::vec2 position_before_network_update =
          client_world->GetSoldier(PLAYER_ID).particle.position;
        const bool was_awaiting_timeline_resynchronization =
          client_state->network.reconciliation_resume_server_tick.has_value();
        client_session.UpdateBeforeWorldTick();
        const glm::vec2 position_after_network_update =
          client_world->GetSoldier(PLAYER_ID).particle.position;
        if (step >= START_MEASURING_AT_STEP) {
            const float displacement =
              glm::distance(position_before_network_update, position_after_network_update);
            const bool completed_timeline_resynchronization =
              was_awaiting_timeline_resynchronization &&
              !client_state->network.reconciliation_resume_server_tick.has_value();
            if (completed_timeline_resynchronization) {
                maximum_resynchronization_displacement =
                  std::max(maximum_resynchronization_displacement, displacement);
                if (displacement > MAXIMUM_ALLOWED_RECONCILIATION_DISPLACEMENT) {
                    resynchronization_displacement_count++;
                }
            } else {
                maximum_steady_state_reconciliation_displacement =
                  std::max(maximum_steady_state_reconciliation_displacement, displacement);
                if (displacement > MAXIMUM_ALLOWED_RECONCILIATION_DISPLACEMENT) {
                    recurring_teleport_count++;
                }
            }
        }

        const std::uint32_t client_tick = client_world->GetStateManager()->GetGameTick();
        const Control control = MakeControl(step);
        ApplyPlayerInputCommand(
          *client_world->GetStateManager(),
          { .soldier_id = PLAYER_ID,
            .input_sequence_id = 0,
            .client_tick = client_tick,
            .apply_server_tick = 0,
            .control = control,
            .mouse_map_position = { static_cast<float>(control.mouse_aim_x),
                                    static_cast<float>(control.mouse_aim_y) } });
        client_session.SendSoldierInput(
          PLAYER_ID,
          { static_cast<float>(control.mouse_aim_x), static_cast<float>(control.mouse_aim_y) });
        TickWorldWithNoInput(*client_world, client_tick);
        client_session.StorePredictedSoldierSnapshot(PLAYER_ID);

        if (!client_world->GetSoldier(PLAYER_ID).on_ground) {
            airborne_tick_count++;
        }
    }

    EXPECT_EQ(network_bridge->GetDispatchFailureCount(), 0U);
    EXPECT_EQ(client_state->network.input_timeline_resync_count, 3U);
    EXPECT_EQ(recurring_teleport_count, 0U)
      << "maximum steady-state reconciliation displacement was "
      << maximum_steady_state_reconciliation_displacement;
    EXPECT_LE(resynchronization_displacement_count,
              client_state->network.input_timeline_resync_count);
    EXPECT_LE(maximum_resynchronization_displacement, 30.0F);
    EXPECT_GT(airborne_tick_count, 100U);
#ifndef NDEBUG
    EXPECT_EQ(late_input_count, 0U);
#endif
}

TEST(FullClientServerReconciliationTest,
     TwoSimultaneousPlayersDoNotDevelopRecurringReconciliationTeleports)
{
    constexpr std::uint8_t FIRST_PLAYER_ID = 1;
    constexpr std::uint8_t SECOND_PLAYER_ID = 2;
    constexpr std::uint64_t STEPS = 2400;
    constexpr std::uint64_t START_MEASURING_AT_STEP = 200;
    constexpr float MAXIMUM_ALLOWED_RECONCILIATION_DISPLACEMENT = 1.0F;

    const auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 0.0F }, { 4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 60.0F }, { -4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->Build();
    const std::vector<std::pair<unsigned int, glm::vec2>> soldier_spawn_positions{
        { FIRST_PLAYER_ID, { -500.0F, -34.0F } },
        { SECOND_PLAYER_ID, { 500.0F, -34.0F } },
    };

    auto server_world = std::make_shared<World>();
    auto first_client_world = std::make_shared<World>();
    auto second_client_world = std::make_shared<World>();
    InitializeWorld(*server_world, *map, soldier_spawn_positions);
    InitializeWorld(*first_client_world, *map, soldier_spawn_positions);
    InitializeWorld(*second_client_world, *map, soldier_spawn_positions);
    first_client_world->SetPreSoldierUpdateCallback(
      [](const auto& soldier) { return soldier.id == FIRST_PLAYER_ID; });
    second_client_world->SetPreSoldierUpdateCallback(
      [](const auto& soldier) { return soldier.id == SECOND_PLAYER_ID; });

    auto first_client_state = std::make_shared<ClientState>();
    first_client_state->client_soldier_id = FIRST_PLAYER_ID;
    auto second_client_state = std::make_shared<ClientState>();
    second_client_state->client_soldier_id = SECOND_PLAYER_ID;

    auto network_bridge = std::make_shared<DeterministicNetworkBridge>(
      std::vector<LatencyPhase>{ { 0, 0 }, { 500, 77 }, { 1000, 280 }, { 1500, 445 } });
    auto first_client_endpoint = network_bridge->CreateClientEndpoint(FIRST_PLAYER_ID);
    auto second_client_endpoint = network_bridge->CreateClientEndpoint(SECOND_PLAYER_ID);
    auto game_server = std::static_pointer_cast<IGameServer>(network_bridge);

    PlayerSessionManager player_sessions;
    ServerCommandQueues command_queues;
    auto server_dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierInputNetworkEventHandler>(
          game_server, player_sessions, command_queues),
      });
    auto first_client_dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierStateNetworkEventHandler>(first_client_world, first_client_state),
      });
    auto second_client_dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierStateNetworkEventHandler>(second_client_world, second_client_state),
      });

    NetworkClientSession first_client_session(
      *first_client_endpoint, first_client_dispatcher, *first_client_world, *first_client_state);
    NetworkClientSession second_client_session(*second_client_endpoint,
                                               second_client_dispatcher,
                                               *second_client_world,
                                               *second_client_state);
    ReplicationService replication_service(*network_bridge, player_sessions);

    ReconciliationDisplacementMetrics first_player_metrics;
    ReconciliationDisplacementMetrics second_player_metrics;
    std::size_t first_player_airborne_tick_count = 0;
    std::size_t second_player_airborne_tick_count = 0;
#ifndef NDEBUG
    std::size_t late_input_count = 0;
#endif

    for (std::uint64_t step = 0; step < STEPS; step++) {
        network_bridge->SetCurrentStep(step);
        const std::uint32_t target_input_delay =
          CalculateInputDelayTicks(network_bridge->GetCurrentRoundTripTime());
        first_client_state->network.target_input_delay_ticks =
          std::max(first_client_state->network.target_input_delay_ticks, target_input_delay);
        second_client_state->network.target_input_delay_ticks =
          std::max(second_client_state->network.target_input_delay_ticks, target_input_delay);

        network_bridge->DeliverMessagesToServer(server_dispatcher);

        const std::uint32_t server_tick = server_world->GetStateManager()->GetGameTick();
        auto server_inputs = command_queues.SelectPlayerInputsForSimulation(server_tick);
#ifndef NDEBUG
        late_input_count += command_queues.GetInputDebugStats().late_applied_input_count;
        command_queues.ResetInputDebugStats();
#endif
        const std::vector<SimulationCommand> server_commands;
        static_cast<void>(server_world->Tick(
          { .tick = server_tick, .player_inputs = server_inputs, .commands = server_commands }));
        for (const auto& input : server_inputs) {
            player_sessions.MarkInputApplied(input.soldier_id, input.input_sequence_id);
        }
        replication_service.BroadcastTick(*server_world->GetStateManager());
        server_world->GetStateManager()->SetGameTick(server_tick + 1U);

        const bool should_measure = step >= START_MEASURING_AT_STEP;
        UpdateClientAndMeasureReconciliation(first_client_session,
                                             first_client_world,
                                             first_client_state,
                                             FIRST_PLAYER_ID,
                                             step,
                                             should_measure,
                                             MAXIMUM_ALLOWED_RECONCILIATION_DISPLACEMENT,
                                             first_player_metrics);
        UpdateClientAndMeasureReconciliation(second_client_session,
                                             second_client_world,
                                             second_client_state,
                                             SECOND_PLAYER_ID,
                                             step,
                                             should_measure,
                                             MAXIMUM_ALLOWED_RECONCILIATION_DISPLACEMENT,
                                             second_player_metrics);

        const Control first_control = MakeControl(step);
        const Control second_control = MakeControl(step + 60U);
        const std::uint32_t first_client_tick =
          first_client_world->GetStateManager()->GetGameTick();
        const std::uint32_t second_client_tick =
          second_client_world->GetStateManager()->GetGameTick();
        ApplyPlayerInputCommand(
          *first_client_world->GetStateManager(),
          { .soldier_id = FIRST_PLAYER_ID,
            .input_sequence_id = 0,
            .client_tick = first_client_tick,
            .apply_server_tick = 0,
            .control = first_control,
            .mouse_map_position = { static_cast<float>(first_control.mouse_aim_x),
                                    static_cast<float>(first_control.mouse_aim_y) } });
        ApplyPlayerInputCommand(
          *second_client_world->GetStateManager(),
          { .soldier_id = SECOND_PLAYER_ID,
            .input_sequence_id = 0,
            .client_tick = second_client_tick,
            .apply_server_tick = 0,
            .control = second_control,
            .mouse_map_position = { static_cast<float>(second_control.mouse_aim_x),
                                    static_cast<float>(second_control.mouse_aim_y) } });
        first_client_session.SendSoldierInput(FIRST_PLAYER_ID,
                                              { static_cast<float>(first_control.mouse_aim_x),
                                                static_cast<float>(first_control.mouse_aim_y) });
        second_client_session.SendSoldierInput(SECOND_PLAYER_ID,
                                               { static_cast<float>(second_control.mouse_aim_x),
                                                 static_cast<float>(second_control.mouse_aim_y) });
        TickWorldWithNoInput(*first_client_world, first_client_tick);
        TickWorldWithNoInput(*second_client_world, second_client_tick);
        first_client_session.StorePredictedSoldierSnapshot(FIRST_PLAYER_ID);
        second_client_session.StorePredictedSoldierSnapshot(SECOND_PLAYER_ID);

        if (!first_client_world->GetSoldier(FIRST_PLAYER_ID).on_ground) {
            first_player_airborne_tick_count++;
        }
        if (!second_client_world->GetSoldier(SECOND_PLAYER_ID).on_ground) {
            second_player_airborne_tick_count++;
        }
    }

    EXPECT_EQ(network_bridge->GetDispatchFailureCount(), 0U);
    EXPECT_GT(player_sessions.GetLastAppliedInputId(FIRST_PLAYER_ID), 2000U);
    EXPECT_GT(player_sessions.GetLastAppliedInputId(SECOND_PLAYER_ID), 2000U);
    EXPECT_EQ(first_client_state->network.input_timeline_resync_count, 3U);
    EXPECT_EQ(second_client_state->network.input_timeline_resync_count, 3U);
    EXPECT_EQ(first_player_metrics.recurring_teleport_count, 0U)
      << "first player's maximum steady-state displacement was "
      << first_player_metrics.maximum_steady_state_displacement << " at step "
      << first_player_metrics.maximum_steady_state_displacement_step;
    EXPECT_EQ(second_player_metrics.recurring_teleport_count, 0U)
      << "second player's maximum steady-state displacement was "
      << second_player_metrics.maximum_steady_state_displacement << " at step "
      << second_player_metrics.maximum_steady_state_displacement_step;
    EXPECT_LE(first_player_metrics.resynchronization_displacement_count,
              first_client_state->network.input_timeline_resync_count * 2U);
    EXPECT_LE(second_player_metrics.resynchronization_displacement_count,
              second_client_state->network.input_timeline_resync_count * 2U);
    EXPECT_LE(first_player_metrics.maximum_resynchronization_displacement, 30.0F);
    EXPECT_LE(second_player_metrics.maximum_resynchronization_displacement, 30.0F);
    EXPECT_GT(first_player_airborne_tick_count, 100U);
    EXPECT_GT(second_player_airborne_tick_count, 100U);
#ifndef NDEBUG
    EXPECT_EQ(late_input_count, 0U);
#endif
}
