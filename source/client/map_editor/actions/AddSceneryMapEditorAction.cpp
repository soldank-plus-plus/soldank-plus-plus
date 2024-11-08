#include "map_editor/actions/AddSceneryMapEditorAction.hpp"
#include "core/map/PMSConstants.hpp"

namespace Soldank
{
AddSceneryMapEditorAction::AddSceneryMapEditorAction(const PMSScenery& new_scenery,
                                                     std::string file_name)
    : added_scenery_(new_scenery)
    , file_name_(std::move(file_name))
    , added_scenery_id_(0)
{
}

bool AddSceneryMapEditorAction::CanExecute(const ClientState& /*client_state*/,
                                           const State& game_state)
{
    return game_state.map.GetSceneryInstances().size() + 1 <= MAX_SCENERIES_COUNT;

    return true;
}

void AddSceneryMapEditorAction::Execute(ClientState& /*client_state*/, State& game_state)
{
    added_scenery_id_ = game_state.map.AddNewScenery(added_scenery_, file_name_);
}

void AddSceneryMapEditorAction::Undo(ClientState& /*client_state*/, State& game_state)
{
    added_scenery_ = game_state.map.RemoveSceneryById(added_scenery_id_);
    // TODO: remove from selection when implemented
}
} // namespace Soldank
