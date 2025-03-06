#include "core/physics/SoldierPhysics.hpp"
#include "core/animations/AnimationState.hpp"
#include "core/state/Control.hpp"
#include "core/state/StateManager.hpp"
#include "core/entities/WeaponParametersFactory.hpp"

#include "shared_lib_testing/MapBuilder.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace SoldankTesting
{
enum class SoldierAnimationPart
{
    Legs = 0,
    Body
};

struct SoldierExpectedAnimationState
{
    SoldierAnimationPart part;
    Soldank::AnimationType expected_animation_type;
    unsigned int expected_frame;
    int expected_speed;
    glm::vec2 expected_position_diff_from_origin;
};

struct ControlToChange
{
    Soldank::ControlActionType control_type;
    bool new_state;
};

class SoldierMovementSimulation
{
public:
    SoldierMovementSimulation(const Soldank::IFileReader& file_reader);

    void HoldRight();
    void HoldLeft();
    void HoldJump();

    void HoldRightAt(unsigned int tick);
    void HoldLeftAt(unsigned int tick);
    void HoldJumpAt(unsigned int tick);
    void HoldJetsAt(unsigned int tick);
    void ReleaseRightAt(unsigned int tick);
    void ReleaseLeftAt(unsigned int tick);
    void ReleaseJumpAt(unsigned int tick);
    void ReleaseJetsAt(unsigned int tick);

    void LookLeft();
    void LookRight();

    void AddSoldierExpectedAnimationState(
      unsigned int tick,
      const SoldierExpectedAnimationState& soldier_expected_animation_state);

    void RunUntilSoldierOnGround(unsigned int ticks_limit = 5000);

    void RunFor(unsigned int ticks_to_run);

private:
    using SoldierExpectedAnimationStates = std::vector<SoldierExpectedAnimationState>;

    static void CheckSoldierAnimationStates(
      const Soldank::Soldier& soldier,
      const SoldierExpectedAnimationStates& expected_animation_states,
      const glm::vec2& soldier_position_origin);

    static void CheckSoldierAnimationState(
      const Soldank::Soldier& soldier,
      const SoldierExpectedAnimationState& expected_animation_state,
      const glm::vec2& soldier_position_origin);

    void AddControlToChangeAt(unsigned int tick, const ControlToChange& control_to_change);

    void TurnSoldierLeft();
    void TurnSoldierRight();

    std::unique_ptr<Soldank::StateManager> state_manager_;
    Soldank::AnimationDataManager animation_data_manager_;

    std::unordered_map<unsigned int, SoldierExpectedAnimationStates> animations_to_check_at_tick_;
    std::unordered_map<unsigned int, std::vector<ControlToChange>> controls_to_change_at_tick_;
    bool is_soldier_looking_left_ = false;
};
} // namespace SoldankTesting
