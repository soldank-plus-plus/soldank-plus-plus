#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <vector>

import Extern.Glm;
import Shared.Core.World;
import Shared.Core.GameModes.BuiltinGameModes;
import Shared.Core.Simulation.WorldTick;
import Shared.Core.Simulation.SimulationEvents;
import Shared.Core.Animations;
import Shared.Core.Animations.SoldierEmotes;
import Shared.Core.Animations.EmoteTypes;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.WeaponType;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.GameModes.GameModeRuntime;
import Testing.Framework.Shared.MapBuilder;
import Gameplay.EmotePresentation;
import Audio.AudioEvents;
import Chat.Model;
import WeaponVisualEffects;

using namespace Soldank;

class EmoteTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        animations_.LoadAllAnimationDatas();
        world_ = std::make_shared<World>();
        world_->GetStateManager()->OverrideMap(*SoldankTesting::MapBuilder::Empty()->Build());
        world_->CreateSoldier(1);
        world_->SpawnSoldier(1, glm::vec2{});
        world_->GetStateManager()->TransformSoldier(1,
                                                    [&](Soldier& soldier) { soldier_ = &soldier; });
        soldier_->wear_helmet = 1;
    }
    void Tick(bool authority = true)
    {
        SoldierEmotes::Update(*soldier_, animations_, authority, 0);
        if (soldier_->IsAlive()) {
            soldier_->body_animation->DoAnimation();
            soldier_->legs_animation->DoAnimation();
        }
    }
    void Begin(Emote emote)
    {
        ASSERT_TRUE(SoldierEmotes::Request(*soldier_, emote));
        Tick();
    }
    void At(unsigned int frame, bool authority = true)
    {
        soldier_->body_animation->SetFrame(frame);
        SoldierEmotes::Update(*soldier_, animations_, authority, 0);
    }
    bool Has(EmoteCue cue) const
    {
        return (soldier_->emote.cues & static_cast<std::uint32_t>(cue)) != 0;
    }
    AnimationDataManager animations_;
    std::shared_ptr<World> world_;
    Soldier* soldier_{};
};

class EveryEmoteTest
    : public EmoteTest
    , public ::testing::WithParamInterface<Emote>
{
};
TEST_P(EveryEmoteTest, PlaysAndCompletesWithoutUnsupportedAnimationStates)
{
    Begin(GetParam());
    EXPECT_TRUE(SoldierEmotes::IsEmoteAnimation(soldier_->body_animation->GetType()));
    for (unsigned int ticks = 0; ticks < 5000 && soldier_->idle_random >= 0; ++ticks)
        Tick();
    EXPECT_EQ(soldier_->idle_random, -1);
    EXPECT_EQ(soldier_->emote.phase, 0);
    EXPECT_EQ(soldier_->emote.instance, 1);
    EXPECT_EQ(soldier_->IsAlive(), GetParam() != Emote::Mercy);
}
INSTANTIATE_TEST_SUITE_P(AllStyles,
                         EveryEmoteTest,
                         ::testing::Values(Emote::Tobacco,
                                           Emote::Smoke,
                                           Emote::Wipe,
                                           Emote::Groin,
                                           Emote::TakeOff,
                                           Emote::Victory,
                                           Emote::Piss,
                                           Emote::Mercy,
                                           Emote::Own));

