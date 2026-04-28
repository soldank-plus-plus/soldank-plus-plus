module;

#include <core/math/Glm.hpp>

#include <cstdint>
#include <memory>

export module Networking.SpawnSoldierNetworkEventHandler;

import Shared.Core.IWorld;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;

import Extern.Spdlog;

export namespace Soldank
{
class SpawnSoldierNetworkEventHandler : public NetworkEventHandlerBase<std::uint8_t, float, float>
{
public:
    SpawnSoldierNetworkEventHandler(const std::shared_ptr<IWorld>& world)
        : world_(world)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::SpawnSoldier; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender_connection_id*/,
                                                       std::uint8_t soldier_id,
                                                       float spawn_position_x,
                                                       float spawn_position_y) override
    {
        glm::vec2 spawn_position = { spawn_position_x, spawn_position_y };
        Spdlog::info(
          "OnSpawnSoldier: {}, ({}, {})", soldier_id, spawn_position.x, spawn_position.y);
        world_->SpawnSoldier(soldier_id, spawn_position);
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
