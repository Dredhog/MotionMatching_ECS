#pragma once

#include "stack_alloc.h"

#include <GL/glew.h>

namespace Shader
{

  GLuint CheckedLoadCompileFreeShader(Memory::stack_allocator* Alloc, const char* RelativePath);
}