TEST_F(EmoteTest, TobaccoChewsThenSpitsAfterStandingDelay)
{
    Begin(Emote::Tobacco);
    At(16);
    EXPECT_FALSE(Has(EmoteCue::Stuff));
    At(17);
    EXPECT_TRUE(Has(EmoteCue::Stuff));
    At(soldier_->body_animation->GetFramesCount());
    EXPECT_EQ(soldier_->emote.phase, 2);
    for (int tick = 0; tick < DEFAULT_IDLE_TICKS - 1; ++tick)
        Tick();
    EXPECT_FALSE(Has(EmoteCue::Spit));
    Tick();
    EXPECT_TRUE(Has(EmoteCue::Spit));
    EXPECT_EQ(soldier_->idle_random, -1);
}
TEST_F(EmoteTest, CigarHasSeparateRetrievalLightingAndSmokingStages)
{
    Begin(Emote::Smoke);
    At(26);
    EXPECT_EQ(soldier_->has_cigar, 5);
    EXPECT_EQ(soldier_->body_animation->GetFrame(), 27);
    At(37);
    EXPECT_EQ(soldier_->emote.phase, 2);
    At(9);
    EXPECT_TRUE(Has(EmoteCue::Match));
    EXPECT_EQ(soldier_->body_animation->GetFrame(), 10);
    At(26);
    EXPECT_EQ(soldier_->has_cigar, 10);
    EXPECT_TRUE(Has(EmoteCue::Light));
    EXPECT_EQ(soldier_->body_animation->GetFrame(), 27);
    EXPECT_EQ(soldier_->idle_time, LONGER_IDLE_TICKS);
    At(soldier_->body_animation->GetFramesCount());
    while (soldier_->emote.phase != 5)
        Tick();
    EXPECT_EQ(soldier_->body_animation->GetType(), AnimationType::Smoke);
    At(17);
    EXPECT_TRUE(Has(EmoteCue::PuffFirst));
    EXPECT_EQ(soldier_->body_animation->GetFrame(), 18);
    At(37);
    EXPECT_TRUE(Has(EmoteCue::PuffSecond));
    EXPECT_TRUE(Has(EmoteCue::Discard));
    EXPECT_EQ(soldier_->has_cigar, 0);
    EXPECT_EQ(soldier_->idle_random, -1);
}
TEST_F(EmoteTest, HelmetTogglesOnceInEitherDirection)
{
    for (int direction = 0; direction < 2; ++direction) {
        Begin(Emote::TakeOff);
        EXPECT_EQ(soldier_->body_animation->GetFrame(), direction == 0 ? 1 : 10);
        At(14);
        EXPECT_EQ(soldier_->wear_helmet, direction == 0 ? 1 : 2);
        At(15);
        EXPECT_EQ(soldier_->wear_helmet, direction == 0 ? 2 : 1);
        At(15);
        EXPECT_EQ(soldier_->wear_helmet, direction == 0 ? 2 : 1);
        At(soldier_->body_animation->GetFramesCount());
    }
}
TEST_F(EmoteTest, RejectsInvalidDeadMovingAndRestrictedRequests)
{
    EXPECT_FALSE(SoldierEmotes::Request(*soldier_, static_cast<Emote>(9)));
    soldier_->control.left = true;
    EXPECT_FALSE(SoldierEmotes::Request(*soldier_, Emote::Victory));
    soldier_->control = {};
    soldier_->dead_meat = true;
    EXPECT_FALSE(SoldierEmotes::Request(*soldier_, Emote::Victory));
    soldier_->dead_meat = false;
    world_->GetStateManager()->ChangeSoldierPrimaryWeapon(1, WeaponType::Bow);
    EXPECT_FALSE(SoldierEmotes::Request(*soldier_, Emote::TakeOff));
    world_->GetStateManager()->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    soldier_->wear_helmet = 0;
    EXPECT_FALSE(SoldierEmotes::Request(*soldier_, Emote::TakeOff));
    soldier_->mounted_weapon = 1;
    EXPECT_FALSE(SoldierEmotes::Request(*soldier_, Emote::Victory));
}
TEST_F(EmoteTest, MovementCancelsAndCigarCanRecoverAfterInterruption)
{
    Begin(Emote::Smoke);
    At(26);
    soldier_->control.left = true;
    Tick();
    EXPECT_EQ(soldier_->idle_random, -1);
    soldier_->control = {};
    Begin(Emote::Smoke);
    EXPECT_EQ(soldier_->body_animation->GetType(), AnimationType::Cigar);
    EXPECT_EQ(soldier_->has_cigar, 0);
    At(37);
    At(26);
    soldier_->control.fire = true;
    Tick();
    soldier_->control = {};
    Begin(Emote::Smoke);
    EXPECT_EQ(soldier_->body_animation->GetType(), AnimationType::Smoke);
}
TEST_F(EmoteTest, OnlyAuthoritySelectsAutomaticIdleStyles)
{
    soldier_->idle_time = 1;
    Tick(false);
    EXPECT_EQ(soldier_->idle_random, -1);
    SoldierEmotes::Update(*soldier_, animations_, true, 3);
    EXPECT_EQ(soldier_->body_animation->GetType(), AnimationType::Groin);
    EXPECT_EQ(soldier_->idle_random, 3);
}
TEST_F(EmoteTest, RespawnClearsMultiStageProgress)
{
    Begin(Emote::Smoke);
    At(26);
    world_->KillSoldier(1);
    world_->SpawnSoldier(1, glm::vec2{});
    EXPECT_EQ(soldier_->idle_random, -1);
    EXPECT_EQ(soldier_->idle_time, DEFAULT_IDLE_TICKS);
    EXPECT_EQ(soldier_->emote, EmoteProgress{});
}
TEST_F(EmoteTest, MercyUsesWeaponSpecificBodyAndLegsAndAuthorityOwnsDeath)
{
    world_->GetStateManager()->ChangeSoldierPrimaryWeapon(1, WeaponType::Minimi);
    Begin(Emote::Mercy);
    EXPECT_EQ(soldier_->body_animation->GetType(), AnimationType::Mercy2);
    EXPECT_EQ(soldier_->legs_animation->GetType(), AnimationType::Mercy2);
    At(19);
    EXPECT_TRUE(soldier_->IsAlive());
    At(20, false);
    EXPECT_TRUE(soldier_->IsAlive());
    SoldierEmotes::Finish(*soldier_, animations_);
    world_->GetStateManager()->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    Begin(Emote::Mercy);
    EXPECT_EQ(soldier_->body_animation->GetType(), AnimationType::Mercy);
    At(20);
    EXPECT_FALSE(soldier_->IsAlive());
    EXPECT_TRUE(Has(EmoteCue::MercyFire));
}
TEST_F(EmoteTest, ChatCommandsDoNotBecomeChatMessages)
{
    ChatModel chat;
    std::vector<Emote> requested;
    chat.request_emote = [&](Emote emote) { requested.push_back(emote); };
    for (const auto* command :
         { "/tabac", "/smoke", "/takeoff", "/victory", "/piss", "/mercy", "/pwn" }) {
        chat.draft = command;
        ASSERT_TRUE(chat.Submit());
    }
    EXPECT_EQ(requested.size(), 7);
    EXPECT_TRUE(chat.entries.empty());
    chat.draft = "Hello";
    EXPECT_TRUE(chat.Submit());
    EXPECT_EQ(chat.entries.size(), 1);
}
TEST_F(EmoteTest, PresentationSkipsDuplicatesRewindsReplayAndLateJoin)
{
    auto audio = std::make_shared<AudioEvents>();
    unsigned int sounds = 0;
    unsigned int effects = 0;
    bool replay = false;
    audio->sound_requested.AddObserver([&](const AudioEvent&) { ++sounds; });
    auto router = EmotePresentation::Create(
      *world_->GetStateManager(),
      world_->GetWorldEvents(),
      audio,
      [&](const Soldier&, std::uint32_t) { ++effects; },
      [&] { return !replay; });
    const auto publish = [&] {
        world_->GetWorldEvents().after_soldier_updates.Notify(*soldier_, 1);
    };
    Begin(Emote::Victory);
    publish();
    EXPECT_EQ(sounds, 1);
    publish();
    soldier_->emote.cues = 0;
    publish();
    soldier_->emote.cues = static_cast<std::uint32_t>(EmoteCue::Roar);
    publish();
    EXPECT_EQ(sounds, 1);
    EXPECT_EQ(effects, 1);
    auto late = EmotePresentation::Create(
      *world_->GetStateManager(),
      world_->GetWorldEvents(),
      audio,
      [](const Soldier&, std::uint32_t) {},
      [&] { return !replay; });
    publish();
    EXPECT_EQ(sounds, 1);
    SoldierEmotes::Finish(*soldier_, animations_);
    replay = true;
    Begin(Emote::Piss);
    publish();
    EXPECT_EQ(sounds, 1);
    replay = false;
    publish();
    EXPECT_EQ(sounds, 3); // Existing and late observer each present the new instance once.
}

