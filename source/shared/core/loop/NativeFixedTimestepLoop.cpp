module;

#include <algorithm>
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
    using Clock = std::chrono::system_clock;

    void LimitFrameRate(int fps_limit)
    {
        if (fps_limit == 0) {
            last_render_time_ = Clock::now();
            return;
        }

        std::chrono::duration<double> render_time_delta = Clock::now() - last_render_time_;
        const double render_time_limit = 1.0 / static_cast<double>(fps_limit);

        while (render_time_delta.count() <= render_time_limit) {
            const double time_to_wait_seconds = render_time_limit - render_time_delta.count();
            const int time_to_wait_ms =
              std::max(0, static_cast<int>(1000.0 * time_to_wait_seconds - 0.2));

            std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::milliseconds(time_to_wait_ms));

            render_time_delta = Clock::now() - last_render_time_;
        }

        last_render_time_ = Clock::now();
    }

    Clock::time_point last_render_time_ = Clock::now();
};
} // namespace Soldank
