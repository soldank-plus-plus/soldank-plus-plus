module;

#include <functional>
#include <memory>
#include <utility>

export module SpawnpointTool;

import Extern.Glm;

import Tool;
import AddSpawnPointMapEditorAction;
import MapEditorAction;
import ClientState;

import Shared.Core.State.StateManager;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.Map;

export namespace Soldank
{
class SpawnpointTool final : public Tool
{
public:
    SpawnpointTool(
      const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action)
        : add_new_map_editor_action_(add_new_map_editor_action)
    {
    }

    ~SpawnpointTool() final = default;

    void OnSelect(ClientState& client_state, const StateManager& /*game_state_manager*/) final
    {
        client_state.map_editor_state.spawn_point_preview_position =
          client_state.input.mouse_map_position;
        client_state.map_editor_state.current_tool_action_description = "Place Spawn Point";
    }

    void OnUnselect(ClientState& /*client_state*/) final {}

    void OnSceneLeftMouseButtonClick(ClientState& client_state,
                                     const StateManager& /*game_state_manager*/) final
    {
        PMSSpawnPoint new_spawn_point{
            .active = 1,
            .x = (int)client_state.map_editor_state.spawn_point_preview_position.x,
            .y = (int)client_state.map_editor_state.spawn_point_preview_position.y,
            .type = client_state.map_editor_state.selected_spawn_point_type
        };
        auto add_spawn_point_action =
          std::make_unique<AddSpawnPointMapEditorAction>(new_spawn_point);
        add_new_map_editor_action_(std::move(add_spawn_point_action));
    }

    void OnSceneLeftMouseButtonRelease(ClientState& /*client_state*/,
                                       const StateManager& /*game_state_manager*/) final
    {
    }

    void OnSceneRightMouseButtonClick(ClientState& client_state) final
    {
        client_state.map_editor_state.should_open_spawn_point_type_popup = true;
    }

    void OnSceneRightMouseButtonRelease() final {}

    void OnMouseScreenPositionChange(ClientState& /*client_state*/,
                                     glm::vec2 /*last_mouse_position*/,
                                     glm::vec2 /*new_mouse_position*/) final
    {
    }

    void OnMouseMapPositionChange(ClientState& client_state,
                                  glm::vec2 /*last_mouse_position*/,
                                  glm::vec2 new_mouse_position,
                                  const StateManager& /*game_state_manager*/) final
    {
        if (client_state.map_editor_state.is_snap_to_grid_enabled) {
            client_state.map_editor_state.spawn_point_preview_position = SnapMousePositionToGrid(
              new_mouse_position, client_state.map_editor_state.grid_interval_division);
        } else {
            client_state.map_editor_state.spawn_point_preview_position = new_mouse_position;
        }
    }

    void OnModifierKey1Pressed(ClientState& /* client_state */) final {}

    void OnModifierKey1Released(ClientState& /* client_state */) final {}

    void OnModifierKey2Pressed(ClientState& /* client_state */) final {}

    void OnModifierKey2Released(ClientState& /* client_state */) final {}

    void OnModifierKey3Pressed(ClientState& /* client_state */) final {}

    void OnModifierKey3Released(ClientState& /* client_state */) final {}

private:
    std::function<void(std::unique_ptr<MapEditorAction>)> add_new_map_editor_action_;
};
} // namespace Soldank
