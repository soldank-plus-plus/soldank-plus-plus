#ifndef __ADD_SCENERY_MAP_EDITOR_ACTION_HPP__
#define __ADD_SCENERY_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

#include <string>

namespace Soldank
{
class AddSceneryMapEditorAction final : public MapEditorAction
{
public:
    AddSceneryMapEditorAction(const PMSScenery& new_scenery, std::string file_name);

    bool CanExecute(const ClientState& client_state, const StateManager& game_state_manager) final;
    void Execute(ClientState& client_state, StateManager& game_state_manager) final;
    void Undo(ClientState& client_state, StateManager& game_state_manager) final;

private:
    PMSScenery added_scenery_;
    std::string file_name_;
    unsigned int added_scenery_id_;
};
} // namespace Soldank

#endif
