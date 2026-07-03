module;

#include "application/config/Config.hpp"

#include "core/math/Glm.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <string>
#include <functional>

export module Application.Window;

import Application.Input.Keyboard;
import Application.Input.Mouse;

import Shared.Core.Utility.Observable;

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

class Window
{
public:
    Window()
        : Window("Soldank++", Config::INITIAL_WINDOW_WIDTH, Config::INITIAL_WINDOWS_HEIGHT)
    {
    }

    Window(std::string title, int width, int height)
        : title_(std::move(title))
        , glfw_window_(nullptr)
        , current_cursor_mode_(CursorMode::Locked)
    {
        if (glfwInit() == 0) {
            Spdlog::error("Error: Failed to initialize GLFW");
            throw "Error: Failed to initialize GLFW";
        }

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
#ifdef __EMSCRIPTEN__
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

        width_ = width;
        height_ = height;
    }

    Window(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;

    ~Window()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwTerminate();
    }

    void SetCursorMode(CursorMode cursor_mode)
    {
#ifdef __EMSCRIPTEN__
        if (cursor_mode == CursorMode::Locked) {
            cursor_mode = CursorMode::Normal;
        }
#endif

        int glfw_cursor_mode = GLFW_CURSOR_HIDDEN;
        switch (cursor_mode) {
            case CursorMode::Locked:
                glfw_cursor_mode = GLFW_CURSOR_DISABLED;
                break;
            case CursorMode::Normal:
                glfw_cursor_mode = GLFW_CURSOR_NORMAL;
                break;
            case CursorMode::Hidden:
                glfw_cursor_mode = GLFW_CURSOR_HIDDEN;
                break;
        }
        glfwSetInputMode(glfw_window_, GLFW_CURSOR, glfw_cursor_mode);
        current_cursor_mode_ = cursor_mode;
    }

    CursorMode GetCursorMode() const { return current_cursor_mode_; }

    glm::vec2 GetCursorScreenPosition() const
    {
        glm::dvec2 cursor_position;
        glfwGetCursorPos(glfw_window_, &cursor_position.x, &cursor_position.y);
        return cursor_position;
    }

    void SetTitle(const char* title) const { glfwSetWindowTitle(glfw_window_, title); }

    void SetWindowSize(const int width, const int height)
    {
        width_ = width;
        height_ = height;
        glfwSetWindowSize(glfw_window_, width, height);
    }

    glm::ivec2 GetWindowSize() const
    {
        glm::ivec2 window_size;
        glfwGetWindowSize(glfw_window_, &window_size.x, &window_size.y);
        return window_size;
    }

    float GetAspectRatio() const
    {
        return static_cast<float>(width_) / static_cast<float>(height_);
    }

    void Create(WindowSizeMode window_size_mode)
    {
#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
        window_size_mode = WindowSizeMode::Windowed;
#endif

        switch (window_size_mode) {
            case WindowSizeMode::Fullscreen: {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                if (monitor == nullptr) {
                    Spdlog::error("Error: Failed to get primary monitor");
                    throw "Error: Failed to get primary monitor";
                }

                const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);
                if (video_mode == nullptr) {
                    Spdlog::error("Error: Failed to get video mode");
                    throw "Error: Failed to get video mode";
                }

                width_ = video_mode->width;
                height_ = video_mode->height;
                glfw_window_ = glfwCreateWindow(width_, height_, title_.c_str(), monitor, nullptr);
                break;
            }
            case WindowSizeMode::BorderlessFullscreen: {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                if (monitor == nullptr) {
                    Spdlog::error("Error: Failed to get primary monitor");
                    throw "Error: Failed to get primary monitor";
                }

                const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);
                if (video_mode == nullptr) {
                    Spdlog::error("Error: Failed to get video mode");
                    throw "Error: Failed to get video mode";
                }

                glfwWindowHint(GLFW_RED_BITS, video_mode->redBits);
                glfwWindowHint(GLFW_GREEN_BITS, video_mode->greenBits);
                glfwWindowHint(GLFW_BLUE_BITS, video_mode->blueBits);
                glfwWindowHint(GLFW_REFRESH_RATE, video_mode->refreshRate);
                width_ = video_mode->width;
                height_ = video_mode->height;
                glfw_window_ = glfwCreateWindow(
                  video_mode->width, video_mode->height, title_.c_str(), monitor, nullptr);
                break;
            }
            case WindowSizeMode::Windowed: {
                glfw_window_ =
                  glfwCreateWindow(width_, height_, title_.c_str(), /* monitor */ nullptr, nullptr);
                break;
            }
        }

        if (nullptr == glfw_window_) {
            Spdlog::error("Error: Failed to create window.");
            throw "Error: Failed to create window.";
        }

        glfwMakeContextCurrent(glfw_window_);

