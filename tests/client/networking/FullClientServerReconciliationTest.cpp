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
import Networking.CoreEventsConnectionNotifier;
import Networking.ProjectilePhysicsNetworkEventHandler;
import Networking.PolygonEffectNetworkEventHandler;
import Shared.Core.Entities.Bullet;
import Shared.Core.Physics.Bullets.BulletLagCompensation;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;
import Networking.INetworkingClient;
import Networking.InputApplicationTimeline;
import Networking.NetworkClientSession;
import Networking.SoldierStateNetworkEventHandler;
import Networking.ProjectileSpawnNetworkEventHandler;
import Networking.ItemStateNetworkEventHandler;
import Networking.HitSoldierNetworkEventHandler;
import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParametersFactory;
import Gameplay.WeaponRecoil;
import Audio.AudioEvents;
import Gameplay.WeaponSoundEventRouter;
import WeaponVisualEffects;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.ItemType;
import Shared.Core.Entities.Item;
import Shared.Core.Physics.Polygons.FlagCarry;
import Shared.Core.Map.Polygons.PolygonBackgroundState;
import Shared.Core.Animations;
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
import Shared.Networking.NetworkPackets;
import Shared.Networking.ProtocolConversions;

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

TEST(FullClientServerReconciliationTest, ProjectilePushesConvergeUnderChangingLatency)
{
    constexpr std::uint8_t PLAYER_ID = 1;
    const auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 0.0F }, { 4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 60.0F }, { -4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->Build();
    auto client_world = std::make_shared<World>();
    auto server_world = std::make_shared<World>();
    const std::vector<std::pair<unsigned int, glm::vec2>> positions{ { PLAYER_ID,
                                                                       { 0.0F, -34.0F } } };
    InitializeWorld(*client_world, *map, positions);
    InitializeWorld(*server_world, *map, positions);
    client_world->GetStateManager()->SetAuthoritativeProjectiles(false);
    auto client_state = std::make_shared<ClientState>();
    client_state->client_soldier_id = PLAYER_ID;
    client_state->network.server_reconciliation = true;
    client_state->network.client_side_prediction = true;
    auto bridge = std::make_shared<DeterministicNetworkBridge>(
      std::vector<LatencyPhase>{ { 0, 77 }, { 250, 280 }, { 500, 445 } });
    auto endpoint = bridge->CreateClientEndpoint(PLAYER_ID);
    auto server = std::static_pointer_cast<IGameServer>(bridge);
    PlayerSessionManager sessions;
    ServerCommandQueues queues;
    auto server_dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierInputNetworkEventHandler>(server, sessions, queues) });
    auto client_dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierStateNetworkEventHandler>(client_world, client_state),
        std::make_shared<PolygonEffectNetworkEventHandler>(client_world),
        std::make_shared<ProjectilePhysicsNetworkEventHandler>(client_world, client_state) });
    CoreEventsConnectionNotifier::ObserveAll(
      bridge.get(), server_world->GetWorldEvents(), server_world->GetPhysicsEvents());
    NetworkClientSession client_session(*endpoint, client_dispatcher, *client_world, *client_state);
    ReplicationService replication(*bridge, sessions);
    unsigned int received_pushes = 0;
    client_world->GetPhysicsEvents().projectile_event.AddObserver(
      [&](const ProjectilePhysicsEvent& event) {
          if (event.kind == ProjectileEventKind::SoldierProjectilePushScheduled) {
              ++received_pushes;
          }
      });
    for (std::uint64_t step = 0; step < 1000; ++step) {
        bridge->SetCurrentStep(step);
        client_state->network.target_input_delay_ticks =
          std::max(client_state->network.target_input_delay_ticks,
                   CalculateInputDelayTicks(bridge->GetCurrentRoundTripTime()));
        bridge->DeliverMessagesToServer(server_dispatcher);
        auto state = server_world->GetStateManager();
        const auto tick = state->GetGameTick();
        if (step == 120 || step == 370 || step == 620) {
            Bullet bullet{ BulletParams{ .style = BulletType::Bullet,
                                         .weapon = WeaponType::Ak74,
                                         .position = {},
                                         .velocity = { 10.0F, 0.0F },
                                         .timeout = 100,
                                         .hit_multiply = 1.0F,
                                         .team = TeamType::None,
                                         .owner_id = 255,
                                         .push = 0.1F,
                                         .owner_ping_ticks = static_cast<std::uint16_t>(
                                           bridge->GetCurrentRoundTripTime() * 60 / 1000) } };
            BulletLagCompensation::SchedulePush(
              *state, state->GetSoldier(PLAYER_ID), bullet, { 2.0F, -1.0F });
        }
        auto inputs = queues.SelectPlayerInputsForSimulation(tick);
        const std::vector<SimulationCommand> commands;
        static_cast<void>(
          server_world->Tick({ .tick = tick, .player_inputs = inputs, .commands = commands }));
        for (const auto& input : inputs) {
            sessions.MarkInputApplied(input.soldier_id, input.input_sequence_id);
        }
        replication.BroadcastTick(*state);
        state->SetGameTick(tick + 1);
        client_session.UpdateBeforeWorldTick();
        client_session.SendSoldierInput(PLAYER_ID, { 100.0F, 0.0F });
        const auto client_tick = client_world->GetStateManager()->GetGameTick();
        TickWorldWithNoInput(*client_world, client_tick);
        client_session.StorePredictedSoldierSnapshot(PLAYER_ID);
    }
    EXPECT_EQ(bridge->GetDispatchFailureCount(), 0U);
    EXPECT_EQ(received_pushes, 3U);
    EXPECT_LE(glm::distance(client_world->GetSoldier(PLAYER_ID).particle.position,
                            server_world->GetSoldier(PLAYER_ID).particle.position),
              1.5F);
    EXPECT_LE(glm::distance(client_world->GetSoldier(PLAYER_ID).particle.GetVelocity(),
                            server_world->GetSoldier(PLAYER_ID).particle.GetVelocity()),
              0.15F);
}

