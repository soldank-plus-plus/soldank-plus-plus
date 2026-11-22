module;
#include <memory>
#include <cstdint>
#include <unordered_map>

export module Networking.EventHandlers.EmoteRequestNetworkEventHandler;
import Networking.IGameServer;
import Runtime.ServerCommandQueues;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Animations.EmoteTypes;
import Shared.Core.Utility.SerialNumber;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;

export namespace Soldank
{
class EmoteRequestNetworkEventHandler : public NetworkEventHandlerBase<EmoteRequestPacket>
{
public:
    EmoteRequestNetworkEventHandler(const std::shared_ptr<IGameServer>& server,
                                    ServerCommandQueues& queues)
        : server_(server)
        , queues_(queues)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::EmoteRequest; }
    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int connection,
                                                       EmoteRequestPacket packet) override
    {
        const auto id = server_->GetSoldierIdFromConnectionId(connection);
        if (!IsValidEmote(packet.emote) || id >= 32) {
            return NetworkEventHandlerResult::Failure;
        }
        const auto found = last_requests_.find(connection);
        if (found != last_requests_.end() &&
            !IsSerialNumberNewer(packet.request_id, found->second)) {
            return NetworkEventHandlerResult::Success;
        }
        last_requests_[connection] = packet.request_id;
        queues_.EnqueueSimulationCommand(
          EmoteCommand{ static_cast<std::uint8_t>(id), packet.emote });
        return NetworkEventHandlerResult::Success;
    }
    std::shared_ptr<IGameServer> server_;
    ServerCommandQueues& queues_;
    std::unordered_map<unsigned int, std::uint32_t> last_requests_;
};
} // namespace Soldank
