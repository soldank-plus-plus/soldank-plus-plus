module;

#include <functional>
#include <memory>
#include <utility>
#include <type_traits>
#include <vector>

export module MapEditor.EditorToolController;

import Tool;
import MapEditorAction;
import ClientState;
import MapEditorState;
import ColorPickerTool;
import ColorTool;
import PolygonTool;
import SceneryTool;
import SelectionTool;
import SpawnpointTool;
import TextureTool;
import TransformTool;
import VertexColorTool;
import VertexSelectionTool;
import WaypointTool;

import Shared.Core.State.StateManager;

export namespace Soldank
{
class EditorToolController
{
public:
    EditorToolController(std::function<void(std::unique_ptr<MapEditorAction>)> add_action,
                         std::function<void(MapEditorAction*)> execute_without_adding_action)
        : selected_tool_(ToolType::Selection)
        , active_(true)
    {
        tools_.emplace_back(
          std::make_unique<TransformTool>(add_action, std::move(execute_without_adding_action)));
        tools_.emplace_back(std::make_unique<PolygonTool>(add_action));
        tools_.emplace_back(std::make_unique<VertexSelectionTool>());
        tools_.emplace_back(std::make_unique<SelectionTool>());
        tools_.emplace_back(std::make_unique<VertexColorTool>());
        tools_.emplace_back(std::make_unique<ColorTool>(add_action));
        tools_.emplace_back(std::make_unique<TextureTool>());
        tools_.emplace_back(std::make_unique<SceneryTool>(add_action));
        tools_.emplace_back(std::make_unique<WaypointTool>());
        tools_.emplace_back(std::make_unique<SpawnpointTool>(add_action));
        tools_.emplace_back(std::make_unique<ColorPickerTool>());
    }

    void Select(ToolType tool_type,
                ClientState& client_state,
                const StateManager& game_state_manager)
    {
        if (!active_) {
            return;
        }

        ActiveTool().OnUnselect(client_state);
        selected_tool_ = tool_type;
        ActiveTool().OnSelect(client_state, game_state_manager);
    }

    void Activate() { active_ = true; }

    void Deactivate() { active_ = false; }

    bool IsActive() const { return active_; }

    Tool& ActiveTool() { return *tools_.at(std::to_underlying(selected_tool_)); }

private:
    ToolType selected_tool_;
    std::vector<std::unique_ptr<Tool>> tools_;
    bool active_;
};
} // namespace Soldank
