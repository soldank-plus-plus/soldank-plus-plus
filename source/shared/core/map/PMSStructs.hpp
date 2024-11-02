#ifndef __PMS_STRUCTS_HPP__
#define __PMS_STRUCTS_HPP__

#include "PMSConstants.hpp"
#include "PMSEnums.hpp"
#include <vector>
#include <cmath>
#include <array>
#include <string>

namespace Soldank
{
struct DOSTime
{
    unsigned short second : 5, minute : 6, hour : 5;
};

struct DOSDate
{
    unsigned short day : 5, month : 4, year : 7;
};

struct PMSCollider
{
    int active;
    float x, y, radius;
};

struct PMSColor
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;

    PMSColor()
        : blue(255)
        , green(255)
        , red(255)
        , alpha(255)
    {
    }

    PMSColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
        : blue(b)
        , green(g)
        , red(r)
        , alpha(a)
    {
    }
};

struct PMSVector
{
    float x;
    float y;
    float z;
};

struct PMSVertex
{
    float x{};
    float y{};
    float z{};
    // Those are most likely texture-related
    float rhw{};
    PMSColor color;
    // S corresponds to X axis
    float texture_s{};
    // T corresponds to Y axis
    float texture_t{};
};

struct PMSPolygon
{
    unsigned int id{};
    std::array<PMSVertex, 3> vertices;
    std::array<PMSVector, 3> perpendiculars{};
    PMSPolygonType polygon_type{};
    float bounciness{};

    /**
     * \brief Checks if the vertices are arranged in clock-wise order.
     * http://stackoverflow.com/questions/1165647/how-to-determine-if-a-list-of-polygon-points-are-in-clockwise-order
     */
    bool AreVerticesClockwise()
    {
        float sum = 0.0F;

        for (unsigned int i = 0; i < 3; ++i) {
            unsigned int j = i + 1;
            if (j > 2) {
                j = 0;
            }

            sum += (vertices.at(j).x - vertices.at(i).x) * (vertices.at(j).y + vertices.at(i).y);
        }

        return sum < 0.0F;
    }
};

struct PMSScenery
{
    bool active{};
    unsigned short style{};
    int width{};
    int height{};
    float x{};
    float y{};
    float rotation{};
    float scale_x{};
    float scale_y{};
    int alpha{};
    PMSColor color;
    int level{};
};

struct PMSTimestamp
{
    DOSTime time;
    DOSDate date;
};

struct PMSSceneryType
{
    std::string name;
    PMSTimestamp timestamp;
};

struct PMSSector
{
    std::vector<unsigned short> polygons;
    std::array<float, 4> boundaries;
};

struct PMSSpawnPoint
{
    int active, x, y;
    PMSSpawnPointType type;
};

struct PMSWayPoint
{
    bool active;
    std::array<unsigned char, 3> filler1;
    int id;
    int x, y;
    bool left, right, up, down, jet;
    unsigned char path;
    PMSSpecialAction special_action;
    unsigned char c2, c3; //?
    std::array<unsigned char, 3> filler2;
    int connections_count;
    std::array<int, 20> connections;
};
} // namespace Soldank

#endif
