module;

#include <array>
#include <cstdint>

export module Sessions.PlayerSessionManager;

import Application.ServerState;
import Shared.Core.Config.Config;

export namespace Soldank
{
class PlayerSessionManager
{
public:
    explicit PlayerSessionManager(ServerState& server_state)
        : server_state_(server_state)
    {
        ResetAllInputIds();
    }

    void ResetAllInputIds()
    {
        for (unsigned int& last_processed_input_id : server_state_.last_processed_input_id) {
            last_processed_input_id = 0;
        }
    }

    bool ShouldAcceptInput(std::uint8_t soldier_id, std::uint32_t input_sequence_id) const
    {
        return input_sequence_id > server_state_.last_processed_input_id.at(soldier_id);
    }

    void MarkInputProcessed(std::uint8_t soldier_id, std::uint32_t input_sequence_id)
    {
        server_state_.last_processed_input_id.at(soldier_id) = input_sequence_id;
    }

    void ClearSoldier(std::uint8_t soldier_id)
    {
        server_state_.last_processed_input_id.at(soldier_id) = 0;
    }

    std::uint32_t GetLastProcessedInputId(std::uint8_t soldier_id) const
    {
        return server_state_.last_processed_input_id.at(soldier_id);
    }

private:
    ServerState& server_state_;
};
} // namespace Soldank
