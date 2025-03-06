#ifndef __ADD_POLYGON_MAP_EDITOR_ACTION_HPP__
#define __ADD_POLYGON_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class AddPolygonMapEditorAction final : public MapEditorAction
{
public:
    AddPolygonMapEditorAction(const PMSPolygon& new_polygon);

    bool CanExecute(const ClientState& client_state, const StateManager& game_state_manager) final;
    void Execute(ClientState& client_state, StateManager& game_state_manager) final;
    void Undo(ClientState& client_state, StateManager& game_state_manager) final;

private:
    PMSPolygon added_polygon_;
};
} // namespace Soldank

#endif
