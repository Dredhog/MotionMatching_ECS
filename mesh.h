#pragma once

#include <stdint.h>

namespace Mesh
{
  struct mesh
  {
    uint32_t VAO;

    uint32_t VBO;
    uint32_t EBO;

    float*    Floats;
    uint32_t* Indices;

    int32_t VerticeCount;
    int32_t IndiceCount;

    int32_t Offsets[3];
    int32_t FloatsPerVertex;
    int32_t AttributesPerVertex;
    bool    UseUVs;
    bool    UseNormals;
  };

  enum mesh_attribute_mask
  {
    MAM_UseNormals = 1,
    MAM_UseUVs     = 2,
    MAM_FlipZ      = 4,
  };

  void
  PrintMesh(const mesh* Mesh)
  {
    printf("VertCount : %d, IndCount: %d\n", Mesh->VerticeCount, Mesh->IndiceCount);
    printf("UseUVs    : %d,	UseNormals : %d\n", Mesh->UseUVs, Mesh->UseNormals);
    printf("FloatsPerV: %d, AttribsPerV: %d\n", Mesh->FloatsPerVertex, Mesh->AttributesPerVertex);
    printf("PosOffset : %d, UVOffset : %d, NormOffset: %d\n", Mesh->Offsets[0], Mesh->Offsets[1],
           Mesh->Offsets[2]);
    for(int i = 0; i < Mesh->VerticeCount; i++)
    {
      int VertexStartFloat = i * Mesh->FloatsPerVertex;

      printf("%d: P:{ %5.2f %5.2f %5.2f }\t", i,
             (double)Mesh->Floats[VertexStartFloat + Mesh->Offsets[0]],
             (double)Mesh->Floats[VertexStartFloat + Mesh->Offsets[0] + 1],
             (double)Mesh->Floats[VertexStartFloat + Mesh->Offsets[0] + 2]);
      if(Mesh->UseUVs)
      {
        printf("UV:{ %5.2f %5.2f }\t", (double)Mesh->Floats[VertexStartFloat + Mesh->Offsets[1]],
               (double)Mesh->Floats[VertexStartFloat + Mesh->Offsets[1] + 1]);
      }
      if(Mesh->UseNormals)
      {
        printf("N:{ %5.2f %5.2f %5.2f }\n",
               (double)Mesh->Floats[VertexStartFloat + Mesh->Offsets[2]],
               (double)Mesh->Floats[VertexStartFloat + Mesh->Offsets[2] + 1],
               (double)Mesh->Floats[VertexStartFloat + Mesh->Offsets[2] + 2]);
      }
    }
    printf("INDICES\n");
    for(int i = 0; i < Mesh->IndiceCount; i++)
    {
      printf("%d: %d\n", i, Mesh->Indices[i]);
    }
  }
}
