#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

namespace Soldank::Config
{
constexpr const bool DEBUG_DRAW = false;
constexpr const int INITIAL_WINDOW_WIDTH = 1280;
constexpr const int INITIAL_WINDOWS_HEIGHT = 1024;

// If FPS_LIMIT == 0, then there's no limit
constexpr const int FPS_LIMIT = 200;
} // namespace Soldank::Config

#endif
