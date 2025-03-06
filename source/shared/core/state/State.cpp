#include "core/state/State.hpp"

#include "core/physics/Particles.hpp"
#include "core/animations/AnimationState.hpp"
#include "core/entities/WeaponParametersFactory.hpp"

#include <memory>
#include <utility>

namespace Soldank
{
State::State(AnimationDataManager& animation_data_manager, std::shared_ptr<ParticleSystem> skeleton)
{
    soldiers.fill(
      { 0,
        animation_data_manager,
        std::move(skeleton),
        std::vector<Weapon>{
          { WeaponParametersFactory::GetParameters(WeaponType::DesertEagles, false) },
          { WeaponParametersFactory::GetParameters(WeaponType::Knife, false) },
          { WeaponParametersFactory::GetParameters(WeaponType::FragGrenade, false) } } });
}
} // namespace Soldank
