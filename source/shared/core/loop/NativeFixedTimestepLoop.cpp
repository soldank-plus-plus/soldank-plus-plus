module;

#include <chrono>
#include <functional>
#include <thread>

export module Shared.Core.Loop.NativeFixedTimestepLoop;

import Shared.Core.Loop.FixedTimestepRunner;

export namespace Soldank
{
class NativeFixedTimestepLoop
{
public:
    void Run(FixedTimestepRunner& runner,
             const FixedTimestepCallbacks& callbacks,
             const std::function<int()>& get_fps_limit)
    {
        while (runner.RunIteration(callbacks)) {
            LimitFrameRate(get_fps_limit ? get_fps_limit() : 0);
        }
    }

private:
    using Clock = std::chrono::steady_clock;

    void LimitFrameRate(int fps_limit)
    {
        if (fps_limit == 0) {
            last_render_time_ = Clock::now();
            return;
        }

        const auto frame_duration = std::chrono::duration_cast<Clock::duration>(
          std::chrono::duration<double>{ 1.0 / static_cast<double>(fps_limit) });
        const Clock::time_point next_frame_time = last_render_time_ + frame_duration;
        std::this_thread::sleep_until(next_frame_time);

        last_render_time_ = Clock::now();
    }

    Clock::time_point last_render_time_ = Clock::now();
};
} // namespace Soldank
