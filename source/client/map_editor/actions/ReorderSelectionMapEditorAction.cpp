module;

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <utility>
#include <vector>

export module ReorderSelectionMapEditorAction;

import MapEditorAction;
import ClientState;
import MapEditorState;

import Shared.Core.Map.Map;
import Shared.Core.Map.PMSStructs;
import Shared.Core.State.StateManager;

namespace
{
using namespace Soldank;

template<typename Value>
struct ReorderedValues
{
    std::vector<Value> values;
    std::vector<unsigned int> old_to_new_indices;
};

template<typename Value>
ReorderedValues<Value> ReorderValues(const std::vector<Value>& values,
                                     const std::vector<unsigned int>& selected_ids,
                                     SelectionLayerOrder layer_order)
{
    std::vector<bool> is_selected(values.size(), false);
    for (const unsigned int selected_id : selected_ids) {
        if (selected_id < values.size()) {
            is_selected.at(selected_id) = true;
        }
    }

    std::vector<std::pair<unsigned int, Value>> indexed_values;
    indexed_values.reserve(values.size());
    for (unsigned int index = 0; index < values.size(); ++index) {
        indexed_values.emplace_back(index, values.at(index));
    }

    switch (layer_order) {
        case SelectionLayerOrder::BringToFront:
            std::stable_partition(
              indexed_values.begin(), indexed_values.end(), [&is_selected](const auto& value) {
                  return !is_selected.at(value.first);
              });
            break;
        case SelectionLayerOrder::BringForward:
            for (std::size_t index = indexed_values.size(); index-- > 1;) {
                if (is_selected.at(indexed_values.at(index - 1).first) &&
                    !is_selected.at(indexed_values.at(index).first)) {
                    std::swap(indexed_values.at(index - 1), indexed_values.at(index));
                }
            }
            break;
        case SelectionLayerOrder::SendBackward:
            for (std::size_t index = 1; index < indexed_values.size(); ++index) {
                if (is_selected.at(indexed_values.at(index).first) &&
                    !is_selected.at(indexed_values.at(index - 1).first)) {
                    std::swap(indexed_values.at(index), indexed_values.at(index - 1));
                }
            }
            break;
        case SelectionLayerOrder::SendToBack:
            std::stable_partition(
              indexed_values.begin(), indexed_values.end(), [&is_selected](const auto& value) {
                  return is_selected.at(value.first);
              });
            break;
    }

    ReorderedValues<Value> reordered_values;
    reordered_values.values.reserve(values.size());
    reordered_values.old_to_new_indices.resize(values.size());
    for (unsigned int new_index = 0; new_index < indexed_values.size(); ++new_index) {
        const auto& [old_index, value] = indexed_values.at(new_index);
        reordered_values.values.push_back(value);
        reordered_values.old_to_new_indices.at(old_index) = new_index;
    }
    return reordered_values;
}

std::vector<unsigned int> GetPolygonIds(
  const std::vector<std::pair<unsigned int, std::bitset<3>>>& selected_polygons)
{
    std::vector<unsigned int> polygon_ids;
    polygon_ids.reserve(selected_polygons.size());
    for (const auto& [polygon_id, selected_vertices] : selected_polygons) {
        if (selected_vertices.any()) {
            polygon_ids.push_back(polygon_id);
        }
    }
    return polygon_ids;
}

void RemapPolygonSelection(std::vector<std::pair<unsigned int, std::bitset<3>>>& selection,
                           const std::vector<unsigned int>& old_to_new_indices)
{
    for (auto& selection_entry : selection) {
        unsigned int& polygon_id = selection_entry.first;
        if (polygon_id < old_to_new_indices.size()) {
            polygon_id = old_to_new_indices.at(polygon_id);
        }
    }
}

void NotifyPolygonSelection(ClientState& client_state, const Map& map)
{
    for (const auto& polygon : map.GetPolygons()) {
        const auto selection =
          std::find_if(client_state.map_editor_state.selected_polygon_vertices.begin(),
                       client_state.map_editor_state.selected_polygon_vertices.end(),
                       [&polygon](const auto& selected_polygon) {
                           return selected_polygon.first == polygon.id;
                       });
        client_state.map_editor_state.event_polygon_selected.Notify(
          polygon,
          selection == client_state.map_editor_state.selected_polygon_vertices.end()
            ? std::bitset<3>{}
            : selection->second);
    }
}

void RemapSelection(std::vector<unsigned int>& selection,
                    const std::vector<unsigned int>& old_to_new_indices)
{
    for (unsigned int& object_id : selection) {
        if (object_id < old_to_new_indices.size()) {
            object_id = old_to_new_indices.at(object_id);
        }
    }
}
} // namespace

