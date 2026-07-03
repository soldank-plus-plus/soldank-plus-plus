#pragma once

#include <GLES3/gl3.h>

using GLADloadproc = void* (*)(const char*);

static inline int gladLoadGLLoader(GLADloadproc)
{
    return 1;
}

#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif

#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif

static inline void glPolygonMode(GLenum, GLenum) {}
