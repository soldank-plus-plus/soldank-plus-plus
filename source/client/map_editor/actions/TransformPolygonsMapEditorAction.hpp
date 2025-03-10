#ifndef __TRANSFORM_POLYGONS_MAP_EDITOR_ACTION_HPP__
#define __TRANSFORM_POLYGONS_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class TransformPolygonsMapEditorAction final : public MapEditorAction
{
public:
    TransformPolygonsMapEditorAction(
      const std::vector<std::pair<unsigned int, PMSPolygon>>& old_polygons,
      const std::function<PMSPolygon(const PMSPolygon&)>& transform_function);

    bool CanExecute(const ClientState& client_state, const StateManager& game_state_manager) final;
    void Execute(ClientState& client_state, StateManager& game_state_manager) final;
    void Undo(ClientState& client_state, StateManager& game_state_manager) final;

private:
    std::vector<std::pair<unsigned int, PMSPolygon>> old_polygons_;
    std::function<PMSPolygon(const PMSPolygon&)> transform_function_;
};
} // namespace Soldank

#endif
