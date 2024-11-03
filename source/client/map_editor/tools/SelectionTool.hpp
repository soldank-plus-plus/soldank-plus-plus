#ifndef __SELECTION_TOOL_HPP__
#define __SELECTION_TOOL_HPP__

#include "map_editor/actions/MapEditorAction.hpp"
#include "map_editor/tools/Tool.hpp"

#include "core/utility/Observable.hpp"

#include "core/math/Glm.hpp"
#include "rendering/ClientState.hpp"

#include <memory>

namespace Soldank
{
class SelectionTool final : public Tool
{
public:
    ~SelectionTool() final = default;

    void OnSelect(ClientState& client_state, const State& game_state) final;
    void OnUnselect(ClientState& client_state) final;

    void OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) final;
    void OnSceneLeftMouseButtonRelease(ClientState& client_state, const State& game_state) final;
    void OnSceneRightMouseButtonClick(ClientState& client_state) final;
    void OnSceneRightMouseButtonRelease() final;
    void OnMouseScreenPositionChange(ClientState& client_state,
                                     glm::vec2 last_mouse_position,
                                     glm::vec2 new_mouse_position) final;
    void OnMouseMapPositionChange(ClientState& client_state,
                                  glm::vec2 last_mouse_position,
                                  glm::vec2 new_mouse_position,
                                  const State& game_state) final;
    void OnModifierKey1Pressed(ClientState& client_state) final;
    void OnModifierKey1Released(ClientState& client_state) final;
    void OnModifierKey2Pressed(ClientState& client_state) final;
    void OnModifierKey2Released(ClientState& client_state) final;
    void OnModifierKey3Pressed(ClientState& client_state) final;
    void OnModifierKey3Released(ClientState& client_state) final;

private:
    enum class SelectionMode
    {
        SingleSelection = 0,
        AddToSelection,
        RemoveFromSelection,
    };

    enum class NextObjectTypeToSelect
    {
        Polygon = 0,
        Scenery,
        SpawnPoint,
        Soldier,
    };

    void SelectNextSingleObject(ClientState& client_state, const State& game_state);
    void SelectNextObject(ClientState& client_state,
                          const State& game_state,
                          unsigned int start_index,
                          NextObjectTypeToSelect next_object_type_to_select);
    void AddFirstFoundObjectToSelection(ClientState& client_state, const State& game_state);
    bool AddFirstFoundPolygonToSelection(ClientState& client_state, const State& game_state);
    bool AddFirstFoundSceneryToSelection(ClientState& client_state, const State& game_state);
    bool AddFirstFoundSpawnPointToSelection(ClientState& client_state, const State& game_state);
    bool AddFirstFoundSoldierToSelection(ClientState& client_state, const State& game_state);
    void RemoveLastFoundObjectFromSelection(ClientState& client_state, const State& game_state);

    bool IsMouseInSpawnPoint(const ClientState& client_state,
                             const glm::vec2& spawn_point_position) const;
    bool IsMouseInSoldier(const glm::vec2& soldier_position) const;
    void SetSelectionMode(SelectionMode new_selection_mode, ClientState& client_state);
    static NextObjectTypeToSelect GetNextObjectTypeToSelect(
      NextObjectTypeToSelect current_object_type_to_select,
      const State& game_state);

    glm::vec2 mouse_map_position_;

    SelectionMode current_selection_mode_;
};
} // namespace Soldank

#endif