TEST(FullClientServerReconciliationTest, AuthoritativeRemovalTargetsTheCorrectProjectile)
{
    auto world = std::make_shared<World>();
    auto client_state = std::make_shared<ClientState>();
    world->GetStateManager()->SetAuthoritativeProjectiles(false);
    world->GetStateManager()->CreateProjectile({ .style = BulletType::Bullet,
                                                 .weapon = WeaponType::Ak74,
                                                 .position = {},
                                                 .velocity = { 10.0F, 0.0F },
                                                 .timeout = 100,
                                                 .hit_multiply = 1.0F,
                                                 .team = TeamType::None,
                                                 .owner_id = 255,
                                                 .push = 1.0F,
                                                 .id = 10 });
    auto dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<PolygonEffectNetworkEventHandler>(world),
        std::make_shared<ProjectilePhysicsNetworkEventHandler>(world, client_state) });
    const ConnectionMetadata connection{
        .connection_id = 1, .send_message_to_connection = [](const NetworkMessage&) {}
    };
    ProjectilePhysicsPacket packet{ .kind = ProjectileEventKind::ProjectileDestroyed,
                                    .projectile_id = 11 };
    EXPECT_EQ(
      dispatcher
        ->ProcessNetworkMessage(connection, NetworkMessage(NetworkEvent::ProjectilePhysics, packet))
        .first,
      NetworkEventDispatchResult::Success);
    EXPECT_EQ(world->GetStateManager()->GetBulletsCount(), 1U);
    packet.projectile_id = 10;
    EXPECT_EQ(
      dispatcher
        ->ProcessNetworkMessage(connection, NetworkMessage(NetworkEvent::ProjectilePhysics, packet))
        .first,
      NetworkEventDispatchResult::Success);
    EXPECT_EQ(world->GetStateManager()->GetBulletsCount(), 0U);
}

