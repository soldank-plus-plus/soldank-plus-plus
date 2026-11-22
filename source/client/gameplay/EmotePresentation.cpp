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
// Remembers presented milestones across prediction rewinds. Recent cigar milestones can
// be recovered after missing snapshots without replaying expired effects or other emotes.
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
        self->paused_ = state.IsGamePaused();
        state.ForEachSoldier([&](const Soldier& soldier) { self->Observe(soldier); });
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
        std::uint32_t fresh{};
        if (!seen.initialized || seen.life != soldier.life_sequence) {
            Seed(soldier);
            fresh = RecentCigarCues(soldier);
        } else if (soldier.emote.instance != seen.instance) {
            if (!IsSerialNumberNewer(soldier.emote.instance, seen.instance))
                return;
            const auto distance =
              SerialNumberForwardDistance(seen.instance, soldier.emote.instance);
            seen.instance = soldier.emote.instance;
            seen.cues = distance == 1 ? 0 : soldier.emote.cues;
            if (distance != 1)
                fresh = RecentCigarCues(soldier);
        }
        fresh |= soldier.emote.cues & ~seen.cues;
        // A delayed snapshot can contain milestones whose particles have already expired.
        fresh &= ~CIGAR_CUES | RecentCigarCues(soldier);
        seen.cues |= soldier.emote.cues;
        if (paused_ || fresh == 0)
            return;
        effect_(soldier, fresh);
        const auto play = [&](EmoteCue cue, std::string_view sound) {
            const auto bit = static_cast<std::uint32_t>(cue);
            unsigned int index = 0;
            while ((1U << index) != bit)
                ++index;
            if ((fresh & bit) != 0 &&
                ((CIGAR_CUES & bit) == 0 || soldier.emote.cue_ages[index] <= 12)) {
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
    static constexpr std::uint32_t CIGAR_CUES = static_cast<std::uint32_t>(EmoteCue::Match) |
                                                static_cast<std::uint32_t>(EmoteCue::Light) |
                                                static_cast<std::uint32_t>(EmoteCue::PuffFirst) |
                                                static_cast<std::uint32_t>(EmoteCue::PuffSecond) |
                                                static_cast<std::uint32_t>(EmoteCue::Discard);

    static std::uint32_t RecentCigarCues(const Soldier& soldier)
    {
        std::uint32_t result{};
        for (unsigned int index = 0; index < EMOTE_CUE_COUNT; ++index) {
            const auto bit = 1U << index;
            const unsigned int lifetime = bit == static_cast<std::uint32_t>(EmoteCue::Discard) ? 245
                                          : bit == static_cast<std::uint32_t>(EmoteCue::Light) ? 245
                                          : bit == static_cast<std::uint32_t>(EmoteCue::Match) ? 13
                                                                                               : 65;
            if ((soldier.emote.cues & CIGAR_CUES & bit) != 0 &&
                soldier.emote.cue_ages[index] < lifetime)
                result |= bit;
        }
        return result;
    }
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
