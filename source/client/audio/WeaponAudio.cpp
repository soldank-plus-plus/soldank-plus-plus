module;

#include <array>
#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module Audio.WeaponAudio;

import Audio.AudioEvents;
import Extern.Miniaudio;
import Extern.Spdlog;

namespace Soldank
{
class MiniaudioPlayer final : public IAudioPlayer
{
public:
    MiniaudioPlayer()
        : ready_(Miniaudio::InitializeEngine(engine_))
    {
        if (!ready_) {
            Spdlog::warn("Audio device unavailable; continuing without audio");
        }
    }
    MiniaudioPlayer(const MiniaudioPlayer&) = delete;
    MiniaudioPlayer& operator=(const MiniaudioPlayer&) = delete;
    MiniaudioPlayer(MiniaudioPlayer&&) = delete;
    MiniaudioPlayer& operator=(MiniaudioPlayer&&) = delete;
    ~MiniaudioPlayer() override
    {
        if (ready_) {
            StopAll();
            Miniaudio::UninitializeEngine(engine_);
        }
    }
    void Handle(const AudioEvent& event) override
    {
        if (!ready_) {
            return;
        }
        if (event.command == AudioCommand::StopAll) {
            StopAll();
            return;
        }
        if (event.command == AudioCommand::Play) {
            if (!event.sound.empty()) {
                const auto path = Path(event.sound);
                std::erase_if(one_shots_,
                              [](const auto& sound) { return Miniaudio::IsAtEnd(sound->sound); });
                constexpr std::size_t MAX_ONE_SHOTS = 64;
                if (one_shots_.size() == MAX_ONE_SHOTS) {
                    one_shots_.erase(one_shots_.begin());
                }
                auto sound = std::make_unique<Voice>();
                if (Miniaudio::InitializeSound(engine_, path.c_str(), sound->sound)) {
                    sound->initialized = true;
                    Miniaudio::StartSound(sound->sound);
                    one_shots_.push_back(std::move(sound));
                }
            }
            return;
        }
        if (event.channel >= voices_.size()) {
            return;
        }
        auto& voice = voices_.at(event.channel);
        if (event.command == AudioCommand::StopLoop) {
            Stop(voice);
        } else if (!event.sound.empty() && (!voice.initialized || voice.name != event.sound)) {
            Stop(voice);
            const auto path = Path(event.sound);
            if (Miniaudio::InitializeSound(engine_, path.c_str(), voice.sound)) {
                voice.initialized = true;
                voice.name = event.sound;
                Miniaudio::SetLooping(voice.sound, true);
                Miniaudio::StartSound(voice.sound);
            }
        }
    }

private:
    struct Voice
    {
        Voice() = default;
        Voice(const Voice&) = delete;
        Voice& operator=(const Voice&) = delete;
        Voice(Voice&&) = delete;
        Voice& operator=(Voice&&) = delete;
        ~Voice()
        {
            if (initialized) {
                Miniaudio::UninitializeSound(sound);
            }
        }
        bool initialized{};
        std::string name;
        Miniaudio::Sound sound{};
    };
    static std::string Path(std::string_view name) { return "sfx/" + std::string(name) + ".wav"; }
    static void Stop(Voice& voice)
    {
        if (voice.initialized) {
            Miniaudio::UninitializeSound(voice.sound);
            voice.initialized = false;
            voice.name.clear();
        }
    }
    void StopAll()
    {
        for (auto& voice : voices_) {
            Stop(voice);
        }
        one_shots_.clear();
    }
    Miniaudio::Engine engine_{};
    bool ready_{};
    std::array<Voice, 32> voices_{};
    std::vector<std::unique_ptr<Voice>> one_shots_;
};
} // namespace Soldank

export namespace Soldank
{
// The audio subscriber knows only playback requests. Gameplay selects sounds elsewhere.
class WeaponAudio
{
public:
    explicit WeaponAudio(AudioEvents& events,
                         std::shared_ptr<IAudioPlayer> player = std::make_shared<MiniaudioPlayer>())
        : binding_(std::make_shared<Binding>(Binding{ std::move(player) }))
    {
        events.sound_requested.AddObserver(
          [weak = std::weak_ptr<Binding>(binding_)](const AudioEvent& event) {
              if (const auto binding = weak.lock()) {
                  binding->player->Handle(event);
              }
          });
    }
    WeaponAudio(const WeaponAudio&) = delete;
    WeaponAudio& operator=(const WeaponAudio&) = delete;
    WeaponAudio(WeaponAudio&&) = delete;
    WeaponAudio& operator=(WeaponAudio&&) = delete;
    ~WeaponAudio() { binding_->player->Handle({ .command = AudioCommand::StopAll }); }

private:
    struct Binding
    {
        std::shared_ptr<IAudioPlayer> player;
    };
    std::shared_ptr<Binding> binding_;
};
} // namespace Soldank
