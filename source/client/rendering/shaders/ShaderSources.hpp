#ifndef __SHADER_SOURCES_HPP__
#define __SHADER_SOURCES_HPP__

namespace Soldank::ShaderSources
{
constexpr const char* const VERTEX_SHADER_SOURCE =
#include "Standard.vs"
  ;

constexpr const char* const FRAGMENT_SHADER_SOURCE =
#include "Standard.fs"
  ;

constexpr const char* const NO_TEXTURE_VERTEX_SHADER_SOURCE =
#include "NoTexture.vs"
  ;

constexpr const char* const NO_TEXTURE_FRAGMENT_SHADER_SOURCE =
#include "NoTexture.fs"
  ;

constexpr const char* const DYNAMIC_COLOR_VERTEX_SHADER_SOURCE =
#include "DynamicColor.vs"
  ;

constexpr const char* const DYNAMIC_COLOR_FRAGMENT_SHADER_SOURCE =
#include "DynamicColor.fs"
  ;

constexpr const char* const DYNAMIC_COLOR_NO_TEXTURE_VERTEX_SHADER_SOURCE =
#include "DynamicColorNoTexture.vs"
  ;

constexpr const char* const DYNAMIC_COLOR_NO_TEXTURE_FRAGMENT_SHADER_SOURCE =
#include "DynamicColorNoTexture.fs"
  ;

constexpr const char* const CIRCLE_VERTEX_SHADER_SOURCE =
#include "Circle.vs"
  ;

constexpr const char* const CIRCLE_FRAGMENT_SHADER_SOURCE =
#include "Circle.fs"
  ;

constexpr const char* const FONT_VERTEX_SHADER_SOURCE =
#include "Font.vs"
  ;

constexpr const char* const FONT_FRAGMENT_SHADER_SOURCE =
#include "Font.fs"
  ;

constexpr const char* const GRID_VERTEX_SHADER_SOURCE =
#include "Grid.vs"
  ;

constexpr const char* const GRID_FRAGMENT_SHADER_SOURCE =
#include "Grid.fs"
  ;
} // namespace Soldank::ShaderSources

#endif
