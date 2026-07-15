module;

#include <cstdint>
export module Editor.PlayTestSession;

import Extern.Glm;

import Application.Window;
import MapEditor;
import ClientState;

import Shared.Core.IWorld;
import Shared.Core.Map.RuntimeMap;
import Shared.Core.State.StateManager;

export namespace Soldank
{
class PlayTestSession
{
public:
    bool IsActive() const { return is_active_; }

    void Start(ClientState& client_state, IWorld& world, Window& window, MapEditor& map_editor)
    {
        if (!client_state.client_soldier_id.has_value() ||
            !world.GetStateManager()->GetSoldier(*client_state.client_soldier_id).active) {
            const auto& soldier = world.CreateSoldier();
            client_state.client_soldier_id = soldier.id;
        }

        std::uint8_t client_soldier_id = *client_state.client_soldier_id;
        if (world.GetStateManager()->GetSoldier(client_soldier_id).dead_meat) {
            world.SpawnSoldier(client_soldier_id);
        }

        client_state.camera.view.ResetZoom();
        world.GetStateManager()->UnPauseGame();
        if (!is_map_normalized_for_runtime_) {
            BuildRuntimeMapAndMoveSoldiers(world);
            is_map_normalized_for_runtime_ = true;
        }
        world.GetStateManager()->GetMap().GenerateSectors();
        window.SetCursorMode(CursorMode::Locked);
        map_editor.Lock();
        is_active_ = true;
    }

    void Stop(ClientState& /*client_state*/, IWorld& world, Window& window, MapEditor& map_editor)
    {
        world.GetStateManager()->PauseGame();
        window.SetCursorMode(CursorMode::Normal);
        map_editor.Unlock();
        is_active_ = false;
    }

private:
    static void BuildRuntimeMapAndMoveSoldiers(IWorld& world)
    {
        RuntimeMap runtime_map = world.GetStateManager()->BuildRuntimeMapFromDocument();
        glm::vec2 move_offset = runtime_map.GetDocumentToRuntimeOffset();
        world.GetStateManager()->ApplyRuntimeMap(runtime_map);

        world.GetStateManager()->TransformSoldiers(
          [&](auto& soldier) { world.GetStateManager()->MoveSoldier(soldier.id, move_offset); });
    }

    bool is_active_ = false;
    bool is_map_normalized_for_runtime_ = false;
};
} // namespace Soldank
