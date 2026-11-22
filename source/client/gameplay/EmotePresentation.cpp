module;
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

export module Gameplay.EmotePresentation;
import Audio.AudioEvents;
import Gameplay.WeaponSoundEventRouter;
import Shared.Core.Entities.Soldier;
import Shared.Core.State.StateManager;
import Shared.Core.WorldEvents;
import Shared.Core.Utility.SerialNumber;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
// Remembers presented milestones across prediction rewinds. New observers seed from the
// current snapshot so joining a running emote never replays its earlier sounds/effects.
class EmotePresentation
{
public:
    using EffectCallback = std::function<void(const Soldier&, std::uint32_t)>;
    static std::shared_ptr<EmotePresentation> Create(
      StateManager& state,
      WorldEvents& world,
      std::shared_ptr<AudioEvents> audio,
      EffectCallback effect,
      std::function<bool()> should_present = [] { return true; })
    {
        auto self = std::shared_ptr<EmotePresentation>(
          new EmotePresentation(std::move(audio), std::move(effect), std::move(should_present)));
        state.ForEachSoldier([&](const Soldier& soldier) { self->Seed(soldier); });
        const std::weak_ptr<EmotePresentation> weak = self;
        world.after_soldier_updates.AddObserver([weak](const Soldier& soldier, unsigned int) {
            if (auto router = weak.lock(); router && router->should_present_()) {
                router->Observe(soldier);
            }
        });
        state.soldier_removed.AddObserver([weak](std::uint8_t id) {
            if (auto router = weak.lock(); router && id < router->entries_.size()) {
                router->entries_.at(id) = {};
            }
        });
        state.game_pause_changed.AddObserver([weak](bool paused) {
            if (auto router = weak.lock())
                router->paused_ = paused;
        });
        return self;
    }
    void Observe(const Soldier& soldier)
    {
        if (soldier.id >= entries_.size())
            return;
        auto& seen = entries_.at(soldier.id);
        if (!seen.initialized || seen.life != soldier.life_sequence) {
            Seed(soldier);
            return;
        }
        if (soldier.emote.instance != seen.instance) {
            if (!IsSerialNumberNewer(soldier.emote.instance, seen.instance))
                return;
            const auto distance =
              SerialNumberForwardDistance(seen.instance, soldier.emote.instance);
            seen.instance = soldier.emote.instance;
            seen.cues = distance == 1 ? 0 : soldier.emote.cues;
        }
        const auto fresh = soldier.emote.cues & ~seen.cues;
        seen.cues |= soldier.emote.cues;
        if (paused_ || fresh == 0)
            return;
        effect_(soldier, fresh);
        const auto play = [&](EmoteCue cue, std::string_view sound) {
            if ((fresh & static_cast<std::uint32_t>(cue)) != 0) {
                audio_->sound_requested.Notify({ AudioCommand::Play, sound });
            }
        };
        play(EmoteCue::Stuff, "stuff");
        play(EmoteCue::Spit, "spit");
        play(EmoteCue::Match, "match");
        play(EmoteCue::Light, "smoke");
        play(EmoteCue::PuffFirst, "smoke");
        play(EmoteCue::PuffSecond, "smoke");
        play(EmoteCue::Roar, "roar");
        play(EmoteCue::Piss, "piss");
        play(EmoteCue::Mercy, "mercy");
        const auto weapon = soldier.emote.weapon;
        if (weapon == WeaponType::Minigun)
            play(EmoteCue::Mercy, "minigun-start");
        if (weapon == WeaponType::Knife)
            play(EmoteCue::MercyFire, "slash");
        else if (weapon == WeaponType::Chainsaw)
            play(EmoteCue::MercyFire, "chainsaw-r");
        else if (weapon == WeaponType::NoWeapon)
            play(EmoteCue::MercyFire, "dead-hit");
        else if (weapon >= WeaponType::DesertEagles && weapon <= WeaponType::NoWeapon) {
            play(EmoteCue::MercyFire, SoundsFor(weapon).fire);
        }
    }

private:
    EmotePresentation(std::shared_ptr<AudioEvents> audio,
                      EffectCallback effect,
                      std::function<bool()> should_present)
        : audio_(std::move(audio))
        , effect_(std::move(effect))
        , should_present_(std::move(should_present))
    {
    }
    void Seed(const Soldier& soldier)
    {
        if (soldier.id < entries_.size()) {
            entries_.at(soldier.id) = {
                true, soldier.life_sequence, soldier.emote.instance, soldier.emote.cues
            };
        }
    }
    struct Entry
    {
        bool initialized{};
        std::uint32_t life{};
        std::uint32_t instance{};
        std::uint32_t cues{};
    };
    std::array<Entry, 32> entries_{};
    std::shared_ptr<AudioEvents> audio_;
    EffectCallback effect_;
    std::function<bool()> should_present_;
    bool paused_{};
};
} // namespace Soldank
