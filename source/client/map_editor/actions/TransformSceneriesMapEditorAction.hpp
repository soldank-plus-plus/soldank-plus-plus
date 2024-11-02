#ifndef __TRANSFORM_SCENERIES_MAP_EDITOR_ACTION_HPP__
#define __TRANSFORM_SCENERIES_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class TransformSceneriesMapEditorAction final : public MapEditorAction
{
public:
    TransformSceneriesMapEditorAction(
      const std::vector<std::pair<unsigned int, PMSScenery>>& old_sceneries,
      const std::function<PMSScenery(const PMSScenery&)>& transform_function);

    void Execute(ClientState& client_state, Map& map) final;
    void Undo(ClientState& client_state, Map& map) final;

private:
    std::vector<std::pair<unsigned int, PMSScenery>> old_sceneries_;
    std::function<PMSScenery(const PMSScenery&)> transform_function_;
};
} // namespace Soldank

#endif