TEST(FullClientServerReconciliationTest, WeaponAmmoAndReloadConvergeUnderChangingLatency)
{
    constexpr std::uint8_t PLAYER_ID = 1;
    const auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 0.0F }, { 4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 60.0F }, { -4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->Build();
    auto client_world = std::make_shared<World>();
    auto server_world = std::make_shared<World>();
    const std::vector<std::pair<unsigned int, glm::vec2>> positions{ { PLAYER_ID,
                                                                       { 0.0F, -34.0F } } };
    InitializeWorld(*client_world, *map, positions);
    InitializeWorld(*server_world, *map, positions);
    client_world->GetStateManager()->SetAuthoritativeProjectiles(false);
    server_world->GetStateManager()->ChangeSoldierPrimaryWeapon(PLAYER_ID, WeaponType::MP5);
    client_world->GetStateManager()->ChangeSoldierPrimaryWeapon(PLAYER_ID, WeaponType::MP5);
    auto client_state = std::make_shared<ClientState>();
    client_state->client_soldier_id = PLAYER_ID;
    client_state->network.server_reconciliation = true;
    client_state->network.client_side_prediction = true;
    auto audio_events = std::make_shared<AudioEvents>();
    std::vector<std::uint32_t> heard_shot_sequences;
    audio_events->sound_requested.AddObserver([&](const AudioEvent& event) {
        if (event.command == AudioCommand::Play && event.sound == "mp5-fire") {
            EXPECT_FALSE(client_state->network.replaying_prediction);
            heard_shot_sequences.push_back(
              client_world->GetSoldier(PLAYER_ID).GetActiveWeapon().GetState().shot_sequence);
        }
    });
    auto sound_router =
      WeaponSoundEventRouter::Create(client_world->GetPhysicsEvents(),
                                     client_world->GetWorldEvents(),
                                     *client_world->GetStateManager(),
                                     audio_events,
                                     [&] { return !client_state->network.replaying_prediction; });
    auto bridge = std::make_shared<DeterministicNetworkBridge>(
      std::vector<LatencyPhase>{ { 0, 77 }, { 250, 280 }, { 500, 445 } });
    auto endpoint = bridge->CreateClientEndpoint(PLAYER_ID);
    auto server = std::static_pointer_cast<IGameServer>(bridge);
    PlayerSessionManager sessions;
    ServerCommandQueues queues;
    auto server_dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierInputNetworkEventHandler>(server, sessions, queues) });
    auto client_dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierStateNetworkEventHandler>(client_world, client_state),
        std::make_shared<PolygonEffectNetworkEventHandler>(client_world),
        std::make_shared<ProjectilePhysicsNetworkEventHandler>(client_world, client_state),
        std::make_shared<ProjectileSpawnNetworkEventHandler>(client_world),
        std::make_shared<ItemStateNetworkEventHandler>(client_world) });
    CoreEventsConnectionNotifier::ObserveAll(
      bridge.get(), server_world->GetWorldEvents(), server_world->GetPhysicsEvents());
    NetworkClientSession client_session(*endpoint, client_dispatcher, *client_world, *client_state);
    ReplicationService replication(*bridge, sessions);
    for (std::uint64_t step = 0; step < 1000; ++step) {
        bridge->SetCurrentStep(step);
        client_state->network.target_input_delay_ticks =
          std::max(client_state->network.target_input_delay_ticks,
                   CalculateInputDelayTicks(bridge->GetCurrentRoundTripTime()));
        bridge->DeliverMessagesToServer(server_dispatcher);
        auto state = server_world->GetStateManager();
        const auto tick = state->GetGameTick();
        auto inputs = queues.SelectPlayerInputsForSimulation(tick);
        const std::vector<SimulationCommand> commands;
        const auto result =
          server_world->Tick({ .tick = tick, .player_inputs = inputs, .commands = commands });
        replication.OnSimulationEvents(result.events);
        for (const auto& input : inputs) {
            sessions.MarkInputApplied(input.soldier_id, input.input_sequence_id);
        }
        replication.BroadcastTick(*state);
        state->SetGameTick(tick + 1);
        client_session.UpdateBeforeWorldTick();
        client_world->GetStateManager()->TransformSoldier(PLAYER_ID, [&](Soldier& soldier) {
            soldier.control.fire = step > 50 && step < 700;
            soldier.control.reload = step == 200 || step == 500;
        });
        client_session.SendSoldierInput(PLAYER_ID, { 1000.0F, -100.0F });
        const auto client_tick = client_world->GetStateManager()->GetGameTick();
        TickWorldWithNoInput(*client_world, client_tick);
        client_session.StorePredictedSoldierSnapshot(PLAYER_ID);
    }
    EXPECT_EQ(bridge->GetDispatchFailureCount(), 0U);
    const auto& server_weapon = server_world->GetSoldier(PLAYER_ID).GetActiveWeapon();
    const auto& client_weapon = client_world->GetSoldier(PLAYER_ID).GetActiveWeapon();
    EXPECT_GT(server_weapon.GetState().shot_sequence, 10U);
    EXPECT_EQ(server_weapon.GetState(), client_weapon.GetState());
    EXPECT_GT(heard_shot_sequences.size(), 10U);
    std::ranges::sort(heard_shot_sequences);
    EXPECT_EQ(std::adjacent_find(heard_shot_sequences.begin(), heard_shot_sequences.end()),
              heard_shot_sequences.end());
    EXPECT_TRUE(client_world->GetStateManager()->GetBulletEmitter().empty());
    EXPECT_LE(glm::distance(client_world->GetSoldier(PLAYER_ID).particle.position,
                            server_world->GetSoldier(PLAYER_ID).particle.position),
              1.5F);
    EXPECT_LE(glm::distance(client_world->GetSoldier(PLAYER_ID).particle.GetVelocity(),
                            server_world->GetSoldier(PLAYER_ID).particle.GetVelocity()),
              0.15F);
}

