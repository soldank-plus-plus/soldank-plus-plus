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
    AddSceneryMapEditorAction(const PMSScenery& new_scenery, const std::string& file_name);

    void Execute(ClientState& client_state, Map& map) final;
    void Undo(ClientState& client_state, Map& map) final;

private:
    PMSScenery added_scenery_;
    std::string file_name_;
    unsigned int added_scenery_id_;
};
} // namespace Soldank

#endif
