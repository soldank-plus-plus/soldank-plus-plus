#include "map_editor/actions/TransformSceneriesMapEditorAction.hpp"
#include "core/map/PMSStructs.hpp"

namespace Soldank
{
TransformSceneriesMapEditorAction::TransformSceneriesMapEditorAction(
  const std::vector<std::pair<unsigned int, PMSScenery>>& old_sceneries,
  const std::function<PMSScenery(const PMSScenery&)>& transform_function)
    : old_sceneries_(old_sceneries)
    , transform_function_(transform_function)
{
}

bool TransformSceneriesMapEditorAction::CanExecute(const ClientState& /*client_state*/,
                                                   const StateManager& /*game_state_manager*/)
{
    return true;
}

void TransformSceneriesMapEditorAction::Execute(ClientState& /*client_state*/,
                                                StateManager& game_state_manager)
{
    std::vector<std::pair<unsigned int, PMSScenery>> new_sceneries;
    for (const auto& old_scenery : old_sceneries_) {
        PMSScenery new_scenery = transform_function_(old_scenery.second);
        new_sceneries.emplace_back(old_scenery.first, new_scenery);
    }
    game_state_manager.GetMap().SetSceneriesById(new_sceneries);
}

void TransformSceneriesMapEditorAction::Undo(ClientState& /*client_state*/,
                                             StateManager& game_state_manager)
{
    game_state_manager.GetMap().SetSceneriesById(old_sceneries_);
}
} // namespace Soldank
