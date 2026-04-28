module;

#include <functional>
#include <utility>
#include <vector>

export module TransformSceneriesMapEditorAction;

import MapEditorAction;
import ClientState;

import Shared.Core.State.StateManager;
import Shared.Core.Map.PMSStructs;

export namespace Soldank
{
class TransformSceneriesMapEditorAction final : public MapEditorAction
{
public:
    TransformSceneriesMapEditorAction(
      const std::vector<std::pair<unsigned int, PMSScenery>>& old_sceneries,
      const std::function<PMSScenery(const PMSScenery&)>& transform_function)
        : old_sceneries_(old_sceneries)
        , transform_function_(transform_function)
    {
    }

    bool CanExecute(const ClientState& /*client_state*/,
                    const StateManager& /*game_state_manager*/) final
    {
        return true;
    }

    void Execute(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        std::vector<std::pair<unsigned int, PMSScenery>> new_sceneries;
        for (const auto& old_scenery : old_sceneries_) {
            PMSScenery new_scenery = transform_function_(old_scenery.second);
            new_sceneries.emplace_back(old_scenery.first, new_scenery);
        }
        game_state_manager.GetMap().SetSceneriesById(new_sceneries);
    }

    void Undo(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        game_state_manager.GetMap().SetSceneriesById(old_sceneries_);
    }

private:
    std::vector<std::pair<unsigned int, PMSScenery>> old_sceneries_;
    std::function<PMSScenery(const PMSScenery&)> transform_function_;
};
} // namespace Soldank
