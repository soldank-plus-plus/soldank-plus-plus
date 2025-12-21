module;

#include "core/map/PMSConstants.hpp"
#include "core/map/PMSStructs.hpp"
#include "core/state/StateManager.hpp"

#include <string>

export module AddSceneryMapEditorAction;

import MapEditorAction;
import ClientState;

export namespace Soldank
{
class AddSceneryMapEditorAction final : public MapEditorAction
{
public:
    AddSceneryMapEditorAction(const PMSScenery& new_scenery, std::string file_name)
        : added_scenery_(new_scenery)
        , file_name_(std::move(file_name))
    {
    }

    bool CanExecute(const ClientState& /*client_state*/,
                    const StateManager& game_state_manager) final
    {
        return game_state_manager.GetConstMap().GetSceneryInstances().size() + 1 <=
               MAX_SCENERIES_COUNT;

        return true;
    }

    void Execute(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        added_scenery_id_ = game_state_manager.GetMap().AddNewScenery(added_scenery_, file_name_);
    }

    void Undo(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        added_scenery_ = game_state_manager.GetMap().RemoveSceneryById(added_scenery_id_);
        // TODO: remove from selection when implemented
    }

private:
    PMSScenery added_scenery_;
    std::string file_name_;
    unsigned int added_scenery_id_{};
};
} // namespace Soldank
