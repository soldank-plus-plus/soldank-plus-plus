module;

#include <cstdint>

export module Shared.Core.Utility.SerialNumber;

export namespace Soldank
{
bool IsSerialNumberNewer(std::uint32_t candidate, std::uint32_t reference)
{
    constexpr std::uint32_t HALF_RANGE = UINT32_MAX / 2U + 1U;
    const std::uint32_t forward_distance = candidate - reference;
    return forward_distance != 0 && forward_distance < HALF_RANGE;
}

bool IsSerialNumberOlder(std::uint32_t candidate, std::uint32_t reference)
{
    return IsSerialNumberNewer(reference, candidate);
}

std::uint32_t SerialNumberForwardDistance(std::uint32_t from, std::uint32_t to)
{
    return to - from;
}

std::int64_t SerialNumberSignedDistance(std::uint32_t from, std::uint32_t to)
{
    if (from == to) {
        return 0;
    }
    if (IsSerialNumberNewer(to, from)) {
        return static_cast<std::int64_t>(SerialNumberForwardDistance(from, to));
    }
    return -static_cast<std::int64_t>(SerialNumberForwardDistance(to, from));
}

std::uint32_t AddSerialNumberOffset(std::uint32_t value, std::int64_t offset)
{
    if (offset >= 0) {
        return value + static_cast<std::uint32_t>(offset);
    }
    return value - static_cast<std::uint32_t>(-offset);
}
} // namespace Soldank
