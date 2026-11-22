module;
#include <cstdint>
#include <memory>

export module Shared.Core.Animations.SoldierEmotes;
import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Animations.EmoteTypes;
import Shared.Core.Entities.Soldier;
import Shared.Core.Combat.HealthEffects;
import Shared.Core.Types.WeaponType;

export namespace Soldank::SoldierEmotes
{
bool IsEmoteAnimation(AnimationType type)
{
    return (type >= AnimationType::Cigar && type <= AnimationType::TakeOff) ||
           type == AnimationType::Victory || type == AnimationType::Own;
}
bool HasInterruptingInput(const Soldier& soldier)
{
    const auto& c = soldier.control;
    return c.left || c.right || c.up || c.down || c.jets || c.prone || c.fire || c.change ||
           c.drop || c.reload || c.throw_grenade;
}
bool CanStart(const Soldier& soldier)
{
    return soldier.active && soldier.IsAlive() && !soldier.IsSpectator() &&
           soldier.mounted_weapon < 0 && !HasInterruptingInput(soldier) &&
           soldier.body_animation->GetType() == AnimationType::Stand &&
           soldier.legs_animation->GetType() == AnimationType::Stand &&
           !soldier.GetActiveWeapon().IsReloading();
}
bool Request(Soldier& soldier, Emote emote)
{
    if (!IsValidEmote(emote) || soldier.idle_random >= 0 || !CanStart(soldier)) {
        return false;
    }
    if (emote == Emote::TakeOff &&
        (soldier.wear_helmet == 0 ||
         soldier.GetActiveWeapon().IsAny({ WeaponType::Bow, WeaponType::FlameBow }))) {
        return false;
    }
    soldier.idle_random = static_cast<std::int8_t>(emote);
    soldier.idle_time = 0;
    soldier.emote.phase = 0;
    return true;
}
void Cue(Soldier& soldier, EmoteCue cue)
{
    soldier.emote.cues |= static_cast<std::uint32_t>(cue);
}
void Finish(Soldier& soldier, const AnimationDataManager& manager)
{
    if (IsEmoteAnimation(soldier.body_animation->GetType())) {
        soldier.body_animation = std::make_shared<BodyStandAnimationState>(manager);
    }
    if (IsEmoteAnimation(soldier.legs_animation->GetType())) {
        soldier.legs_animation = std::make_shared<LegsStandAnimationState>(manager);
    }
    soldier.idle_random = -1;
    soldier.idle_time = DEFAULT_IDLE_TICKS;
    soldier.emote.phase = 0;
}
void Play(Soldier& soldier,
          const AnimationDataManager& manager,
          AnimationType type,
          bool both = false,
          unsigned int frame = 1)
{
    soldier.body_animation = std::make_shared<EmoteAnimationState>(manager, type);
    soldier.body_animation->SetFrame(frame);
    if (both) {
        soldier.legs_animation = std::make_shared<EmoteAnimationState>(manager, type);
    }
}

// Round-end celebration is presentation while gameplay is frozen. It must not depend
// on a reload or held input left over from the last live simulation tick.
void CelebrateRound(Soldier& soldier, const AnimationDataManager& manager)
{
    Finish(soldier, manager);
    soldier.legs_animation = std::make_shared<LegsStandAnimationState>(manager);
    Play(soldier, manager, AnimationType::Victory);
    soldier.idle_random = static_cast<std::int8_t>(Emote::Victory);
    ++soldier.emote.instance;
    soldier.emote.phase = 1;
    soldier.emote.cues = static_cast<std::uint32_t>(EmoteCue::Roar);
    soldier.emote.weapon = soldier.GetActiveWeapon().GetWeaponParameters().kind;
}

// Called before ordinary animation transitions. Only the authority chooses random idle styles.
void Update(Soldier& soldier,
            const AnimationDataManager& manager,
            bool authoritative,
            unsigned int automatic_style)
{
    if (!soldier.IsAlive() || soldier.IsSpectator()) {
        soldier.idle_random = -1;
        soldier.idle_time = DEFAULT_IDLE_TICKS;
        soldier.emote.phase = 0;
        return;
    }
    if (soldier.idle_random < 0) {
        if (!CanStart(soldier)) {
            soldier.idle_time = DEFAULT_IDLE_TICKS;
            return;
        }
        if (soldier.idle_time > 0)
            --soldier.idle_time;
        if (!authoritative || soldier.idle_time > 0)
            return;
        Request(soldier, static_cast<Emote>(automatic_style % 4));
    }
    if (HasInterruptingInput(soldier) || soldier.mounted_weapon >= 0 ||
        soldier.GetActiveWeapon().IsReloading()) {
        Finish(soldier, manager);
        return;
    }
    const auto style = static_cast<Emote>(soldier.idle_random);
    auto& phase = soldier.emote.phase;
    if (phase == 0) {
        if (!CanStart(soldier)) {
            Finish(soldier, manager);
            return;
        }
        ++soldier.emote.instance;
        soldier.emote.cues = 0;
        soldier.emote.weapon = soldier.GetActiveWeapon().GetWeaponParameters().kind;
        phase = 1;
        switch (style) {
            case Emote::Tobacco:
                Play(soldier, manager, AnimationType::Smoke);
                break;
            case Emote::Smoke:
                if (soldier.has_cigar == 10) {
                    phase = 5;
                    Play(soldier, manager, AnimationType::Smoke);
                } else {
                    soldier.has_cigar = 0;
                    Play(soldier, manager, AnimationType::Cigar);
                }
                break;
            case Emote::Wipe:
                Play(soldier, manager, AnimationType::Wipe);
                break;
            case Emote::Groin:
                Play(soldier, manager, AnimationType::Groin);
                break;
            case Emote::TakeOff:
                phase = soldier.wear_helmet == 1 ? 1 : 2;
                Play(soldier, manager, AnimationType::TakeOff, false, phase == 1 ? 1 : 10);
                break;
            case Emote::Victory:
                Play(soldier, manager, AnimationType::Victory);
                Cue(soldier, EmoteCue::Roar);
                break;
            case Emote::Piss:
                Play(soldier, manager, AnimationType::Piss);
                Cue(soldier, EmoteCue::Piss);
                break;
            case Emote::Mercy: {
                const bool long_weapon = soldier.GetActiveWeapon().IsAny({ WeaponType::M79,
                                                                           WeaponType::Minimi,
                                                                           WeaponType::Spas12,
                                                                           WeaponType::LAW,
                                                                           WeaponType::Chainsaw,
                                                                           WeaponType::Barrett,
                                                                           WeaponType::Minigun });
                Play(soldier,
                     manager,
                     long_weapon ? AnimationType::Mercy2 : AnimationType::Mercy,
                     true);
                Cue(soldier, EmoteCue::Mercy);
                break;
            }
            case Emote::Own:
                Play(soldier, manager, AnimationType::Own, true);
                break;
            default:
                Finish(soldier, manager);
                return;
        }
    }
    const auto type = soldier.body_animation->GetType();
    const auto frame = soldier.body_animation->GetFrame();
    const bool end = frame == soldier.body_animation->GetFramesCount();
    if (!IsEmoteAnimation(type) && phase != 2 && phase != 3 && phase != 4) {
        Finish(soldier, manager);
        return;
    }
    if (style == Emote::Tobacco) {
        if (phase == 1 && frame >= 17)
            Cue(soldier, EmoteCue::Stuff);
        if (phase == 1 && end) {
            soldier.body_animation = std::make_shared<BodyStandAnimationState>(manager);
            phase = 2;
            soldier.idle_time = DEFAULT_IDLE_TICKS;
        } else if (phase == 2 && --soldier.idle_time <= 0) {
            Cue(soldier, EmoteCue::Spit);
            Finish(soldier, manager);
        }
    } else if (style == Emote::Smoke) {
        if (phase == 1 && frame >= 26)
            soldier.has_cigar = 5;
        if (phase == 1 && frame >= 37) {
            Play(soldier, manager, AnimationType::Cigar);
            phase = 2;
        } else if (phase == 2) {
            if (frame >= 9)
                Cue(soldier, EmoteCue::Match);
            if (frame >= 26) {
                soldier.has_cigar = 10;
                Cue(soldier, EmoteCue::Light);
                soldier.idle_time = LONGER_IDLE_TICKS;
                phase = 3;
            }
        } else if (phase == 3) {
            if (end) {
                soldier.body_animation = std::make_shared<BodyStandAnimationState>(manager);
                phase = 4;
            }
            --soldier.idle_time;
        } else if (phase == 4 && --soldier.idle_time <= 0) {
            Play(soldier, manager, AnimationType::Smoke);
            phase = 5;
        } else if (phase == 5) {
            if (frame >= 17)
                Cue(soldier, EmoteCue::PuffFirst);
            if (frame >= 37)
                Cue(soldier, EmoteCue::PuffSecond);
            if (frame >= 38) {
                soldier.has_cigar = 0;
                Cue(soldier, EmoteCue::Discard);
                Finish(soldier, manager);
            }
        }
    } else if (style == Emote::TakeOff) {
        if (frame >= 15 && phase < 3) {
            soldier.wear_helmet = phase == 1 ? 2 : 1;
            phase = 3;
        }
        if (end || (phase == 3 && soldier.wear_helmet == 2 && frame >= 22)) {
            Finish(soldier, manager);
        }
    } else if (style == Emote::Mercy) {
        if (frame >= 20) {
            // Spend the shot without emitting a projectile that could hit other players.
            // The reference shot is directed into the soldier's own body.
            if ((soldier.emote.cues & static_cast<std::uint32_t>(EmoteCue::MercyFire)) == 0) {
                soldier.GetActiveWeapon().TryFire();
            }
            Cue(soldier, EmoteCue::MercyFire);
            if (authoritative) {
                soldier.vest = 0;
                soldier.health = 0;
                HealthEffects::Finalize(soldier);
            }
            soldier.idle_random = -1;
            soldier.idle_time = DEFAULT_IDLE_TICKS;
            phase = 0;
        }
    } else if (end) {
        Finish(soldier, manager);
    }
}
} // namespace Soldank::SoldierEmotes
