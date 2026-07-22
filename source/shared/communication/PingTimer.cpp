module;

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>

export module Shared.Networking.PingTimer;

export namespace Soldank
{
class PingTimer
{
public:
    void Start()
    {
        start_time_ = std::chrono::system_clock::now();
        is_running_ = true;
    }

    void Stop()
    {
        Update();
        is_running_ = false;
        last_completed_ping_ = last_ping_;
    }
    void Update()
    {
        auto current_time = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = (current_time - start_time_);
        last_ping_ = (std::uint16_t)(diff.count() * 1000.0);
        std::uint16_t run_time_threshold_milliseconds = RUN_TIME_THRESHOLD_SECONDS * 1000.0;
        last_ping_ = std::min(last_ping_, run_time_threshold_milliseconds);
    }

    std::uint16_t GetLastPingMeasure() const { return last_ping_; }
    std::optional<std::uint16_t> GetLastCompletedPingMeasure() const
    {
        return last_completed_ping_;
    }
    bool IsRunning() const { return is_running_; }
    bool IsOverThreshold() const
    {
        auto current_time = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = (current_time - start_time_);
        return diff.count() > RUN_TIME_THRESHOLD_SECONDS;
    }

private:
    constexpr const static double RUN_TIME_THRESHOLD_SECONDS = 9.999;

    std::chrono::time_point<std::chrono::system_clock> start_time_;
    std::uint16_t last_ping_{};
    std::optional<std::uint16_t> last_completed_ping_;
    bool is_running_{};
};
} // namespace Soldank
