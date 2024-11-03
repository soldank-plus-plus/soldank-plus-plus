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

    void Execute(ClientState& client_state, State& game_state) final;
    void Undo(ClientState& client_state, State& game_state) final;

private:
    PMSPolygon added_polygon_;
};
} // namespace Soldank

#endif
