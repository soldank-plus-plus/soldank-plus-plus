#include "Window.hpp"

#include "application/config/Config.hpp"
#include "application/input/Keyboard.hpp"
#include "application/input/Mouse.hpp"
#include "application/window/Window.hpp"

#include "spdlog/spdlog.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

namespace Soldank
{
Window::Window()
    : Window("Soldank++", Config::INITIAL_WINDOW_WIDTH, Config::INITIAL_WINDOWS_HEIGHT)
{
}

Window::Window(std::string title, int width, int height)
    : title_(std::move(title))
    , glfw_window_(nullptr)
    , current_cursor_mode_(CursorMode::Locked)
{
    if (glfwInit() == 0) {
        spdlog::error("Error: Failed to initialize GLFW");
        throw "Error: Failed to initialize GLFW";
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    width_ = width;
    height_ = height;
}

Window::~Window()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

void Window::Create(WindowSizeMode window_size_mode)
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor == nullptr) {
        spdlog::error("Error: Failed to get primary monitor");
        throw "Error: Failed to get primary monitor";
    }

    const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);
    if (video_mode == nullptr) {
        spdlog::error("Error: Failed to get video mode");
        throw "Error: Failed to get video mode";
    }

    switch (window_size_mode) {
        case WindowSizeMode::Fullscreen: {
            // TODO: there is a bug where alt-tabbing and then alt-tabbing back makes the scene
            // disappear. This is a quick workaround which will prevent alt-tabbing out of the game.
            // Need to handle it gracefully.
            glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

            width_ = video_mode->width;
            height_ = video_mode->height;
            glfw_window_ = glfwCreateWindow(width_, height_, title_.c_str(), monitor, nullptr);
            break;
        }
        case WindowSizeMode::BorderlessFullscreen: {
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
        spdlog::error("Error: Failed to create window.");
        throw "Error: Failed to create window.";
    }

    glfwMakeContextCurrent(glfw_window_);

    glfwSwapInterval(0);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        spdlog::error("Error: Failed to initialize GLAD.");
        glfwTerminate();
        throw "Error: Failed to initialize GLAD.";
    }

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
    SetCursorMode(CursorMode::Locked);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(glfw_window_, true);
    ImGui_ImplOpenGL3_Init();
}

void Window::SetCursorMode(CursorMode cursor_mode)
{
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

CursorMode Window::GetCursorMode() const
{
    return current_cursor_mode_;
}

void Window::SetTitle(const char* title) const
{
    glfwSetWindowTitle(glfw_window_, title);
}

void Window::SetWindowSize(const int width, const int height)
{
    width_ = width;
    height_ = height;
    glfwSetWindowSize(glfw_window_, width, height);
}

glm::ivec2 Window::GetWindowSize() const
{
    glm::ivec2 window_size;
    glfwGetWindowSize(glfw_window_, &window_size.x, &window_size.y);
    return window_size;
}

float Window::GetAspectRatio() const
{
    return static_cast<float>(width_) / static_cast<float>(height_);
}

glm::vec2 Window::GetCursorScreenPosition() const
{
    glm::dvec2 cursor_position;
    glfwGetCursorPos(glfw_window_, &cursor_position.x, &cursor_position.y);
    return cursor_position;
}

void Window::MakeContextCurrent()
{
    glfwMakeContextCurrent(glfw_window_);
}

void Window::PollInput()
{
    glfwPollEvents();
}

void Window::SwapBuffers()
{
    glfwSwapBuffers(glfw_window_);
}

bool Window::ShouldClose()
{
    return glfwWindowShouldClose(glfw_window_) != 0;
}

void Window::Close()
{
    glfwSetWindowShouldClose(glfw_window_, 1);
}

void Window::ResizeCallback(GLFWwindow* /*window*/, const int width, const int height)
{
    width_ = width;
    height_ = height;
    glViewport(0, 0, width, height);
    event_screen_resized_.Notify({ width, height });
}

void Window::GLFWErrorCallback(int error, const char* description)
{
    spdlog::error("Error({}): {}", error, description);
}

void Window::RegisterOnScreenResizedObserver(
  std::function<void(glm::vec2)> on_screen_resized_observer)
{
    event_screen_resized_.AddObserver(on_screen_resized_observer);
}
} // namespace Soldank
