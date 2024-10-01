#ifndef __MAP_EDITOR_ACTION_HPP__
#define __MAP_EDITOR_ACTION_HPP__

#include "rendering/ClientState.hpp"
#include "core/map/Map.hpp"

namespace Soldank
{
class MapEditorAction
{
public:
    virtual ~MapEditorAction() = default;
    virtual void Execute(ClientState& client_state, Map& map) = 0;
    virtual void Undo(ClientState& client_state, Map& map) = 0;
};
} // namespace Soldank

#endif
