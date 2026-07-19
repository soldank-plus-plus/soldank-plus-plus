module;

#include <algorithm>
#include <chrono>
#include <functional>

export module Shared.Core.Loop.FixedTimestepRunner;

export namespace Soldank
{
struct FixedTimestepCallbacks
{
    std::function<bool()> should_continue;
    std::function<void()> before_frame;
    std::function<bool()> should_tick;
    std::function<void(double)> tick;
    std::function<void(unsigned int)> after_tick;
    std::function<void(double, int)> render;
    std::function<void(int, int)> report_stats;
};

class FixedTimestepRunner
{
public:
    bool RunIteration(const FixedTimestepCallbacks& callbacks)
    {
        if (callbacks.should_continue && !callbacks.should_continue()) {
            return false;
        }

        if (callbacks.before_frame) {
            callbacks.before_frame();
        }

        auto current_frame_time = Clock::now();
        std::chrono::duration<double> delta_time = current_frame_time - last_frame_time_;
        last_frame_time_ = current_frame_time;

        frame_count_since_last_fps_check_++;
        std::chrono::duration<double> fps_check_delta =
          current_frame_time - last_fps_check_time_;
        if (fps_check_delta.count() >= 1.0) {
            last_fps_ = frame_count_since_last_fps_check_;
            if (callbacks.report_stats) {
                callbacks.report_stats(frame_count_since_last_fps_check_, world_updates_);
            }
            frame_count_since_last_fps_check_ = 0;
            world_updates_ = 0;
            last_fps_check_time_ = current_frame_time;
        }

        while (time_accumulator_.count() >= FIXED_TIMESTEP_SECONDS) {
            time_accumulator_ -= std::chrono::duration<double>{ FIXED_TIMESTEP_SECONDS };

            if (!callbacks.should_tick || callbacks.should_tick()) {
                if (callbacks.tick) {
                    callbacks.tick(delta_time.count());
                }
                if (callbacks.after_tick) {
                    callbacks.after_tick(game_tick_ + 1);
                }
                world_updates_++;
                game_tick_++;
            }

            AccumulateElapsedTime();
        }

        double frame_percent = 1.0;
        if (!callbacks.should_tick || callbacks.should_tick()) {
            frame_percent = std::min(
              1.0, std::max(0.0, time_accumulator_.count() / FIXED_TIMESTEP_SECONDS));
        }

        if (callbacks.render) {
            callbacks.render(frame_percent, last_fps_);
        }

        AccumulateElapsedTime();
        return true;
    }

    unsigned int GetGameTick() const { return game_tick_; }

private:
    using Clock = std::chrono::steady_clock;
    static constexpr double FIXED_TIMESTEP_SECONDS = 1.0 / 60.0;

    void AccumulateElapsedTime()
    {
        auto time_current = Clock::now();
        time_accumulator_ += time_current - previous_accumulator_time_;
        previous_accumulator_time_ = time_current;
    }

    Clock::time_point last_frame_time_ = Clock::now();
    Clock::time_point last_fps_check_time_ = Clock::now();
    Clock::time_point previous_accumulator_time_ = Clock::now();
    std::chrono::duration<double> time_accumulator_{ 0 };
    unsigned int game_tick_ = 0;
    int world_updates_ = 0;
    int frame_count_since_last_fps_check_ = 0;
    int last_fps_ = 0;
};
} // namespace Soldank
