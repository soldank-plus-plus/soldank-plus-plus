module;

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

export module Extern.Glm;

export using namespace glm;

export namespace glm
{
using ::glm::dvec2;
using ::glm::ivec2;
using ::glm::mat4;
using ::glm::uvec2;
using ::glm::vec2;
using ::glm::vec3;
using ::glm::vec4;
using ::glm::dot;
using ::glm::length;
using ::glm::mix;
using ::glm::ortho;
using ::glm::rotate;
using ::glm::scale;
using ::glm::translate;
using ::glm::value_ptr;
using ::glm::operator+;
using ::glm::operator-;
using ::glm::operator*;
using ::glm::operator/;
}