#ifndef __EMSCRIPTEN__
        glfwSwapInterval(0);
#endif

#ifndef __EMSCRIPTEN__
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
            Spdlog::error("Error: Failed to initialize GLAD.");
            glfwTerminate();
            throw "Error: Failed to initialize GLAD.";
        }
#endif

        glfwSetWindowUserPointer(glfw_window_, this);
        glfwSetFramebufferSizeCallback(
          glfw_window_, [](GLFWwindow* glfw_window, int width, int height) {
              auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
              window->ResizeCallback(glfw_window, width, height);
          });
        glfwSetKeyCallback(glfw_window_, Keyboard::KeyCallback);
        glfwSetCursorPosCallback(glfw_window_, Mouse::CursorPosCallback);
        glfwSetScrollCallback(glfw_window_, Mouse::MouseWheelCallback);
        glfwSetMouseButtonCallback(glfw_window_, Mouse::MouseButtonCallback);
        glfwSetErrorCallback(GLFWErrorCallback);
        glfwSetWindowFocusCallback(glfw_window_, [](GLFWwindow* glfw_window, int focused) {
            auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
            window->OnFocusStateChange(glfw_window, focused);
        });
#ifdef __EMSCRIPTEN__
        SetCursorMode(CursorMode::Normal);
#else
        SetCursorMode(CursorMode::Locked);
#endif

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        ImGui_ImplGlfw_InitForOpenGL(glfw_window_, true);
#ifdef __EMSCRIPTEN__
        ImGui_ImplOpenGL3_Init("#version 300 es");
#else
        ImGui_ImplOpenGL3_Init();
#endif
    }

    void Close() { glfwSetWindowShouldClose(glfw_window_, 1); }

    void MakeContextCurrent() { glfwMakeContextCurrent(glfw_window_); }
    static void PollInput() { glfwPollEvents(); }
    void SwapBuffers() { glfwSwapBuffers(glfw_window_); }
    bool ShouldClose() { return glfwWindowShouldClose(glfw_window_) != 0; }

    void ResizeCallback(GLFWwindow* /*window*/, const int width, const int height)
    {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);
        event_screen_resized_.Notify({ width, height });
    }

    void OnFocusStateChange(GLFWwindow* window, int focused)
    {
        if (window != glfw_window_) {
            return;
        }

        if (focused == GL_TRUE) {
            event_window_gained_focus_.Notify();
        } else {
            event_window_lost_focus_.Notify();
        }
    }

    static void GLFWErrorCallback(int error, const char* description)
    {
        Spdlog::error("Error({}): {}", error, description);
    }

    void RegisterOnScreenResizedObserver(std::function<void(glm::vec2)> on_screen_resized_observer)
    {
        event_screen_resized_.AddObserver(on_screen_resized_observer);
    }

    void RegisterOnFocusLossObserver(const std::function<void()>& on_window_lost_focus_observer)
    {
        event_window_lost_focus_.AddObserver(on_window_lost_focus_observer);
    }

    void RegisterOnFocusGainObserver(const std::function<void()>& on_window_gained_focus_observer)
    {
        event_window_gained_focus_.AddObserver(on_window_gained_focus_observer);
    }

private:
    int width_;
    int height_;
    std::string title_;
    GLFWwindow* glfw_window_;

    Observable<glm::vec2> event_screen_resized_;
    Observable<> event_window_lost_focus_;
    Observable<> event_window_gained_focus_;

    CursorMode current_cursor_mode_;
};
} // namespace Soldank