TEST(FullClientServerReconciliationTest, WeaponMismatchForcesCorrectionDespiteAccuratePosition)
{
    auto world = std::make_shared<World>();
    world->CreateSoldier(1);
    world->SpawnSoldier(1, glm::vec2{});
    world->GetStateManager()->SetAuthoritativeProjectiles(false);
    auto client = std::make_shared<ClientState>();
    client->client_soldier_id = 1;
    client->network.client_side_prediction = true;
    client->network.server_reconciliation = true;
    client->network.soldier_snapshot_history.emplace_back(0, 0, 10, world->GetSoldier(1));
    SoldierStatePacket packet{};
    packet.server_tick = 10;
    packet.player_id = 1;
    packet.body_animation_type = AnimationType::Stand;
    packet.body_animation_frame = 1;
    packet.body_animation_speed = 3;
    packet.legs_animation_type = AnimationType::Stand;
    packet.legs_animation_frame = 1;
    packet.legs_animation_speed = 3;
    packet.weapon_count = 3;
    packet.health = 120.0F;
    packet.vest = 30.0F;
    for (std::size_t i = 0; i < 3; ++i) {
        packet.weapons[i] = world->GetSoldier(1).weapons[i].GetState();
    }
    packet.weapons[0].ammo = 1;
    packet.weapons[0].trigger_latched = true;
    auto dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierStateNetworkEventHandler>(world, client),
        std::make_shared<HitSoldierNetworkEventHandler>(world) });
    const ConnectionMetadata connection{
        .connection_id = 1, .send_message_to_connection = [](const NetworkMessage&) {}
    };
    EXPECT_EQ(
      dispatcher
        ->ProcessNetworkMessage(connection, NetworkMessage(NetworkEvent::SoldierState, packet))
        .first,
      NetworkEventDispatchResult::Success);
    EXPECT_EQ(world->GetSoldier(1).GetActiveWeapon().GetAmmoCount(), 1);
    EXPECT_TRUE(world->GetSoldier(1).GetActiveWeapon().GetState().trigger_latched);
    EXPECT_FLOAT_EQ(world->GetSoldier(1).health, 120.0F);
    EXPECT_FLOAT_EQ(world->GetSoldier(1).vest, 30.0F);

    EXPECT_EQ(dispatcher
                ->ProcessNetworkMessage(
                  connection, NetworkMessage(NetworkEvent::HitSoldier, std::uint8_t{ 1 }, 20.0F))
                .first,
              NetworkEventDispatchResult::Success);
    EXPECT_FLOAT_EQ(world->GetSoldier(1).health, 120.0F);

    packet.server_tick = 9;
    packet.weapons[0].ammo = 2;
    EXPECT_EQ(
      dispatcher
        ->ProcessNetworkMessage(connection, NetworkMessage(NetworkEvent::SoldierState, packet))
        .first,
      NetworkEventDispatchResult::Success);
    EXPECT_EQ(world->GetSoldier(1).GetActiveWeapon().GetAmmoCount(), 1);
}

TEST(FullClientServerReconciliationTest, ItemSnapshotsPreserveIdentityAmmoAndRejectStaleRemoval)
{
    auto world = std::make_shared<World>();
    auto dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<ItemStateNetworkEventHandler>(world) });
    const ConnectionMetadata connection{
        .connection_id = 1, .send_message_to_connection = [](const NetworkMessage&) {}
    };
    ItemStatePacket packet{};
    packet.id = 7;
    packet.active = true;
    packet.server_tick = 10;
    packet.style = ItemType::Ak74;
    packet.owner = 255;
    packet.ammo = 2;
    packet.radius = 10.0F;
    packet.timeout = 100;
    packet.particle_count = 2;
    packet.particles[0] = { 10.0F, 20.0F, 9.0F, 20.0F };
    packet.particles[1] = { 12.0F, 20.0F, 11.0F, 20.0F };
    const auto send = [&] {
        return dispatcher
          ->ProcessNetworkMessage(connection, NetworkMessage(NetworkEvent::ItemState, packet))
          .first;
    };
    ASSERT_EQ(send(), NetworkEventDispatchResult::Success);
    unsigned int items = 0;
    world->GetStateManager()->ForEachItem([&](const auto& item) {
        ++items;
        EXPECT_EQ(item.id, 7);
        EXPECT_EQ(item.ammo_count, 2);
    });
    EXPECT_EQ(items, 1U);
    packet.server_tick = 9;
    packet.active = false;
    EXPECT_EQ(send(), NetworkEventDispatchResult::Success);
    items = 0;
    world->GetStateManager()->ForEachItem([&](const auto&) { ++items; });
    EXPECT_EQ(items, 1U);
    packet.server_tick = 11;
    EXPECT_EQ(send(), NetworkEventDispatchResult::Success);
    items = 0;
    world->GetStateManager()->ForEachItem([&](const auto&) { ++items; });
    EXPECT_EQ(items, 0U);
}

TEST(WeaponPresentationTest, RecoilRaisesAimOnBothSidesAndPreservesZeroLengthAim)
{
    auto world = std::make_shared<World>();
    world->CreateSoldier(1);
    world->SpawnSoldier(1, glm::vec2{});
    world->GetStateManager()->SetRealistic(true);
    world->GetStateManager()->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    auto& soldier = world->GetStateManager()->GetSoldier(1);
    auto firing = soldier.GetActiveWeapon().GetState();
    firing.burst_count = 10;
    ASSERT_TRUE(soldier.GetActiveWeapon().RestoreState(firing));
    EXPECT_EQ(ApplyWeaponRecoil(soldier, {}, {}), glm::vec2{});
    const auto right = ApplyWeaponRecoil(soldier, {}, { 100.0F, 0.0F });
    const auto left = ApplyWeaponRecoil(soldier, {}, { -100.0F, 0.0F });
    EXPECT_LT(right.y, 0.0F);
    EXPECT_LT(left.y, 0.0F);
    EXPECT_NEAR(right.x, -left.x, 0.0001F);
}

