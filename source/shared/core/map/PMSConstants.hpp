#ifndef __PMS_CONSTANTS_HPP__
#define __PMS_CONSTANTS_HPP__

namespace Soldank
{
const int MAP_NAME_MAX_LENGTH = 16;
const int DESCRIPTION_MAX_LENGTH = 38;
const int SCENERY_NAME_MAX_LENGTH = 50;
const int TEXTURE_NAME_MAX_LENGTH = 24;

const float MAP_BOUNDARY = 50.0F;
const int MAP_VERSION = 11;

const unsigned int MAX_POLYGONS_COUNT = 5000;
const unsigned int POLYGON_TYPES_COUNT = 24;

const unsigned int MAX_SCENERIES_COUNT = 500;
const unsigned int MAX_SPAWN_POINTS_COUNT = 128;

const int SECTORS_COUNT = 51; // Each map is a 51x51 grid of sectors.
} // namespace Soldank

#endif
