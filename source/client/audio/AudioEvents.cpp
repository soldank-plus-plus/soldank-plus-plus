module;

#include <cstdint>
#include <string_view>

export module Audio.AudioEvents;

import Shared.Core.Utility.Observable;

export namespace Soldank
{
enum class AudioCommand : std::uint8_t
{
    Play,
    StartLoop,
    StopLoop,
    StopAll,
};

// Playback requests contain no simulation entities or network state.
struct AudioEvent
{
    AudioCommand command{};
    std::string_view sound{};
    std::uint8_t channel{};
};

struct AudioEvents
{
    Observable<const AudioEvent&> sound_requested;
};

class IAudioPlayer
{
public:
    virtual ~IAudioPlayer() = default;
    virtual void Handle(const AudioEvent& event) = 0;
};
} // namespace Soldank
