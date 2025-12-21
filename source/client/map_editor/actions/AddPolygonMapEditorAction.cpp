module;

#include "core/map/PMSStructs.hpp"
#include "core/state/StateManager.hpp"

export module AddPolygonMapEditorAction;

import MapEditorAction;
import ClientState;

export namespace Soldank
{
class AddPolygonMapEditorAction final : public MapEditorAction
{
public:
    AddPolygonMapEditorAction(const PMSPolygon& new_polygon)
        : added_polygon_(new_polygon)
    {
    }

    bool CanExecute(const ClientState& /*client_state*/,
                    const StateManager& game_state_manager) final
    {
        return game_state_manager.GetConstMap().GetPolygonsCount() + 1 <= MAX_POLYGONS_COUNT;
    }

    void Execute(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        added_polygon_ = game_state_manager.GetMap().AddNewPolygon(added_polygon_);
    }

    void Undo(ClientState& client_state, StateManager& game_state_manager) final
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

private:
    PMSPolygon added_polygon_;
};
} // namespace Soldank
