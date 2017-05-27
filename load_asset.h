#pragma once

#include "asset.h"
#include "file_io.h"
#include "load_shader.h"
#include "stack_alloc.h"

void
CheckedLoadAndSetUpModel(Memory::stack_allocator* Alloc, const char* RelativePath,
                         Render::model** OutputModel)
{
  debug_read_file_result AssetReadResult = ReadEntireFile(Alloc, RelativePath);

  assert(AssetReadResult.Contents);
  Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

  UnpackAsset(AssetHeader);

  *OutputModel = (Render::model*)AssetHeader->Model;
  assert(OutputModel);

  for(int i = 0; i < (*OutputModel)->MeshCount; i++)
  {
    SetUpMesh((*OutputModel)->Meshes[i]);
  }
}

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
