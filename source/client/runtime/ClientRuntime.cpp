module;

export module Runtime.ClientRuntime;

import Application.ClientModes;
import Application.Input.InputSnapshot;

export namespace Soldank
{
class ClientRuntime
{
public:
    void SetClientMode(ClientMode client_mode) { client_mode_ = client_mode; }

    ClientMode GetClientMode() const { return client_mode_; }

    void SetEditorMode(EditorMode editor_mode) { editor_mode_ = editor_mode; }

    EditorMode GetEditorMode() const { return editor_mode_; }

    bool IsGameplayActive() const
    {
        return client_mode_ == ClientMode::LocalGame || client_mode_ == ClientMode::OnlineGame ||
               editor_mode_ == EditorMode::PlayTest;
    }

    InputContext GetInputContext(bool is_ui_captured) const
    {
        if (is_ui_captured) {
            return InputContext::UiCaptured;
        }

        if (client_mode_ == ClientMode::MapEditor) {
            return editor_mode_ == EditorMode::PlayTest ? InputContext::EditorPlayTest
                                                        : InputContext::Editor;
        }

        return InputContext::Gameplay;
    }

private:
    ClientMode client_mode_ = ClientMode::LocalGame;
    EditorMode editor_mode_ = EditorMode::Edit;
};
} // namespace Soldank
