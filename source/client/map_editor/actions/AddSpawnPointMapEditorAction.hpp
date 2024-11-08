#ifndef __ADD_SPAWN_POINT_MAP_EDITOR_ACTION_HPP__
#define __ADD_SPAWN_POINT_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class AddSpawnPointMapEditorAction final : public MapEditorAction
{
public:
    AddSpawnPointMapEditorAction(const PMSSpawnPoint& new_spawn_point);

    bool CanExecute(const ClientState& client_state, const State& game_state) final;
    void Execute(ClientState& client_state, State& game_state) final;
    void Undo(ClientState& client_state, State& game_state) final;

private:
    PMSSpawnPoint added_spawn_point_;
    unsigned int added_spawn_point_id_;
};
} // namespace Soldank

#endif