TEST(WeaponPresentationTest, CasingsAreDeduplicatedAcrossPredictionCorrection)
{
    auto world = std::make_shared<World>();
    world->CreateSoldier(1);
    world->SpawnSoldier(1, glm::vec2{});
    auto state = world->GetStateManager();
    const auto empty_map = SoldankTesting::MapBuilder::Empty()->Build();
    state->OverrideMap(*empty_map);
    WeaponVisualEffects effects;
    effects.Update(*state, {});
    auto ready = state->GetSoldier(1).GetActiveWeapon().GetState();
    ready.fire_interval = 0;
    ready.startup = 0;
    auto& weapon = state->GetSoldier(1).GetActiveWeapon();
    ASSERT_TRUE(weapon.RestoreState(ready));
    ASSERT_TRUE(weapon.TryFire());
    const auto fired = weapon.GetState();
    state->SetGameTick(1);
    effects.Update(*state, {});
    ASSERT_EQ(effects.GetEffects().size(), 2U);
    effects.Update(*state, {});
    EXPECT_EQ(effects.GetEffects().size(), 2U);
    ASSERT_TRUE(weapon.RestoreState(ready));
    state->SetGameTick(2);
    effects.Update(*state, {});
    ASSERT_TRUE(weapon.RestoreState(fired));
    state->SetGameTick(3);
    effects.Update(*state, {});
    EXPECT_EQ(effects.GetEffects().size(), 2U);
    const auto previous_instance = weapon.GetState().instance_id;
    world->SpawnSoldier(1, glm::vec2{});
    EXPECT_NE(weapon.GetState().instance_id, previous_instance);
    state->SetGameTick(4);
    effects.Update(*state, {});
    auto new_ready = weapon.GetState();
    new_ready.fire_interval = 0;
    new_ready.startup = 0;
    ASSERT_TRUE(weapon.RestoreState(new_ready));
    ASSERT_TRUE(weapon.TryFire());
    state->SetGameTick(5);
    effects.Update(*state, {});
    EXPECT_EQ(effects.GetEffects().size(), 4U);
}

TEST(WeaponPresentationTest, AuthoritativeImpactEffectsDoNotChangeGameplayOrItsRandomSequence)
{
    auto world = std::make_shared<World>();
    auto state = world->GetStateManager();
    state->SetRandomSeed(99);
    const auto expected_random = state->GetRandomInt(0, 10000);
    state->SetRandomSeed(99);
    WeaponVisualEffects effects;
    const ProjectilePhysicsEvent explosion{ .kind = ProjectileEventKind::ProjectileExploded,
                                            .projectile_id = 10,
                                            .style = BulletType::FragGrenade,
                                            .weapon = WeaponType::FragGrenade,
                                            .tick = 42,
                                            .position = { 100.0F, 100.0F },
                                            .radius = 64.0F };
    const std::vector<ProjectilePhysicsEvent> repeated{ explosion, explosion };
    effects.Update(*state, repeated);
    ASSERT_EQ(effects.GetEffects().size(), 3U);
    EXPECT_EQ(effects.GetEffects().back().kind, WeaponVisualEffectKind::Explosion);
    EXPECT_EQ(state->GetRandomInt(0, 10000), expected_random);
    EXPECT_TRUE(state->GetBulletEmitter().empty());
    for (unsigned int tick = 1; tick <= 190; ++tick) {
        state->SetGameTick(tick);
        effects.Update(*state, repeated);
    }
    EXPECT_TRUE(effects.GetEffects().empty());
}

TEST(FullClientServerReconciliationTest,
     InputLoadoutSelectionAppliesAtRespawnWithoutRefillingLiveAmmo)
{
    auto world = std::make_shared<World>();
    world->CreateSoldier(1);
    world->SpawnSoldier(1, glm::vec2{});
    SoldierInputPacket input{};
    input.has_weapon_choice = true;
    input.primary_choice = WeaponType::MP5;
    input.secondary_choice = WeaponType::LAW;
    const auto command = ProtocolConversions::ToPlayerInputCommand(1, input);
    ApplyPlayerInputCommand(*world->GetStateManager(), command);
    EXPECT_EQ(world->GetSoldier(1).GetActiveWeapon().GetWeaponParameters().kind,
              WeaponType::DesertEagles);
    world->SpawnSoldier(1, glm::vec2{});
    EXPECT_EQ(world->GetSoldier(1).weapons.at(0).GetWeaponParameters().kind, WeaponType::MP5);
    EXPECT_EQ(world->GetSoldier(1).weapons.at(1).GetWeaponParameters().kind, WeaponType::LAW);
    input.primary_choice = WeaponType::M2;
    ApplyPlayerInputCommand(*world->GetStateManager(),
                            ProtocolConversions::ToPlayerInputCommand(1, input));
    EXPECT_EQ(world->GetSoldier(1).weapon_choices.at(0), WeaponType::MP5);
}

