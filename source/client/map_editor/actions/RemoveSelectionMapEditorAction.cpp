#include "map_editor/actions/RemoveSelectionMapEditorAction.hpp"

namespace Soldank
{
RemoveSelectionMapEditorAction::RemoveSelectionMapEditorAction(const ClientState& client_state,
                                                               Map& map)
{
    for (const auto& selected_polygon_vertices :
         client_state.map_editor_state.selected_polygon_vertices) {
        if (selected_polygon_vertices.second.all()) {
            polygon_ids_to_remove_.push_back(selected_polygon_vertices.first);
            removed_polygons_.push_back(map.GetPolygons().at(selected_polygon_vertices.first));
        }
    }
}

void RemoveSelectionMapEditorAction::Execute(ClientState& client_state, Map& map)
{
    map.RemovePolygonsById(polygon_ids_to_remove_);
    client_state.map_editor_state.selected_polygon_vertices.clear();
    client_state.map_editor_state.selected_scenery_ids.clear();
    client_state.map_editor_state.selected_spawn_point_ids.clear();
}

void RemoveSelectionMapEditorAction::Undo(ClientState& client_state, Map& map)
{
    map.AddPolygons(removed_polygons_);
}
} // namespace Soldank
