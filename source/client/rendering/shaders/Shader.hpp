#ifndef __SHADER_H__
#define __SHADER_H__

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>

namespace Soldank
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
    static void CheckCompileErrors(unsigned int shader, const std::string& type);

private:
    unsigned int id_;
};
} // namespace Soldank

#endif
