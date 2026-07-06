module;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <functional>

export module Shared.Core.Loop.PlatformFixedTimestepLoop;

import Shared.Core.Loop.FixedTimestepRunner;
import Shared.Core.Loop.NativeFixedTimestepLoop;

export namespace Soldank
{
class PlatformFixedTimestepLoop
{
public:
    void Run(FixedTimestepRunner& runner,
             const FixedTimestepCallbacks& callbacks,
             const std::function<int()>& get_fps_limit)
    {
#ifdef __EMSCRIPTEN__
        (void)runner;
        auto* loop_state = new EmscriptenLoopState{
            .runner = FixedTimestepRunner{},
            .callbacks = callbacks,
        };
        const int emscripten_fps = get_fps_limit && get_fps_limit() > 0 ? get_fps_limit() : 0;
        emscripten_set_main_loop_arg(
          [](void* arg) {
              auto* loop_state = static_cast<EmscriptenLoopState*>(arg);
              if (!loop_state->runner.RunIteration(loop_state->callbacks)) {
                  emscripten_cancel_main_loop();
                  delete loop_state;
              }
          },
          loop_state,
          emscripten_fps,
          false);
#else
        native_loop_.Run(runner, callbacks, get_fps_limit);
#endif
    }

private:
#ifdef __EMSCRIPTEN__
    struct EmscriptenLoopState
    {
        FixedTimestepRunner runner;
        FixedTimestepCallbacks callbacks;
    };
#else
    NativeFixedTimestepLoop native_loop_;
#endif
};
} // namespace Soldank
