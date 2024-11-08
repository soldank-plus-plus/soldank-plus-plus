#ifndef __MAP_EDITOR_ACTION_HPP__
#define __MAP_EDITOR_ACTION_HPP__

#include "rendering/ClientState.hpp"
#include "core/state/State.hpp"

namespace Soldank
{
class MapEditorAction
{
public:
    virtual ~MapEditorAction() = default;
    virtual bool CanExecute(const ClientState& client_state, const State& game_state) = 0;
    virtual void Execute(ClientState& client_state, State& game_state) = 0;
    virtual void Undo(ClientState& client_state, State& game_state) = 0;
};
} // namespace Soldank

#endif
