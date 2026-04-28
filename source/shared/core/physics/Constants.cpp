module;

#include <cstdint>

export module Shared.Core.Physics.Constants;

export namespace Soldank::PhysicsConstants
{
const float SLIDELIMIT = 0.2F;
const float GRAV = 0.06F;
const float SURFACECOEFX = 0.970F;
const float SURFACECOEFY = 0.970F;
const float CROUCHMOVESURFACECOEFX = 0.85F;
const float CROUCHMOVESURFACECOEFY = 0.97F;
const float STANDSURFACECOEFX = 0.00F;
const float STANDSURFACECOEFY = 0.00F;

const std::uint8_t STANCE_STAND = 1;
const std::uint8_t STANCE_CROUCH = 2;
const std::uint8_t STANCE_PRONE = 3;

const float MAX_VELOCITY = 11.0F;
const float SOLDIER_COL_RADIUS = 3.0F;

const float RUNSPEED = 0.118F;
const float RUNSPEEDUP = RUNSPEED / 6.0F;
const float FLYSPEED = 0.03F;
const float JUMPSPEED = 0.66F;
const float CROUCHRUNSPEED = RUNSPEED / 0.6F;
const float PRONESPEED = RUNSPEED * 4.0F;
const float ROLLSPEED = RUNSPEED / 1.2F;
const float JUMPDIRSPEED = 0.30F;
const float JETSPEED = 0.10F;
const int SECOND = 60;

const int DEFAULT_IDLETIME = SECOND * 8;

const int FLAG_HOLDING_FORCEUP = -14;
const int FLAG_STAND_FORCEUP = -16;

const float MINMOVEDELTA = 0.63;

const float SOLDIER_SPEED_WITH_PARACHUTE = -0.5 * 0.06; // Grav
} // namespace Soldank::PhysicsConstants
