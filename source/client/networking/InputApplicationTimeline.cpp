module;

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>

export module Networking.InputApplicationTimeline;

import Shared.Core.Utility.SerialNumber;

export namespace Soldank
{
std::uint32_t CalculateInputDelayTicks(std::uint16_t round_trip_time_milliseconds)
{
    constexpr std::uint32_t TICKS_PER_SECOND = 60;
    constexpr std::uint32_t MILLISECONDS_PER_SECOND = 1000;
    constexpr std::uint32_t MINIMUM_INPUT_DELAY_TICKS = 5;
    constexpr std::uint32_t JITTER_MARGIN_TICKS = 2;

    const std::uint32_t round_trip_ticks =
      (static_cast<std::uint32_t>(round_trip_time_milliseconds) * TICKS_PER_SECOND +
       MILLISECONDS_PER_SECOND - 1) /
      MILLISECONDS_PER_SECOND;
    return std::max(MINIMUM_INPUT_DELAY_TICKS, round_trip_ticks + JITTER_MARGIN_TICKS);
}

class InputApplicationTimeline
{
public:
    std::optional<std::uint32_t> Schedule(std::uint32_t client_tick,
                                          std::optional<std::int64_t> server_tick_offset,
                                          std::uint32_t input_delay_ticks)
    {
        if (!last_client_tick_.has_value()) {
            if (!server_tick_offset.has_value()) {
                return std::nullopt;
            }

            const std::uint32_t estimated_server_tick =
              AddSerialNumberOffset(client_tick, *server_tick_offset);
            const std::uint32_t apply_server_tick = estimated_server_tick + input_delay_ticks;
            last_client_tick_ = client_tick;
            last_apply_server_tick_ = apply_server_tick;
            applied_target_input_delay_ticks_ = input_delay_ticks;
            active_input_delay_ticks_ = input_delay_ticks;
            return apply_server_tick;
        }

        was_last_schedule_resynchronized_ = false;
        const std::uint32_t client_tick_delta = client_tick - *last_client_tick_;
        assert(client_tick_delta == 1);

        const std::uint32_t normal_apply_server_tick = *last_apply_server_tick_ + client_tick_delta;
        assert(normal_apply_server_tick - *last_apply_server_tick_ == client_tick_delta);

        std::uint32_t apply_server_tick = normal_apply_server_tick;
        if (input_delay_ticks > applied_target_input_delay_ticks_ &&
            server_tick_offset.has_value()) {
            const std::uint32_t estimated_server_tick =
              AddSerialNumberOffset(client_tick, *server_tick_offset);
            const std::uint32_t target_apply_server_tick =
              estimated_server_tick + input_delay_ticks;
            if (IsSerialNumberNewer(target_apply_server_tick, normal_apply_server_tick)) {
                apply_server_tick = target_apply_server_tick;
                was_last_schedule_resynchronized_ = true;
            }
            applied_target_input_delay_ticks_ = input_delay_ticks;
        }

        last_client_tick_ = client_tick;
        last_apply_server_tick_ = apply_server_tick;
        active_input_delay_ticks_ = CalculateActiveInputDelayTicks(
          client_tick, server_tick_offset, apply_server_tick, input_delay_ticks);
        return apply_server_tick;
    }

    bool WasLastScheduleResynchronized() const { return was_last_schedule_resynchronized_; }

    std::uint32_t GetActiveInputDelayTicks() const { return active_input_delay_ticks_; }

    void Reset()
    {
        last_client_tick_.reset();
        last_apply_server_tick_.reset();
        applied_target_input_delay_ticks_ = 0;
        active_input_delay_ticks_ = 0;
        was_last_schedule_resynchronized_ = false;
    }

private:
    static std::uint32_t CalculateActiveInputDelayTicks(
      std::uint32_t client_tick,
      std::optional<std::int64_t> server_tick_offset,
      std::uint32_t apply_server_tick,
      std::uint32_t fallback_input_delay_ticks)
    {
        if (!server_tick_offset.has_value()) {
            return fallback_input_delay_ticks;
        }

        const std::uint32_t estimated_server_tick =
          AddSerialNumberOffset(client_tick, *server_tick_offset);
        return SerialNumberForwardDistance(estimated_server_tick, apply_server_tick);
    }

    std::optional<std::uint32_t> last_client_tick_;
    std::optional<std::uint32_t> last_apply_server_tick_;
    std::uint32_t applied_target_input_delay_ticks_ = 0;
    std::uint32_t active_input_delay_ticks_ = 0;
    bool was_last_schedule_resynchronized_ = false;
};
} // namespace Soldank
