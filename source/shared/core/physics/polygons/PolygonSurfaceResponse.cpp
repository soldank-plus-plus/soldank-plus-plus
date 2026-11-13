module;

#include <cmath>

export module Shared.Core.Physics.Polygons.PolygonSurfaceResponse;

import Extern.Glm;
import Shared.Core.Map.Polygons.PolygonTraits;
import Shared.Core.Animations;
import Shared.Core.Physics.Constants;

export namespace Soldank::PolygonSurfaceResponse
{
glm::vec2 BounceImpulse(glm::vec2 normal, glm::vec2 incoming_velocity, float strength)
{
    const float length = glm::length(normal);
    if (length <= 0.000001F || !std::isfinite(strength) || strength < 0.0F) {
        return {};
    }
    return normal / length * glm::length(incoming_velocity) * strength;
}
struct FrictionResult
{
    glm::vec2 velocity;
    glm::vec2 force;
};
FrictionResult Friction(PolygonSurface surface,
                        AnimationType animation,
                        unsigned int frame,
                        bool down_moving,
                        glm::vec2 step,
                        glm::vec2 velocity,
                        glm::vec2 force)
{
    const bool resting = animation == AnimationType::Stand || animation == AnimationType::Fall ||
                         animation == AnimationType::Crouch || animation == AnimationType::Prone ||
                         animation == AnimationType::ProneMove ||
                         animation == AnimationType::GetUp || animation == AnimationType::Mercy ||
                         animation == AnimationType::Mercy2 || animation == AnimationType::Own;
    if (resting) {
        if (step.y > PhysicsConstants::SLIDELIMIT && surface == PolygonSurface::Ordinary) {
            if (animation == AnimationType::Stand || animation == AnimationType::Fall ||
                animation == AnimationType::Crouch || animation == AnimationType::ProneMove ||
                (animation == AnimationType::Prone && frame > 24 && !down_moving)) {
                velocity *= glm::vec2(PhysicsConstants::STANDSURFACECOEFX,
                                      PhysicsConstants::STANDSURFACECOEFY);
                if (animation != AnimationType::ProneMove) {
                    force.x -= velocity.x;
                }
            } else if (animation == AnimationType::GetUp ||
                       (animation == AnimationType::Prone && frame <= 24)) {
                velocity *=
                  glm::vec2(PhysicsConstants::SURFACECOEFX, PhysicsConstants::SURFACECOEFY);
            }
        }
    } else if (animation == AnimationType::CrouchRun || animation == AnimationType::CrouchRunBack) {
        velocity *= glm::vec2(PhysicsConstants::CROUCHMOVESURFACECOEFX,
                              PhysicsConstants::CROUCHMOVESURFACECOEFY);
    } else {
        velocity *= glm::vec2(PhysicsConstants::SURFACECOEFX, PhysicsConstants::SURFACECOEFY);
    }
    return { .velocity = velocity, .force = force };
}
} // namespace Soldank::PolygonSurfaceResponse