TEST_F(EmoteTest, WorldCommitsMercyDeathAndScorePenaltyOnceWithoutImmediateRespawn)
{
    world_->GetStateManager()->ConfigureGameMode(MakeDeathmatch());
    world_->Tick({ .tick = 1 });
    auto* mode = world_->GetStateManager()->GetGameMode();
    auto snapshot = mode->GetState();
    snapshot.players.at(1).kills = 3;
    mode->ApplySnapshot(snapshot);
    soldier_->body_animation = world_->GetBodyAnimationState(AnimationType::Stand);
    soldier_->legs_animation = world_->GetLegsAnimationState(AnimationType::Stand);
    Begin(Emote::Mercy);
    soldier_->body_animation->SetFrame(20);
    soldier_->legs_animation->SetFrame(20);
    auto result = world_->Tick({ .tick = 2 });
    EXPECT_TRUE(soldier_->dead_meat);
    EXPECT_TRUE(soldier_->death_committed);
    EXPECT_GT(soldier_->ticks_to_respawn, 0);
    EXPECT_EQ(mode->GetState().players.at(1).kills, 2);
    EXPECT_EQ(mode->GetState().players.at(1).deaths, 1);
    world_->KillSoldier(1);
    EXPECT_EQ(mode->GetState().players.at(1).kills, 2);
    EXPECT_EQ(mode->GetState().players.at(1).deaths, 1);
}
TEST_F(EmoteTest, TimedVisualCuesUseExistingParticlePrimitives)
{
    WeaponVisualEffects visuals;
    visuals.ObserveEmote(*soldier_,
                         static_cast<std::uint32_t>(EmoteCue::Light) |
                           static_cast<std::uint32_t>(EmoteCue::Spit) |
                           static_cast<std::uint32_t>(EmoteCue::Discard));
    ASSERT_EQ(visuals.GetEffects().size(), 4);
    for (const auto& effect : visuals.GetEffects()) {
        EXPECT_EQ(effect.owner, 1);
        EXPECT_EQ(effect.owner_life, soldier_->life_sequence);
        EXPECT_GT(effect.lifetime, 0);
    }
}