TEST(FullClientServerReconciliationTest,
     WeaponConfigurationMismatchStopsPredictionUntilMatchingSnapshot)
{
    auto world = std::make_shared<World>();
    world->CreateSoldier(1);
    world->SpawnSoldier(1, glm::vec2{});
    auto client = std::make_shared<ClientState>();
    auto dispatcher =
      std::make_shared<NetworkEventDispatcher>(std::vector<std::shared_ptr<INetworkEventHandler>>{
        std::make_shared<SoldierStateNetworkEventHandler>(world, client) });
    SoldierStatePacket packet{};
    packet.server_tick = 10;
    packet.player_id = 1;
    packet.body_animation_type = AnimationType::Stand;
    packet.body_animation_frame = 1;
    packet.body_animation_speed = 3;
    packet.legs_animation_type = AnimationType::Stand;
    packet.legs_animation_frame = 1;
    packet.legs_animation_speed = 3;
    packet.weapon_count = 3;
    for (std::size_t i = 0; i < 3; ++i) {
        packet.weapons[i] = world->GetSoldier(1).weapons[i].GetState();
    }
    const auto expected_hash = WeaponParametersFactory::GetConfigurationHash(false);
    packet.weapon_config_hash = expected_hash ^ 1U;
    const ConnectionMetadata connection{
        .connection_id = 1, .send_message_to_connection = [](const NetworkMessage&) {}
    };
    const auto send = [&] {
        return dispatcher
          ->ProcessNetworkMessage(connection, NetworkMessage(NetworkEvent::SoldierState, packet))
          .first;
    };
    EXPECT_EQ(send(), NetworkEventDispatchResult::HandlerFailure);
    EXPECT_TRUE(client->network.weapon_config_mismatch);
    packet.server_tick = 9;
    packet.weapon_config_hash = expected_hash;
    EXPECT_EQ(send(), NetworkEventDispatchResult::Success);
    EXPECT_TRUE(client->network.weapon_config_mismatch);
    packet.server_tick = 11;
    EXPECT_EQ(send(), NetworkEventDispatchResult::Success);
    EXPECT_FALSE(client->network.weapon_config_mismatch);
}

TEST(WeaponPresentationTest, ExplosionSpritesUseReferenceTimingScaleAnchorsAndCompositing)
{
    for (const auto weapon :
         { WeaponType::M79, WeaponType::LAW, WeaponType::FragGrenade, WeaponType::Cluster }) {
        SCOPED_TRACE(static_cast<int>(weapon));
        const bool cluster = weapon == WeaponType::Cluster;
        const bool frag = weapon == WeaponType::FragGrenade;
        WeaponVisualEffect effect{ .kind = WeaponVisualEffectKind::Explosion,
                                   .weapon = weapon,
                                   .position = { 100.0F, 200.0F },
                                   .lifetime = 48,
                                   .initial_lifetime = 48 };
        const auto initial = GetExplosionSprites(effect, 1.0F);
        ASSERT_EQ(initial.size(), cluster ? 1U : 2U);
        const auto& blast = initial.back();
        EXPECT_EQ(blast.frame, cluster ? 0 : 3);
        EXPECT_EQ(blast.scale, glm::vec2(cluster ? 0.5F : (frag ? 1.0F : 0.75F)));
        EXPECT_EQ(blast.position,
                  glm::vec2(100.0F, 200.0F) +
                    (cluster ? glm::vec2(-15.0F, -37.0F)
                             : (frag ? glm::vec2(-25.0F, -50.0F) : glm::vec2(-19.0F, -38.0F))));
        EXPECT_FLOAT_EQ(blast.color.a, (cluster ? 159.0F : 223.0F) / 255.0F);
        if (!cluster) {
            EXPECT_EQ(initial.front().frame, 2);
            EXPECT_FLOAT_EQ(initial.front().color.r, (frag ? 170.0F : 173.0F) / 255.0F);
            EXPECT_FLOAT_EQ(initial.front().color.a, 100.0F / 255.0F);
        }
        effect.lifetime = 1;
        const auto final = GetExplosionSprites(effect, 1.0F);
        EXPECT_EQ(final.back().frame, 15);
        EXPECT_FLOAT_EQ(final.back().color.a, (cluster ? 253.0F : 176.0F) / 255.0F);
    }
}

TEST(WeaponPresentationTest, ExplosionFrameInterpolationUsesPascalTiesToEven)
{
    WeaponVisualEffect effect{ .kind = WeaponVisualEffectKind::Explosion,
                               .weapon = WeaponType::M79,
                               .lifetime = 42,
                               .initial_lifetime = 48 };
    EXPECT_EQ(GetExplosionSprites(effect, 1.0F).back().frame, 5); // Round(10.5) = 10
    EXPECT_EQ(GetExplosionSprites(effect, 0.0F).back().frame, 4); // Previous life 43
    effect.lifetime = 38;
    EXPECT_EQ(GetExplosionSprites(effect, 1.0F).back().frame, 5); // Round(9.5) = 10
    effect.lifetime = 37;
    EXPECT_EQ(GetExplosionSprites(effect, 0.5F).back().frame, 6);
}

