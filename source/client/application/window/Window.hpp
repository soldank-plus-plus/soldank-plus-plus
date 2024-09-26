#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__

#include "core/math/Glm.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <cstdint>

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

    void SetCursorMode(CursorMode cursor_mode) const;
    void SetTitle(const char* title) const;
    void SetWindowSize(uint32_t width, uint32_t height) const;
    glm::ivec2 GetWindowSize() const;
    float GetAspectRatio() const;
    glm::vec2 GetRealCursorPos() const;

    void Create();
    void Close();
    void Destroy();

    void MakeContextCurrent();
    static void PollInput();
    void SwapBuffers();
    bool ShouldClose();

    void ResizeCallback(GLFWwindow* window, int width, int height);
    static void GLFWErrorCallback(int error, const char* description);

private:
    int width_;
    int height_;
    std::string title_;
    GLFWwindow* glfw_window_;
};
} // namespace Soldank

#endif
