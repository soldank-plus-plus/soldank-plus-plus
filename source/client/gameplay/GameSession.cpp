module;

export module Gameplay.GameSession;

import Application.ClientModes;

export namespace Soldank
{
class GameSession
{
public:
    bool IsGameplayActive(ClientMode client_mode, EditorMode editor_mode) const
    {
        return client_mode == ClientMode::LocalGame || client_mode == ClientMode::OnlineGame ||
               editor_mode == EditorMode::PlayTest;
    }
};
} // namespace Soldank
