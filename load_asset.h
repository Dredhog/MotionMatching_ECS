#pragma once

#include "asset.h"
#include "file_io.h"
#include "load_shader.h"
#include "stack_alloc.h"


int32_t
CheckedLoadCompileFreeShader(Memory::stack_allocator* Alloc, const char* RelativePath)
{
  Memory::marker LoadStart = Alloc->GetMarker();
  int32_t        Result    = Shader::LoadShader(Alloc, RelativePath);
  Alloc->FreeToMarker(LoadStart);

  if(Result < 0)
  {
    printf("Shader loading failed!\n");
    assert(false);
  }
  return Result;
}
