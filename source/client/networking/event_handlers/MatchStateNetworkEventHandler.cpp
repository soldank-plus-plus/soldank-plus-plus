module;

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <filesystem>
#include <exception>
#include <cstdint>

export module Networking.MatchStateNetworkEventHandler;

import Shared.Core.IWorld;
import Shared.Core.State.StateManager;
import Shared.Core.GameModes.GameModeBuilder;
import Shared.Core.GameModes.GameModeRuntime;
import Shared.Core.Utility.SerialNumber;
import Shared.Networking.GameModeProtocol;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkEventDispatcher;

export namespace Soldank
{
class MatchStateNetworkEventHandler final : public NetworkEventHandlerBase<MatchStatePacket>
{
public:
    explicit MatchStateNetworkEventHandler(std::shared_ptr<IWorld> world)
        : world_(std::move(world))
    {
    }

protected:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::MatchState; }
    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender*/,
                                                       MatchStatePacket packet) override
    {
        if (!IsValidMatchStatePacket(packet)) {
            return NetworkEventHandlerResult::Failure;
        }
        if (latest_epoch_ && packet.state.epoch != *latest_epoch_ &&
            !IsSerialNumberNewer(packet.state.epoch, *latest_epoch_)) {
            return NetworkEventHandlerResult::Success;
        }
        if (latest_epoch_ && packet.state.epoch == *latest_epoch_ && latest_tick_) {
            if (IsSerialNumberOlder(packet.server_tick, *latest_tick_) ||
                (packet.server_tick == *latest_tick_ && latest_revision_ &&
                 !IsSerialNumberNewer(packet.state.revision, *latest_revision_))) {
                return NetworkEventHandlerResult::Success;
            }
        }
        const auto state = world_->GetStateManager();
        if (!latest_epoch_ || packet.state.epoch != *latest_epoch_) {
            if (!LoadMap(*state, packet.map_path.data())) {
                return NetworkEventHandlerResult::Failure;
            }
            GameModeBuilder builder(std::string(packet.mode_id.data()));
            builder.DisplayName(std::string(packet.display_name.data()))
              .Teams(packet.team_count)
              .FriendlyFire(packet.friendly_fire)
              .ScoreLimit(packet.score_limit == 0 ? std::nullopt
                                                  : std::optional<int>(packet.score_limit))
              .TimeLimit(packet.time_limit == 0 ? std::nullopt
                                                : std::optional<std::uint32_t>(packet.time_limit));
            if (packet.survival) {
                builder.Respawn({ .kind = RespawnKind::RoundOnly }).Rounds({ .enabled = true });
            }
            state->ConfigureGameMode(builder.Build());
        }
        state->GetGameMode()->ApplySnapshot(packet.state);
        latest_epoch_ = packet.state.epoch;
        latest_tick_ = packet.server_tick;
        latest_revision_ = packet.state.revision;
        return NetworkEventHandlerResult::Success;
    }

private:
    static bool LoadMap(StateManager& state, const char* name)
    {
        const std::filesystem::path map_path(name);
        if (map_path.empty()) {
            return true;
        }
        if (map_path.is_absolute() || map_path.extension() != ".pms") {
            return false;
        }
        for (const auto& component : map_path) {
            if (component == "..") {
                return false;
            }
        }
        if (state.GetLoadedMapPath() == map_path.generic_string()) {
            return true;
        }
        try {
            state.LoadMapDocument(map_path);
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }
    std::shared_ptr<IWorld> world_;
    std::optional<std::uint32_t> latest_epoch_;
    std::optional<std::uint32_t> latest_tick_;
    std::optional<std::uint32_t> latest_revision_;
};
} // namespace Soldank
