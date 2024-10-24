#include "Shader.hpp"

#include "spdlog/spdlog.h"

#include <array>

namespace Soldank
{
Shader::Shader(const char* vertex_shader_source, const char* fragment_shader_source)
{
    unsigned int vertex = 0;
    unsigned int fragment = 0;

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");

    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");

    // shader Program
    id_ = glCreateProgram(); // NOLINT(cppcoreguidelines-prefer-member-initializer)
    glAttachShader(id_, vertex);
    glAttachShader(id_, fragment);
    glLinkProgram(id_);
    CheckCompileErrors(id_, "PROGRAM");

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
    glUseProgram(id_);
}

void Shader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2f(glGetUniformLocation(id_, name.c_str()), value.x, value.y);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3f(glGetUniformLocation(id_, name.c_str()), value.x, value.y, value.z);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4f(glGetUniformLocation(id_, name.c_str()), value.x, value.y, value.z, value.w);
}

void Shader::SetMatrix4(const std::string& name, const glm::mat4& value) const
{
    glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::CheckCompileErrors(unsigned int shader, const std::string& type)
{
    int success = 0;
    std::array<char, 1024> info_log{};
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == 0) {
            glGetShaderInfoLog(shader, 1024, nullptr, info_log.data());
            // TODO: We should log it better
            spdlog::error("ERROR::SHADER_COMPILATION_ERROR of type: {}, {}", type, info_log.data());
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (success == 0) {
            glGetProgramInfoLog(shader, 1024, nullptr, info_log.data());
            // TODO: We should log it better
            spdlog::error("ERROR::PROGRAM_LINKING_ERROR of type: {}, {}", type, info_log.data());
        }
    }
}
} // namespace Soldank
