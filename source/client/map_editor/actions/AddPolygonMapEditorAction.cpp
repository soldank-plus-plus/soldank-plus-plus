#include "map_editor/actions/AddPolygonMapEditorAction.hpp"

namespace Soldank
{
AddPolygonMapEditorAction::AddPolygonMapEditorAction(const PMSPolygon& new_polygon)
    : added_polygon_(new_polygon)
{
}

void AddPolygonMapEditorAction::Execute(ClientState& client_state, Map& map)
{
    added_polygon_ = map.AddNewPolygon(added_polygon_);
}

void AddPolygonMapEditorAction::Undo(ClientState& client_state, Map& map)
{
    map.RemovePolygonById(added_polygon_.id);
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
