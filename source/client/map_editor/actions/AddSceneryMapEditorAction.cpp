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
                                           const StateManager& game_state_manager)
{
    return game_state_manager.GetConstMap().GetSceneryInstances().size() + 1 <= MAX_SCENERIES_COUNT;

    return true;
}

void AddSceneryMapEditorAction::Execute(ClientState& /*client_state*/,
                                        StateManager& game_state_manager)
{
    added_scenery_id_ = game_state_manager.GetMap().AddNewScenery(added_scenery_, file_name_);
}

void AddSceneryMapEditorAction::Undo(ClientState& /*client_state*/,
                                     StateManager& game_state_manager)
{
    added_scenery_ = game_state_manager.GetMap().RemoveSceneryById(added_scenery_id_);
    // TODO: remove from selection when implemented
}
} // namespace Soldank
