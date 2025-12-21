module;

#include "core/map/PMSStructs.hpp"
#include "core/state/StateManager.hpp"

#include <functional>

export module TransformPolygonsMapEditorAction;

import MapEditorAction;
import ClientState;

export namespace Soldank
{
class TransformPolygonsMapEditorAction final : public MapEditorAction
{
public:
    TransformPolygonsMapEditorAction(
      const std::vector<std::pair<unsigned int, PMSPolygon>>& old_polygons,
      const std::function<PMSPolygon(const PMSPolygon&)>& transform_function)
        : old_polygons_(old_polygons)
        , transform_function_(transform_function)
    {
    }

    bool CanExecute(const ClientState& /*client_state*/,
                    const StateManager& /*game_state_manager*/) final
    {
        return true;
    }

    void Execute(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        std::vector<std::pair<unsigned int, PMSPolygon>> new_polygons;
        for (const auto& old_polygon : old_polygons_) {
            PMSPolygon new_polygon = transform_function_(old_polygon.second);
            new_polygons.emplace_back(old_polygon.first, new_polygon);
        }
        game_state_manager.GetMap().SetPolygonsById(new_polygons);
    }

    void Undo(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        game_state_manager.GetMap().SetPolygonsById(old_polygons_);
    }

private:
    std::vector<std::pair<unsigned int, PMSPolygon>> old_polygons_;
    std::function<PMSPolygon(const PMSPolygon&)> transform_function_;
};
} // namespace Soldank