TEST_F(EmoteTest, RoundVictoryStartsDespiteHeldActions)
{
    soldier_->control.fire = true;
    soldier_->control.reload = true;
    SoldierEmotes::CelebrateRound(*soldier_, animations_);
    EXPECT_EQ(soldier_->body_animation->GetType(), AnimationType::Victory);
    EXPECT_EQ(soldier_->idle_random, static_cast<std::int8_t>(Emote::Victory));
    EXPECT_TRUE(Has(EmoteCue::Roar));
    EXPECT_TRUE(soldier_->control.fire);
}

TEST_F(EmoteTest, CigarCuesAgeEvenAfterTheSequenceFinishes)
{
    soldier_->emote.cues = static_cast<std::uint32_t>(EmoteCue::Discard);
    for (unsigned int tick = 0; tick < MAX_EMOTE_CUE_AGE + 10U; ++tick)
        Tick(false);
    EXPECT_EQ(soldier_->emote.cue_ages[6], MAX_EMOTE_CUE_AGE);
    EXPECT_EQ(soldier_->idle_random, -1);
}

TEST_F(EmoteTest, LateCigarObserverRecoversRemainingEffectsWithoutExpiredSmokeOrOldAudio)
{
    soldier_->emote.instance = 5;
    soldier_->emote.cues = static_cast<std::uint32_t>(EmoteCue::Light) |
                           static_cast<std::uint32_t>(EmoteCue::PuffFirst) |
                           static_cast<std::uint32_t>(EmoteCue::Discard);
    soldier_->emote.cue_ages[3] = 100;
    soldier_->emote.cue_ages[4] = 10;
    soldier_->emote.cue_ages[6] = 2;
    WeaponVisualEffects visuals;
    auto audio = std::make_shared<AudioEvents>();
    unsigned int sounds = 0;
    unsigned int emissions = 0;
    audio->sound_requested.AddObserver([&](const AudioEvent&) { ++sounds; });
    auto router = EmotePresentation::Create(*world_->GetStateManager(),
                                            world_->GetWorldEvents(),
                                            audio,
                                            [&](const Soldier& soldier, std::uint32_t cues) {
                                                ++emissions;
                                                visuals.ObserveEmote(soldier, cues);
                                            });
    ASSERT_EQ(visuals.GetEffects().size(), 3);
    EXPECT_EQ(sounds, 1);
    for (const auto& effect : visuals.GetEffects()) {
        if (effect.kind == WeaponVisualEffectKind::Match)
            EXPECT_EQ(effect.lifetime, 145);
        if (effect.kind == WeaponVisualEffectKind::CigarSmoke)
            EXPECT_EQ(effect.lifetime, 55);
        if (effect.kind == WeaponVisualEffectKind::Cigar)
            EXPECT_EQ(effect.lifetime, 243);
    }
    router->Observe(*soldier_);
    soldier_->emote.cue_ages = {}; // A correction must not make old cues fresh again.
    router->Observe(*soldier_);
    EXPECT_EQ(emissions, 1);
    EXPECT_EQ(sounds, 1);
}

