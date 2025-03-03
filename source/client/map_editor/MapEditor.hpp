#ifndef __MAP_EDITOR_HPP__
#define __MAP_EDITOR_HPP__

#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"
#include "map_editor/tools/Tool.hpp"
#include "map_editor/actions/MapEditorAction.hpp"

#include "rendering/ClientState.hpp"

#include "core/state/StateManager.hpp"
#include "core/math/Glm.hpp"

#include <deque>
#include <vector>
#include <memory>
#include <functional>

namespace Soldank
{
class MapEditor
{
public:
    MapEditor(ClientState& client_state, StateManager& game_state_manager);

    void Lock();
    void Unlock();

private:
    static const int ACTION_HISTORY_LIMIT = 50;

    void OnSelectNewTool(ToolType tool_type,
                         ClientState& client_state,
                         const StateManager& game_state_manager);
    void OnSceneLeftMouseButtonClick(ClientState& client_state,
                                     const StateManager& game_state_manager);
    void OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                       const StateManager& game_state_manager);
    void OnSceneRightMouseButtonClick(ClientState& client_state);
    void OnSceneRightMouseButtonRelease();
    void OnSceneMiddleMouseButtonClick(ClientState& client_state);
    void OnSceneMiddleMouseButtonRelease();
    void OnMouseScrollUp(ClientState& client_state) const;
    void OnMouseScrollDown(ClientState& client_state) const;
    void OnMouseScreenPositionChange(ClientState& client_state,
                                     glm::vec2 last_mouse_position,
                                     glm::vec2 new_mouse_position);
    void OnMouseMapPositionChange(ClientState& client_state,
                                  glm::vec2 last_mouse_position,
                                  glm::vec2 new_mouse_position,
                                  const StateManager& game_state_manager);

    void OnKeyPressed(int key, ClientState& client_state, StateManager& game_state_manager);
    void OnKeyReleased(int key, ClientState& client_state);

    void ExecuteNewAction(ClientState& client_state,
                          StateManager& game_state_manager,
                          std::unique_ptr<MapEditorAction> new_action);
    void UndoLastAction(ClientState& client_state, StateManager& game_state_manager);
    void RedoUndoneAction(ClientState& client_state, StateManager& game_state_manager);
    void UpdateUndoRedoButtons(ClientState& client_state);

    void RemoveCurrentSelection(ClientState& client_state, StateManager& game_state_manager);

    void OnChangeSelectedSpawnPointsTypes(PMSSpawnPointType new_spawn_point_type,
                                          ClientState& client_state,
                                          StateManager& game_state_manager);
    void OnChangeSelectedSceneriesLevel(int new_level,
                                        ClientState& client_state,
                                        StateManager& game_state_manager);
    void OnTransformSelectedPolygons(
      const std::function<PMSPolygon(const PMSPolygon&)>& transform_function,
      ClientState& client_state,
      StateManager& game_state_manager);

    ToolType selected_tool_;
    std::vector<std::unique_ptr<Tool>> tools_;
    std::function<void(std::unique_ptr<MapEditorAction>)> add_new_map_editor_action_;
    std::function<void(MapEditorAction*)> execute_without_adding_map_editor_action_;
    std::deque<std::unique_ptr<MapEditorAction>> map_editor_executed_actions_;
    std::vector<std::unique_ptr<MapEditorAction>> map_editor_undone_actions_;

    glm::vec2 current_mouse_screen_position_;
    glm::vec2 camera_position_on_start_dragging_;
    glm::vec2 mouse_screen_position_on_start_dragging_;
    bool is_dragging_camera_;
    bool locked_;

    bool is_holding_left_ctrl_;
    bool is_holding_left_shift_;

    std::vector<PMSPolygon> copied_polygons_;
    std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>> copied_sceneries_;
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> copied_spawn_points_;
};
} // namespace Soldank

#endif
