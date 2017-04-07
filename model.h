#pragma once

#include <stdint.h>
#include <stdio.h>
#include <cassert>

#include "mesh.h"

namespace Render
{
  struct model
  {
    int32_t        MeshCount;
    Render::mesh** Meshes;
  };

  inline void
  PrintModelHeader(Render::model* Model)
  {
    printf("MODEL HEADER\n");
    printf("MeshCount: %d\n", Model->MeshCount);
  }

  inline void
  PrintModel(Render::model* Model)
  {
    PrintModelHeader(Model);

    assert(Model->MeshCount >= 0);
    for(int i = 0; i < Model->MeshCount; i++)
    {
      Render::PrintMeshHeader(Model->Meshes[i], i);
    }
  }
}