TEST_F(EmoteTest, SkippedInstancesRecoverLatestCigarMilestonesOnce)
{
    auto audio = std::make_shared<AudioEvents>();
    std::vector<std::uint32_t> emissions;
    auto router = EmotePresentation::Create(
      *world_->GetStateManager(),
      world_->GetWorldEvents(),
      audio,
      [&](const Soldier&, std::uint32_t cues) { emissions.push_back(cues); });
    soldier_->emote.instance = 7;
    soldier_->emote.cues = static_cast<std::uint32_t>(EmoteCue::Light);
    soldier_->emote.cue_ages[3] = 5;
    router->Observe(*soldier_);
    router->Observe(*soldier_);
    ASSERT_EQ(emissions.size(), 1);
    EXPECT_EQ(emissions.front(), static_cast<std::uint32_t>(EmoteCue::Light));
    soldier_->emote.instance = 6;
    router->Observe(*soldier_);
    EXPECT_EQ(emissions.size(), 1);
}

TEST_F(EmoteTest, JoiningAfterCigarParticlesExpireDoesNotRecreateThem)
{
    soldier_->emote.instance = 7;
    soldier_->emote.cues = static_cast<std::uint32_t>(EmoteCue::Light) |
                           static_cast<std::uint32_t>(EmoteCue::PuffFirst) |
                           static_cast<std::uint32_t>(EmoteCue::Discard);
    soldier_->emote.cue_ages.fill(MAX_EMOTE_CUE_AGE);
    auto audio = std::make_shared<AudioEvents>();
    unsigned int emissions = 0;
    auto router = EmotePresentation::Create(*world_->GetStateManager(),
                                            world_->GetWorldEvents(),
                                            audio,
                                            [&](const Soldier&, std::uint32_t) { ++emissions; });
    EXPECT_EQ(emissions, 0);
}

TEST_F(EmoteTest, CigarMilestonesRetainPositionsWhenAnimationOrFacingChanges)
{
    soldier_->direction = -1;
    soldier_->skeleton->SetPos(12, { 10, 20 });
    soldier_->skeleton->SetPos(15, { 30, 40 });
    Begin(Emote::Smoke);
    At(26);
    At(37);
    At(26);
    soldier_->skeleton->SetPos(12, { 100, 200 });
    soldier_->skeleton->SetPos(15, { 300, 400 });
    soldier_->direction = 1;
    WeaponVisualEffects visuals;
    visuals.ObserveEmote(*soldier_, static_cast<std::uint32_t>(EmoteCue::Light));
    ASSERT_EQ(visuals.GetEffects().size(), 2);
    for (const auto& effect : visuals.GetEffects()) {
        if (effect.kind == WeaponVisualEffectKind::Match) {
            EXPECT_EQ(effect.position, glm::vec2(30, 40));
            EXPECT_FLOAT_EQ(effect.velocity.x, -0.5F);
        }
        if (effect.kind == WeaponVisualEffectKind::CigarSmoke) {
            EXPECT_EQ(effect.position, glm::vec2(6, 20));
        }
    }
}

TEST_F(EmoteTest, PausedLateObserverDoesNotPlayCigarCatchUp)
{
    soldier_->emote.instance = 5;
    soldier_->emote.cues = static_cast<std::uint32_t>(EmoteCue::Light);
    world_->GetStateManager()->PauseGame();
    auto audio = std::make_shared<AudioEvents>();
    unsigned int emissions = 0;
    auto router = EmotePresentation::Create(*world_->GetStateManager(),
                                            world_->GetWorldEvents(),
                                            audio,
                                            [&](const Soldier&, std::uint32_t) { ++emissions; });
    EXPECT_EQ(emissions, 0);
    world_->GetStateManager()->UnPauseGame();
    router->Observe(*soldier_);
    EXPECT_EQ(emissions, 0);
}
