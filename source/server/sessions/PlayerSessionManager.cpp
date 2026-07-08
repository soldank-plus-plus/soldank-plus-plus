module;

#include <array>
#include <cstdint>

export module Sessions.PlayerSessionManager;

import Shared.Core.Config.Config;

export namespace Soldank
{
class PlayerSessionManager
{
public:
    PlayerSessionManager() { ResetAllInputIds(); }

    void ResetAllInputIds()
    {
        for (unsigned int& last_processed_input_id : last_processed_input_id_) {
            last_processed_input_id = 0;
        }
    }

    bool ShouldAcceptInput(std::uint8_t soldier_id, std::uint32_t input_sequence_id) const
    {
        return input_sequence_id > last_processed_input_id_.at(soldier_id);
    }

    void MarkInputProcessed(std::uint8_t soldier_id, std::uint32_t input_sequence_id)
    {
        last_processed_input_id_.at(soldier_id) = input_sequence_id;
    }

    void ClearSoldier(std::uint8_t soldier_id) { last_processed_input_id_.at(soldier_id) = 0; }

    std::uint32_t GetLastProcessedInputId(std::uint8_t soldier_id) const
    {
        return last_processed_input_id_.at(soldier_id);
    }

private:
    std::array<unsigned int, Config::MAX_PLAYERS> last_processed_input_id_{};
};
} // namespace Soldank
