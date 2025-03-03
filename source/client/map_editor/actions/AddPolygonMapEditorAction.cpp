#include "map_editor/actions/AddPolygonMapEditorAction.hpp"

namespace Soldank
{
AddPolygonMapEditorAction::AddPolygonMapEditorAction(const PMSPolygon& new_polygon)
    : added_polygon_(new_polygon)
{
}

bool AddPolygonMapEditorAction::CanExecute(const ClientState& /*client_state*/,
                                           const StateManager& game_state_manager)
{
    return game_state_manager.GetConstMap().GetPolygonsCount() + 1 <= MAX_POLYGONS_COUNT;
}

void AddPolygonMapEditorAction::Execute(ClientState& /*client_state*/,
                                        StateManager& game_state_manager)
{
    added_polygon_ = game_state_manager.GetMap().AddNewPolygon(added_polygon_);
}

void AddPolygonMapEditorAction::Undo(ClientState& client_state, StateManager& game_state_manager)
{
    game_state_manager.GetMap().RemovePolygonById(added_polygon_.id);
    for (unsigned int i = 0; i < client_state.map_editor_state.selected_polygon_vertices.size();
         ++i) {
        if (client_state.map_editor_state.selected_polygon_vertices.at(i).first ==
            added_polygon_.id) {
            std::swap(client_state.map_editor_state.selected_polygon_vertices[i],
                      client_state.map_editor_state.selected_polygon_vertices.back());
            client_state.map_editor_state.selected_polygon_vertices.pop_back();
            break;
        }
    }
}
} // namespace Soldank
