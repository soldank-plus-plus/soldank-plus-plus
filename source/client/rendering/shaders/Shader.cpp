module;

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iostream>

export module Shader;

import Extern.Spdlog;

export namespace Soldank
{
class Shader
{
public:
    Shader(const char* vertex_shader_source, const char* fragment_shader_source);

    ~Shader();

    // it's not safe to be able to copy/move Shaders because we would also need to take care of the
    // created OpenGL Programs
    Shader(const Shader&) = delete;
    Shader& operator=(Shader other) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&& other) = delete;

    void Use() const;
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMatrix4(const std::string& name, const glm::mat4& value) const;
    static bool CheckCompileErrors(unsigned int shader, const std::string& type);

private:
    bool IsReady() const;

    unsigned int id_;
    bool is_linked_;
};
} // namespace Soldank

namespace Soldank
{
#ifdef __EMSCRIPTEN__
namespace
{
void ReplaceAll(std::string& source, std::string_view from, std::string_view to)
{
    std::size_t position = 0;
    while ((position = source.find(from, position)) != std::string::npos) {
        source.replace(position, from.size(), to);
        position += to.size();
    }
}

std::string BuildWebGLShaderSource(const char* source, GLenum shader_type)
{
    std::string webgl_source = source;
    constexpr std::string_view GLSL_120_VERSION = "#version 120";
    if (const auto version_position = webgl_source.find(GLSL_120_VERSION);
        version_position != std::string::npos) {
        webgl_source.erase(version_position, GLSL_120_VERSION.size());
    }

    ReplaceAll(webgl_source, "attribute ", "in ");
    ReplaceAll(webgl_source, "texture2D(", "texture(");

    if (shader_type == GL_VERTEX_SHADER) {
        ReplaceAll(webgl_source, "varying ", "out ");
        return "#version 300 es\nprecision mediump float;\n" + webgl_source;
    }

    ReplaceAll(webgl_source, "varying ", "in ");
    ReplaceAll(webgl_source, "gl_FragColor", "fragColor");
    return "#version 300 es\nprecision mediump float;\nout vec4 fragColor;\n" + webgl_source;
}
} // namespace
#endif

Shader::Shader(const char* vertex_shader_source, const char* fragment_shader_source)
    : id_(0)
    , is_linked_(false)
{
    unsigned int vertex = 0;
    unsigned int fragment = 0;

#ifdef __EMSCRIPTEN__
    static bool gl_info_logged = false;
    if (!gl_info_logged) {
        gl_info_logged = true;
        const auto* gl_version = glGetString(GL_VERSION);
        const auto* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
        Spdlog::info("WebGL GL_VERSION: {}", gl_version != nullptr
                                               ? reinterpret_cast<const char*>(gl_version)
                                               : "unknown");
        Spdlog::info("WebGL GLSL_VERSION: {}", glsl_version != nullptr
                                                 ? reinterpret_cast<const char*>(glsl_version)
                                                 : "unknown");
    }

    std::string webgl_vertex_shader_source =
      BuildWebGLShaderSource(vertex_shader_source, GL_VERTEX_SHADER);
    std::string webgl_fragment_shader_source =
      BuildWebGLShaderSource(fragment_shader_source, GL_FRAGMENT_SHADER);
    vertex_shader_source = webgl_vertex_shader_source.c_str();
    fragment_shader_source = webgl_fragment_shader_source.c_str();
#endif

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex);
    const bool vertex_compiled = CheckCompileErrors(vertex, "VERTEX");

    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragment);
    const bool fragment_compiled = CheckCompileErrors(fragment, "FRAGMENT");

    // shader Program
    id_ = glCreateProgram();
    glAttachShader(id_, vertex);
    glAttachShader(id_, fragment);
    std::string_view vertex_source_view{ vertex_shader_source };
    if (vertex_source_view.find(" vec4 vertex") != std::string_view::npos) {
        glBindAttribLocation(id_, 0, "vertex");
    } else {
        glBindAttribLocation(id_, 0, "position");
    }
    if (vertex_source_view.find(" vec4 color") != std::string_view::npos) {
        glBindAttribLocation(id_, 1, "color");
    }
    if (vertex_source_view.find(" vec2 texturePosition") != std::string_view::npos) {
        glBindAttribLocation(id_, 2, "texturePosition");
    }
    glLinkProgram(id_);
    is_linked_ = CheckCompileErrors(id_, "PROGRAM");

#ifdef __EMSCRIPTEN__
    if (!vertex_compiled || !fragment_compiled || !is_linked_) {
        std::cerr << "WebGL vertex shader source:\n" << vertex_shader_source << '\n';
        std::cerr << "WebGL fragment shader source:\n" << fragment_shader_source << '\n';
    }
#endif

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader()
{
    glDeleteProgram(id_);
}

void Shader::Use() const
{
#ifdef __EMSCRIPTEN__
    if (!IsReady()) {
        static bool logged = false;
        if (!logged) {
            logged = true;
            Spdlog::error("Attempted to use an unlinked WebGL shader program");
        }
        return;
    }
#endif
    glUseProgram(id_);
}

bool Shader::IsReady() const
{
#ifdef __EMSCRIPTEN__
    return is_linked_;
#else
    return true;
#endif
}

void Shader::SetBool(const std::string& name, bool value) const
{
    if (!IsReady()) {
        return;
    }
    glUniform1i(glGetUniformLocation(id_, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const
{
    if (!IsReady()) {
        return;
    }
    glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    if (!IsReady()) {
        return;
    }
    glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) const
{
    if (!IsReady()) {
        return;
    }
    glUniform2f(glGetUniformLocation(id_, name.c_str()), value.x, value.y);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    if (!IsReady()) {
        return;
    }
    glUniform3f(glGetUniformLocation(id_, name.c_str()), value.x, value.y, value.z);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const
{
    if (!IsReady()) {
        return;
    }
    glUniform4f(glGetUniformLocation(id_, name.c_str()), value.x, value.y, value.z, value.w);
}

void Shader::SetMatrix4(const std::string& name, const glm::mat4& value) const
{
    if (!IsReady()) {
        return;
    }
    glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

bool Shader::CheckCompileErrors(unsigned int shader, const std::string& type)
{
    int success = 0;
    std::array<char, 4096> info_log{};
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == 0) {
            glGetShaderInfoLog(
              shader, static_cast<GLsizei>(info_log.size()), nullptr, info_log.data());
            // TODO: We should log it better
            Spdlog::error("ERROR::SHADER_COMPILATION_ERROR of type: {}, {}", type, info_log.data());
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (success == 0) {
            glGetProgramInfoLog(
              shader, static_cast<GLsizei>(info_log.size()), nullptr, info_log.data());
            // TODO: We should log it better
            Spdlog::error("ERROR::PROGRAM_LINKING_ERROR of type: {}, {}", type, info_log.data());
        }
    }

    return success != 0;
}
} // namespace Soldank
