module;

#include "application/config/Config.hpp"

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <cstdint>

export module Application.WindowBackendAdapter;

import Extern.Spdlog;

export namespace Soldank
{
enum class CursorMode : std::uint8_t
{
    Locked,
    Normal,
    Hidden
};

enum class WindowSizeMode : std::uint8_t
{
    Fullscreen = 0,
    BorderlessFullscreen,
    Windowed
};

class WindowBackendAdapter
{
public:
    static void ApplyGlfwWindowHints()
    {
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
#ifdef __EMSCRIPTEN__
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    }

    static WindowSizeMode AdjustWindowSizeMode(WindowSizeMode window_size_mode)
    {
#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
        return WindowSizeMode::Windowed;
#else
        return window_size_mode;
#endif
    }

    static WindowSizeMode GetDefaultWindowSizeMode()
    {
#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
        return WindowSizeMode::Windowed;
#else
        return WindowSizeMode::Fullscreen;
#endif
    }

    static CursorMode AdjustCursorMode(CursorMode cursor_mode)
    {
#ifdef __EMSCRIPTEN__
        if (cursor_mode == CursorMode::Locked) {
            return CursorMode::Normal;
        }
#endif
        return cursor_mode;
    }

    static int ToGlfwCursorMode(CursorMode cursor_mode)
    {
        switch (cursor_mode) {
            case CursorMode::Locked:
                return GLFW_CURSOR_DISABLED;
            case CursorMode::Normal:
                return GLFW_CURSOR_NORMAL;
            case CursorMode::Hidden:
                return GLFW_CURSOR_HIDDEN;
        }
        return GLFW_CURSOR_HIDDEN;
    }

    static void ConfigureSwapInterval()
    {
#ifndef __EMSCRIPTEN__
        glfwSwapInterval(0);
#endif
    }

    static void LoadOpenGl()
    {
#ifndef __EMSCRIPTEN__
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
            Spdlog::error("Error: Failed to initialize GLAD.");
            glfwTerminate();
            throw "Error: Failed to initialize GLAD.";
        }
#endif
    }

    static const char* GetGlslVersion()
    {
#ifdef __EMSCRIPTEN__
        return "#version 300 es";
#else
        return nullptr;
#endif
    }

    static CursorMode GetInitialCursorMode()
    {
#ifdef __EMSCRIPTEN__
        return CursorMode::Normal;
#else
        return CursorMode::Locked;
#endif
    }
};
} // namespace Soldank
