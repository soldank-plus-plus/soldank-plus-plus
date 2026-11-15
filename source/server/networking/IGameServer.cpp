module;
#include <span>
#include <vector>
#include <string_view>

export module Networking.IGameServer;

import Shared.Networking.NetworkMessage;
export import Server.Chat.ChatService;

export namespace Soldank
{
class IGameServer
{
public:
    virtual ~IGameServer() = default;
    virtual void Update() = 0;

    virtual void SendNetworkMessage(unsigned int connection_id,
                                    const NetworkMessage& network_message) = 0;
    virtual void SendNetworkMessageToAll(const NetworkMessage& network_message) = 0;

    virtual bool BroadcastSystemMessage(std::string_view /*text*/) { return false; }
    virtual bool SendSystemMessage(std::span<const ChatPeer> /*recipients*/,
                                   std::string_view /*text*/)
    {
        return false;
    }
    virtual std::vector<ChatPeer> GetChatPeers() const { return {}; }

    virtual unsigned int GetSoldierIdFromConnectionId(unsigned int connection_id) = 0;
};
} // namespace Soldank
