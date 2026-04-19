module;

#include "spdlog/spdlog.h"

export module Extern.Spdlog;

export namespace Spdlog
{
namespace level
{
constexpr auto trace = spdlog::level::trace;
constexpr auto debug = spdlog::level::debug;
constexpr auto info = spdlog::level::info;
constexpr auto warn = spdlog::level::warn;
constexpr auto err = spdlog::level::err;
constexpr auto critical = spdlog::level::critical;
constexpr auto off = spdlog::level::off;
constexpr auto n_levels = spdlog::level::n_levels;
} // namespace level

const auto set_level = spdlog::set_level;

template<typename... Args>
constexpr void critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
{
    spdlog::critical(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
constexpr void error(spdlog::format_string_t<Args...> fmt, Args&&... args)
{
    spdlog::error(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
constexpr void warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
{
    spdlog::warn(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
constexpr void info(spdlog::format_string_t<Args...> fmt, Args&&... args)
{
    spdlog::info(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
constexpr void debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
{
    spdlog::debug(fmt, std::forward<Args>(args)...);
}

template<typename T>
constexpr void critical(const T& msg)
{
    spdlog::critical(msg);
}

template<typename T>
constexpr void error(const T& msg)
{
    spdlog::error(msg);
}

template<typename T>
constexpr void warn(const T& msg)
{
    spdlog::warn(msg);
}

template<typename T>
constexpr void info(const T& msg)
{
    spdlog::info(msg);
}

template<typename T>
constexpr void debug(const T& msg)
{
    spdlog::debug(msg);
}
} // namespace Spdlog