TEST(WeaponPresentationTest, ExplosionSmokeHasReferenceDelayAndSeparateFade)
{
    WeaponVisualEffect effect{ .kind = WeaponVisualEffectKind::ExplosionSmoke,
                               .position = { 100.0F, 200.0F },
                               .lifetime = 50,
                               .initial_lifetime = 50 };
    EXPECT_TRUE(GetExplosionSprites(effect, 1.0F).empty());
    effect.lifetime = 40;
    const auto start = GetExplosionSprites(effect, 1.0F);
    ASSERT_EQ(start.size(), 1U);
    EXPECT_EQ(start.front().kind, ExplosionSpriteKind::Smoke);
    EXPECT_EQ(start.front().frame, 0);
    EXPECT_EQ(start.front().position, glm::vec2(74.0F, 152.0F));
    EXPECT_FLOAT_EQ(start.front().color.a, 130.0F / 255.0F);
    effect.lifetime = 36;
    const auto layered = GetExplosionSprites(effect, 1.0F);
    ASSERT_EQ(layered.size(), 2U);
    EXPECT_EQ(layered.front().frame, 0);
    EXPECT_EQ(layered.back().frame, 1);
    effect.kind = WeaponVisualEffectKind::ExplosionAfterSmoke;
    effect.initial_lifetime = 190;
    effect.lifetime = 190;
    const auto lingering = GetExplosionSprites(effect, 1.0F);
    ASSERT_EQ(lingering.size(), 2U);
    EXPECT_EQ(lingering.front().kind, ExplosionSpriteKind::BigSmoke);
    EXPECT_EQ(lingering.back().kind, ExplosionSpriteKind::BigSmoke2);
    EXPECT_FLOAT_EQ(lingering.front().scale.x, 0.5F + 16.0F / 240.0F);
}

TEST(WeaponPresentationTest, ExplosionsKeepReferenceLifetimesAndDoNotScaleWithDamageRadius)
{
    auto world = std::make_shared<World>();
    auto state = world->GetStateManager();
    WeaponVisualEffects effects;
    const ProjectilePhysicsEvent event{ .kind = ProjectileEventKind::ProjectileExploded,
                                        .projectile_id = 10,
                                        .style = BulletType::FragGrenade,
                                        .weapon = WeaponType::FragGrenade,
                                        .position = { 100.0F, 100.0F },
                                        .radius = 1000.0F };
    effects.Update(*state, std::vector{ event });
    ASSERT_EQ(effects.GetEffects().size(), 3U);
    EXPECT_EQ(effects.GetEffects().back().initial_lifetime, 48);
    EXPECT_EQ(GetExplosionSprites(effects.GetEffects().back(), 1.0F).back().scale, glm::vec2(1.0F));
    for (unsigned int tick = 1; tick <= 47; ++tick) {
        state->SetGameTick(tick);
        effects.Update(*state, {});
    }
    ASSERT_EQ(effects.GetEffects().size(), 3U);
    EXPECT_EQ(effects.GetEffects().back().lifetime, 1);
    state->SetGameTick(48);
    effects.Update(*state, {});
    ASSERT_EQ(effects.GetEffects().size(), 2U);
    EXPECT_EQ(effects.GetEffects().back().kind, WeaponVisualEffectKind::ExplosionSmoke);
}

TEST(WeaponPresentationTest, ClusterBreakupAndM2UseSmokeInsteadOfFullSizeFireball)
{
    auto world = std::make_shared<World>();
    auto state = world->GetStateManager();
    for (const bool m2 : { false, true }) {
        WeaponVisualEffects effects;
        const ProjectilePhysicsEvent event{ .kind = m2 ? ProjectileEventKind::ProjectileExploded
                                                       : ProjectileEventKind::ProjectileFragmented,
                                            .projectile_id = 10,
                                            .style = m2 ? BulletType::M2Bullet
                                                        : BulletType::ClusterGrenade };
        effects.Update(*state, std::vector{ event });
        ASSERT_EQ(effects.GetEffects().size(), 1U);
        EXPECT_EQ(effects.GetEffects().front().kind, WeaponVisualEffectKind::ClusterSmoke);
        EXPECT_EQ(effects.GetEffects().front().initial_lifetime, 55);
    }
}

