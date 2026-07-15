module;

#include <memory>

export module RenderPipeline;

import Application.ClientModes;
import ClientState;
import Scene;

import Shared.Core.State.StateManager;

export namespace Soldank
{
class RenderPipeline
{
public:
    RenderPipeline(const std::shared_ptr<StateManager>& game_state, ClientState& client_state)
        : scene_(game_state, client_state)
    {
    }
    ~RenderPipeline();

    void Render(const StateManager& game_state_manager,
                ClientState& client_state,
                ClientMode client_mode,
                EditorMode editor_mode,
                double frame_percent,
                int fps)
    {
        if (client_mode == ClientMode::MapEditor) {
            if (editor_mode == EditorMode::PlayTest) {
                scene_.RenderPlayTest(game_state_manager, client_state, frame_percent, fps);
            } else {
                scene_.RenderEditor(game_state_manager, client_state, frame_percent, fps);
            }
            return;
        }

        scene_.RenderGame(game_state_manager, client_state, frame_percent, fps);
    }

private:
    Scene scene_;
};
} // namespace Soldank

namespace Soldank
{
RenderPipeline::~RenderPipeline() {}
} // namespace Soldank
