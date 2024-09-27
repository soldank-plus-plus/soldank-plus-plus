#ifndef __MAP_EDITOR_HPP__
#define __MAP_EDITOR_HPP__

#include "map_editor/tools/Tool.hpp"
#include "rendering/ClientState.hpp"

#include <vector>
#include <memory>

namespace Soldank
{
class MapEditor
{
public:
    MapEditor(ClientState& client_state);

private:
    void OnSelectNewTool(ToolType tool_type);

    ToolType selected_tool_;
    std::vector<std::unique_ptr<Tool>> tools_;
};
} // namespace Soldank

#endif
