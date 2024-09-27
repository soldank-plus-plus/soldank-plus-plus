#include "map_editor/tools/DummyTool.hpp"

#include "spdlog/spdlog.h"

namespace Soldank
{
void DummyTool::OnSelect()
{
    spdlog::debug("DummyTool Select");
}

void DummyTool::OnUnselect()
{
    spdlog::debug("DummyTool Unselect");
}
} // namespace Soldank
