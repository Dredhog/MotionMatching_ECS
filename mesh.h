#pragma once

#include <stdint.h>
#include <stdio.h>

#include "linear_math/vector.h"

#define VERTEX_MAX_BONE_COUNT 4

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
    float   BoneWeights[VERTEX_MAX_BONE_COUNT];
    int32_t BoneIndices[VERTEX_MAX_BONE_COUNT];
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

    bool    HasUVs;
    bool    HasBones;
    int32_t BoneCount;
  };

  void SetUpMesh(Render::mesh* Mesh);
  void PrintMesh(const mesh* Mesh);

  inline void
  PrintMeshHeader(const mesh* Mesh)
  {
    printf("MESH HEADER\n");
    printf("VerticeCount: %d\n", Mesh->VerticeCount);
    printf("IndiceCount: %d\n", Mesh->IndiceCount);
  }

  inline void
  PrintMeshHeader(const mesh* Mesh, int MeshIndex)
  {
    printf("MESH HEADER #%d \n", MeshIndex);
    printf("VerticeCount: %d\n", Mesh->VerticeCount);
    printf("IndiceCount: %d\n", Mesh->IndiceCount);
  }
}