export namespace Soldank
{
class ReorderSelectionMapEditorAction final : public MapEditorAction
{
public:
    ReorderSelectionMapEditorAction(const ClientState& client_state,
                                    const StateManager& game_state_manager,
                                    SelectionLayerOrder layer_order)
        : old_polygons_(game_state_manager.GetConstMap().GetPolygons())
        , old_sceneries_(game_state_manager.GetConstMap().GetSceneryInstances())
        , old_spawn_points_(game_state_manager.GetConstMap().GetSpawnPoints())
        , old_selected_polygon_vertices_(client_state.map_editor_state.selected_polygon_vertices)
        , old_selected_scenery_ids_(client_state.map_editor_state.selected_scenery_ids)
        , old_selected_spawn_point_ids_(client_state.map_editor_state.selected_spawn_point_ids)
    {
        const auto reordered_polygons =
          ReorderValues(old_polygons_, GetPolygonIds(old_selected_polygon_vertices_), layer_order);
        new_polygons_ = reordered_polygons.values;
        new_selected_polygon_vertices_ = old_selected_polygon_vertices_;
        RemapPolygonSelection(new_selected_polygon_vertices_,
                              reordered_polygons.old_to_new_indices);

        const auto reordered_sceneries =
          ReorderValues(old_sceneries_, old_selected_scenery_ids_, layer_order);
        new_sceneries_ = reordered_sceneries.values;
        new_selected_scenery_ids_ = old_selected_scenery_ids_;
        RemapSelection(new_selected_scenery_ids_, reordered_sceneries.old_to_new_indices);

        const auto reordered_spawn_points =
          ReorderValues(old_spawn_points_, old_selected_spawn_point_ids_, layer_order);
        new_spawn_points_ = reordered_spawn_points.values;
        new_selected_spawn_point_ids_ = old_selected_spawn_point_ids_;
        RemapSelection(new_selected_spawn_point_ids_, reordered_spawn_points.old_to_new_indices);
    }

    bool CanExecute(const ClientState& /*client_state*/,
                    const StateManager& /*game_state_manager*/) final
    {
        return !old_selected_polygon_vertices_.empty() || !old_selected_scenery_ids_.empty() ||
               !old_selected_spawn_point_ids_.empty();
    }

    void Execute(ClientState& client_state, StateManager& game_state_manager) final
    {
        game_state_manager.GetMap().ReplacePolygons(new_polygons_);
        game_state_manager.GetMap().ReplaceSceneries(new_sceneries_);
        game_state_manager.GetMap().ReplaceSpawnPoints(new_spawn_points_);
        client_state.map_editor_state.selected_polygon_vertices = new_selected_polygon_vertices_;
        client_state.map_editor_state.selected_scenery_ids = new_selected_scenery_ids_;
        client_state.map_editor_state.selected_spawn_point_ids = new_selected_spawn_point_ids_;
        NotifyPolygonSelection(client_state, game_state_manager.GetConstMap());
    }

    void Undo(ClientState& client_state, StateManager& game_state_manager) final
    {
        game_state_manager.GetMap().ReplacePolygons(old_polygons_);
        game_state_manager.GetMap().ReplaceSceneries(old_sceneries_);
        game_state_manager.GetMap().ReplaceSpawnPoints(old_spawn_points_);
        client_state.map_editor_state.selected_polygon_vertices = old_selected_polygon_vertices_;
        client_state.map_editor_state.selected_scenery_ids = old_selected_scenery_ids_;
        client_state.map_editor_state.selected_spawn_point_ids = old_selected_spawn_point_ids_;
        NotifyPolygonSelection(client_state, game_state_manager.GetConstMap());
    }

private:
    std::vector<PMSPolygon> old_polygons_;
    std::vector<PMSPolygon> new_polygons_;
    std::vector<PMSScenery> old_sceneries_;
    std::vector<PMSScenery> new_sceneries_;
    std::vector<PMSSpawnPoint> old_spawn_points_;
    std::vector<PMSSpawnPoint> new_spawn_points_;
    std::vector<std::pair<unsigned int, std::bitset<3>>> old_selected_polygon_vertices_;
    std::vector<std::pair<unsigned int, std::bitset<3>>> new_selected_polygon_vertices_;
    std::vector<unsigned int> old_selected_scenery_ids_;
    std::vector<unsigned int> new_selected_scenery_ids_;
    std::vector<unsigned int> old_selected_spawn_point_ids_;
    std::vector<unsigned int> new_selected_spawn_point_ids_;
};
} // namespace Soldank
