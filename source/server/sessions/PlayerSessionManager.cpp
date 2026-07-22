module;

#include <array>
#include <cstdint>

export module Sessions.PlayerSessionManager;

import Shared.Core.Utility.SerialNumber;

import Shared.Core.Config.Config;

export namespace Soldank
{
class PlayerSessionManager
{
public:
    PlayerSessionManager() { ResetAllInputIds(); }

    void ResetAllInputIds()
    {
        for (unsigned int& last_received_input_id : last_received_input_id_) {
            last_received_input_id = 0;
        }
        for (unsigned int& last_applied_input_id : last_applied_input_id_) {
            last_applied_input_id = 0;
        }
    }

    bool ShouldAcceptInput(std::uint8_t soldier_id, std::uint32_t input_sequence_id) const
    {
        const std::uint32_t last_received_input_id = last_received_input_id_.at(soldier_id);
        return IsSerialNumberNewer(input_sequence_id, last_received_input_id);
    }

    void MarkInputReceived(std::uint8_t soldier_id, std::uint32_t input_sequence_id)
    {
        last_received_input_id_.at(soldier_id) = input_sequence_id;
    }

    void ClearSoldier(std::uint8_t soldier_id)
    {
        last_received_input_id_.at(soldier_id) = 0;
        last_applied_input_id_.at(soldier_id) = 0;
    }

    std::uint32_t GetLastReceivedInputId(std::uint8_t soldier_id) const
    {
        return last_received_input_id_.at(soldier_id);
    }

    void MarkInputApplied(std::uint8_t soldier_id, std::uint32_t input_sequence_id)
    {
        last_applied_input_id_.at(soldier_id) = input_sequence_id;
    }

    std::uint32_t GetLastAppliedInputId(std::uint8_t soldier_id) const
    {
        return last_applied_input_id_.at(soldier_id);
    }

private:
    std::array<unsigned int, Config::MAX_PLAYERS> last_received_input_id_{};
    std::array<unsigned int, Config::MAX_PLAYERS> last_applied_input_id_{};
};
} // namespace Soldank
