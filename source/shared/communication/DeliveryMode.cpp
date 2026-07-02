module;

#include <cstdint>

export module Shared.Networking.DeliveryMode;

export namespace Soldank
{
enum class DeliveryMode : std::uint8_t
{
    Unreliable,
    Reliable
};
} // namespace Soldank
