module;

export module Editor.EditorSession;

import Application.ClientModes;
import Application.Window;
import Editor.PlayTestSession;
import MapEditor;
import ClientState;

import Shared.Core.IWorld;

export namespace Soldank
{
class EditorSession
{
public:
    EditorSession(ClientState& client_state, IWorld& world, Window& window, MapEditor& map_editor)
        : client_state_(client_state)
        , world_(world)
        , window_(window)
        , map_editor_(map_editor)
    {
    }

    EditorMode GetEditorMode() const
    {
        return play_test_session_.IsActive() ? EditorMode::PlayTest : EditorMode::Edit;
    }

    bool IsPlayTestActive() const { return play_test_session_.IsActive(); }

    void StartPlayTest()
    {
        play_test_session_.Start(client_state_, world_, window_, map_editor_);
    }

    void StopPlayTest()
    {
        play_test_session_.Stop(client_state_, world_, window_, map_editor_);
    }

    void TogglePlayTest()
    {
        if (play_test_session_.IsActive()) {
            StopPlayTest();
        } else {
            StartPlayTest();
        }
    }

private:
    ClientState& client_state_;
    IWorld& world_;
    Window& window_;
    MapEditor& map_editor_;
    PlayTestSession play_test_session_;
};
} // namespace Soldank
