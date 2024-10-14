#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__

#include "core/math/Glm.hpp"
#include "core/utility/Observable.hpp"

#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <cstdint>
#include <functional>

namespace Soldank
{
enum class CursorMode : uint8_t
{
    Locked,
    Normal,
    Hidden
};

class Window
{
public:
    Window();
    Window(std::string title, int width, int height);
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;

    ~Window();

    void SetCursorMode(CursorMode cursor_mode);
    CursorMode GetCursorMode() const;
    void SetTitle(const char* title) const;
    void SetWindowSize(int width, int height);
    glm::ivec2 GetWindowSize() const;
    float GetAspectRatio() const;
    glm::vec2 GetCursorScreenPosition() const;

    void Create();
    void Close();
    void Destroy();

    void MakeContextCurrent();
    static void PollInput();
    void SwapBuffers();
    bool ShouldClose();

    void ResizeCallback(GLFWwindow* window, int width, int height);
    static void GLFWErrorCallback(int error, const char* description);

    void RegisterOnScreenResizedObserver(std::function<void(glm::vec2)> on_screen_resized_observer);

private:
    int width_;
    int height_;
    std::string title_;
    GLFWwindow* glfw_window_;

    Observable<glm::vec2> event_screen_resized_;

    CursorMode current_cursor_mode_;
};
} // namespace Soldank

#endif
