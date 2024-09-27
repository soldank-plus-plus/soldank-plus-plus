#include "map_editor/MapEditor.hpp"

#include "map_editor/tools/DummyTool.hpp"

#include "spdlog/spdlog.h"

#include <utility>

namespace Soldank
{
MapEditor::MapEditor(ClientState& client_state)
    : selected_tool_(client_state.map_editor_state.selected_tool)
{

    client_state.map_editor_state.event_selected_new_tool.AddObserver(
      [this](ToolType tool_type) { OnSelectNewTool(tool_type); });

    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
}

void MapEditor::OnSelectNewTool(ToolType tool_type)
{
    tools_.at(std::to_underlying(selected_tool_))->OnUnselect();
    selected_tool_ = tool_type;
    tools_.at(std::to_underlying(selected_tool_))->OnSelect();
}
} // namespace Soldank
