#pragma once

#include <stdint.h>
#include <stdio.h>

#include "linear_math/vector.h"

namespace Render
{
  struct vertex
  {
    vec3 Position;
    vec3 Normal;
    struct UV
    {
      float U;
      float V;
    } UV;
  };

  struct skinned_vertex
  {
    vec3    Position;
    vec3    Normal;
    vec3    UV;
    vec3    BoneWeights;
    int32_t BoneIndex;
  };

  enum mesh_attribute_mask
  {
    MAM_UseNormals = 1,
    MAM_UseUVs     = 2,
    MAM_FlipZ      = 4,
  };

  struct mesh
  {
    uint32_t VAO;
    uint32_t VBO;
    uint32_t EBO;

    vertex*   Vertices;
    uint32_t* Indices;

    int32_t VerticeCount;
    int32_t IndiceCount;

    bool HasUVs;
  };

#if 0
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
#endif

  struct skinned_mesh
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

  void PrintMesh(const mesh* Mesh);

  inline void
  PrintMeshHeader(const mesh* Mesh)
  {
    printf("MESH HEADER\n");
    printf("VerticeCount: %d\n", Mesh->VerticeCount);
    printf("IndiceCount: %d\n", Mesh->IndiceCount);

#if 0
  for(int i = 0; i < Mesh->VerticeCount; i++)
  {
    printf("%d: Pos{ %f %f %f }; Norm{ %f %f %f }; UV{ %f %f %f }", i, );
  }

  printf("INDICES:\n");
  for(int i = 0; i < Mesh->IndiceCount; i++)
  {
    printf("%d: %d\n", Mesh->Indices[i]);
  }
#endif
  }
  inline void
  PrintMeshHeader(const mesh* Mesh, int MeshIndex)
  {
    printf("MESH HEADER #%d \n", MeshIndex);
    printf("VerticeCount: %d\n", Mesh->VerticeCount);
    printf("IndiceCount: %d\n", Mesh->IndiceCount);
  }
  void SetUpMesh(Render::mesh* Mesh);
}
