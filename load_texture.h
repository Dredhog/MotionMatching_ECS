#pragma once

#include "stack_alloc.h"

#include <GL/glew.h>
#include <SDL2/SDL_image.h>

namespace Texture
{
  uint32_t LoadTexture(const char* FileName);
  uint32_t LoadCubemap(Memory::stack_allocator* const Allocator, char* CubemapPath,
                       char* FileFormat);
}