TEST(FullClientServerReconciliationTest, BackgroundAndFlagCarryConvergeUnderChangingLatency)
{
    for (bool realistic : { false, true }) {
        SCOPED_TRACE(realistic);
        constexpr std::uint8_t PLAYER_ID = 1;
        const auto map =
          SoldankTesting::MapBuilder::Empty()
            ->AddPolygon(
              { -4000.0F, 0.0F }, { 4000.0F, 0.0F }, { 4000.0F, 60.0F }, PMSPolygonType::Normal)
            ->AddPolygon(
              { -4000.0F, 0.0F }, { 4000.0F, 60.0F }, { -4000.0F, 60.0F }, PMSPolygonType::Normal)
            ->AddPolygon(
              { -200, -500 }, { 200, -500 }, { 0, 500 }, PMSPolygonType::BackgroundTransition)
            ->AddPolygon({ -1200, -500 }, { 1200, -500 }, { 0, 700 }, PMSPolygonType::Background)
            ->Build();
        auto client_world = std::make_shared<World>();
        auto server_world = std::make_shared<World>();
        client_world->GetStateManager()->SetRealistic(realistic);
        server_world->GetStateManager()->SetRealistic(realistic);
        const float spawn_y = map->GetPolygons().front().vertices.front().y - 34.0F;
        const std::vector<std::pair<unsigned int, glm::vec2>> positions{ { PLAYER_ID,
                                                                           { 0.0F, spawn_y } } };
        InitializeWorld(*client_world, *map, positions);
        InitializeWorld(*server_world, *map, positions);
        client_world->GetStateManager()->SetAuthoritativeProjectiles(false);
        auto client_state = std::make_shared<ClientState>();
        client_state->client_soldier_id = PLAYER_ID;
        client_state->network.server_reconciliation = true;
        client_state->network.client_side_prediction = true;
        auto bridge = std::make_shared<DeterministicNetworkBridge>(
          std::vector<LatencyPhase>{ { 0, 77 }, { 250, 280 }, { 500, 445 } });
        auto endpoint = bridge->CreateClientEndpoint(PLAYER_ID);
        auto server = std::static_pointer_cast<IGameServer>(bridge);
        PlayerSessionManager sessions;
        ServerCommandQueues queues;
        auto server_dispatcher = std::make_shared<NetworkEventDispatcher>(
          std::vector<std::shared_ptr<INetworkEventHandler>>{
            std::make_shared<SoldierInputNetworkEventHandler>(server, sessions, queues) });
        auto client_dispatcher = std::make_shared<NetworkEventDispatcher>(
          std::vector<std::shared_ptr<INetworkEventHandler>>{
            std::make_shared<SoldierStateNetworkEventHandler>(client_world, client_state),
            std::make_shared<PolygonEffectNetworkEventHandler>(client_world),
            std::make_shared<ProjectilePhysicsNetworkEventHandler>(client_world, client_state),
            std::make_shared<ProjectileSpawnNetworkEventHandler>(client_world),
            std::make_shared<ItemStateNetworkEventHandler>(client_world) });
        CoreEventsConnectionNotifier::ObserveAll(
          bridge.get(), server_world->GetWorldEvents(), server_world->GetPhysicsEvents());
        NetworkClientSession client_session(
          *endpoint, client_dispatcher, *client_world, *client_state);
        ReplicationService replication(*bridge, sessions);
        bool saw_carrier = false;
        bool saw_transition = false;
        for (std::uint64_t step = 0; step < 1000; ++step) {
            bridge->SetCurrentStep(step);
            client_state->network.target_input_delay_ticks =
              std::max(client_state->network.target_input_delay_ticks,
                       CalculateInputDelayTicks(bridge->GetCurrentRoundTripTime()));
            bridge->DeliverMessagesToServer(server_dispatcher);
            auto state = server_world->GetStateManager();
            if (step == 150) {
                auto& soldier = state->GetSoldier(PLAYER_ID);
                auto& flag = state->CreateItem(soldier.particle.position, 255, ItemType::AlphaFlag);
                ASSERT_TRUE(FlagCarry::TryAcquire(*state, soldier, flag));
            }
            if (step == 550) {
                state->ReleaseSoldierFlags(PLAYER_ID);
            }
            const auto tick = state->GetGameTick();
            auto inputs = queues.SelectPlayerInputsForSimulation(tick);
            const std::vector<SimulationCommand> commands;
            const auto result =
              server_world->Tick({ .tick = tick, .player_inputs = inputs, .commands = commands });
            replication.OnSimulationEvents(result.events);
            for (const auto& input : inputs) {
                sessions.MarkInputApplied(input.soldier_id, input.input_sequence_id);
            }
            replication.BroadcastTick(*state);
            state->SetGameTick(tick + 1);
            client_session.UpdateBeforeWorldTick();
            client_world->GetStateManager()->TransformSoldier(PLAYER_ID, [&](Soldier& soldier) {
                soldier.control.right = step > 180 && step < 350;
                soldier.control.left = step > 400 && step < 600;
            });
            client_session.SendSoldierInput(PLAYER_ID, { 1000.0F, -100.0F });
            const auto client_tick = client_world->GetStateManager()->GetGameTick();
            TickWorldWithNoInput(*client_world, client_tick);
            client_session.StorePredictedSoldierSnapshot(PLAYER_ID);
            saw_carrier = saw_carrier || client_world->GetSoldier(PLAYER_ID).is_holding_flags;
            saw_transition =
              saw_transition || client_world->GetSoldier(PLAYER_ID).polygon_background.phase ==
                                  PolygonBackgroundPhase::Transition;
        }
        EXPECT_EQ(bridge->GetDispatchFailureCount(), 0U);
        EXPECT_TRUE(saw_carrier);
        EXPECT_TRUE(saw_transition);
        EXPECT_EQ(client_world->GetSoldier(PLAYER_ID).is_holding_flags,
                  server_world->GetSoldier(PLAYER_ID).is_holding_flags);
        EXPECT_EQ(client_world->GetSoldier(PLAYER_ID).polygon_background,
                  server_world->GetSoldier(PLAYER_ID).polygon_background);
        EXPECT_LE(glm::distance(client_world->GetSoldier(PLAYER_ID).particle.position,
                                server_world->GetSoldier(PLAYER_ID).particle.position),
                  1.5F)
          << "client=" << client_world->GetSoldier(PLAYER_ID).particle.position.x << ","
          << client_world->GetSoldier(PLAYER_ID).particle.position.y
          << " server=" << server_world->GetSoldier(PLAYER_ID).particle.position.x << ","
          << server_world->GetSoldier(PLAYER_ID).particle.position.y;
    }
}
