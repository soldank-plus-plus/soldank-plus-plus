#include <gtest/gtest.h>

#include <memory>

import Shared.Core.World;
import Shared.Core.GameModes.BuiltinGameModes;
import Shared.Core.GameModes.GameModeRuntime;
import Shared.Networking.GameModeProtocol;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkMessage;
import Networking.MatchStateNetworkEventHandler;

using namespace Soldank;

namespace
{
class GameModeProtocolTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        server_ = std::make_shared<World>();
        server_->GetStateManager()->ConfigureGameMode(MakeCaptureTheFlag());
        client_ = std::make_shared<World>();
        client_->GetStateManager()->SetAuthoritativeProjectiles(false);
        handler_ = std::make_unique<MatchStateNetworkEventHandler>(client_);
    }
    NetworkEventHandlerResult Receive(const MatchStatePacket& packet)
    {
        return handler_->HandleNetworkMessage(0, { NetworkEvent::MatchState, packet });
    }
    World& Server() const { return *server_; }
    World& Client() const { return *client_; }

private:
    std::shared_ptr<World> server_;
    std::shared_ptr<World> client_;
    std::unique_ptr<MatchStateNetworkEventHandler> handler_;
};
} // namespace

TEST_F(GameModeProtocolTest, JoinReceivesResolvedSettingsAndScores)
{
    auto packet = MakeMatchStatePacket(*Server().GetStateManager());
    packet.state.teams.at(1) = 7;
    packet.score_limit = 20;
    EXPECT_EQ(Receive(packet), NetworkEventHandlerResult::Success);
    ASSERT_NE(Client().GetStateManager()->GetGameMode(), nullptr);
    EXPECT_EQ(Client().GetStateManager()->GetGameMode()->GetState().teams.at(1), 7);
    EXPECT_EQ(Client().GetStateManager()->GetGameMode()->Settings().score_limit, 20);
}

TEST_F(GameModeProtocolTest, SameTickNewerRevisionIsAccepted)
{
    auto packet = MakeMatchStatePacket(*Server().GetStateManager());
    Receive(packet);
    packet.state.revision = 1;
    packet.state.teams.at(1) = 2;
    EXPECT_EQ(Receive(packet), NetworkEventHandlerResult::Success);
    EXPECT_EQ(Client().GetStateManager()->GetGameMode()->GetState().teams.at(1), 2);
}

TEST_F(GameModeProtocolTest, OlderSnapshotsDoNotRollBackState)
{
    auto packet = MakeMatchStatePacket(*Server().GetStateManager());
    packet.server_tick = 10;
    packet.state.revision = 3;
    packet.state.teams.at(1) = 2;
    Receive(packet);
    --packet.server_tick;
    packet.state.teams.at(1) = 0;
    Receive(packet);
    EXPECT_EQ(Client().GetStateManager()->GetGameMode()->GetState().teams.at(1), 2);
}

TEST_F(GameModeProtocolTest, NewEpochReplacesStateAndRejectsPreviousMatch)
{
    auto packet = MakeMatchStatePacket(*Server().GetStateManager());
    const auto old = packet;
    Receive(packet);
    ++packet.state.epoch;
    packet.state.teams.at(1) = 3;
    Receive(packet);
    Receive(old);
    EXPECT_EQ(Client().GetStateManager()->GetGameMode()->GetState().teams.at(1), 3);
}

TEST_F(GameModeProtocolTest, InvalidVersionCannotMutateClientState)
{
    auto packet = MakeMatchStatePacket(*Server().GetStateManager());
    ++packet.version;
    EXPECT_EQ(Receive(packet), NetworkEventHandlerResult::Failure);
    EXPECT_EQ(Client().GetStateManager()->GetGameMode(), nullptr);
}

TEST_F(GameModeProtocolTest, SurvivalSnapshotDoesNotRunAuthoritativeRules)
{
    Server().GetStateManager()->ConfigureGameMode(WithSurvival(MakeCaptureTheFlag()));
    const auto packet = MakeMatchStatePacket(*Server().GetStateManager());
    EXPECT_EQ(Receive(packet), NetworkEventHandlerResult::Success);
    EXPECT_TRUE(Client().GetStateManager()->GetGameMode()->Settings().rounds.enabled);
    EXPECT_TRUE(Client().GetStateManager()->GetGameMode()->Settings().rules.empty());
}
