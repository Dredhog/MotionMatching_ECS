#pragma once

#include "asset.h"
#include "file_io.h"
#include "stack_allocator.h"

void
CheckedLoadAndSetUpAsset(Memory::stack_allocator* Alloc, const char* RelativePath,
                         Render::model** OutputModel, Anim::skeleton** OutputSkeleton)
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
  Render::PrintModelHeader(*OutputModel);

  if(OutputSkeleton)
  {
    assert((Anim::skeleton*)AssetHeader->Skeleton);
  }

  if((Anim::skeleton*)AssetHeader->Skeleton && OutputSkeleton)
  {
    *OutputSkeleton = (Anim::skeleton*)AssetHeader->Skeleton;
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
