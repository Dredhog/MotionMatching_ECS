#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <cstdio>
#include <cassert>

#include "stack_allocator.h"

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

  mesh
  LoadOBJMesh(Memory::stack_allocator* ScratchAllocator,
              Memory::stack_allocator* PersistentMemAllocator, const char* FileName,
              uint32_t AttributeMask)
  {
    const int MaxAttributesPerIndex = 3;
    FILE*     FileHandle            = fopen(FileName, "r");
    if(!FileHandle)
    {
      printf("Parse OBJ error: could not find file: %s", FileName);
      return {};
    }

    // DANGER!! Memory corruption if model is too large
    float* Positions =
      (float*)ScratchAllocator->AlignedAlloc(ScratchAllocator->GetCapacity() / 5, 16);
    float* Normals =
      (float*)ScratchAllocator->AlignedAlloc(ScratchAllocator->GetCapacity() / 5, 16);
    float*    UVs = (float*)ScratchAllocator->AlignedAlloc(ScratchAllocator->GetCapacity() / 5, 16);
    uint32_t* Indices =
      (uint32_t*)ScratchAllocator->AlignedAlloc(ScratchAllocator->GetCapacity() / 4, 16);

    int PositionCount = 0;
    int NormalCount   = 0;
    int UVCount       = 0;
    int IndiceCount   = 0;

    mesh Mesh       = {};
    Mesh.UseUVs     = AttributeMask & MAM_UseUVs;
    Mesh.UseNormals = AttributeMask & MAM_UseNormals;

    Mesh.FloatsPerVertex     = 3;
    Mesh.AttributesPerVertex = 1;
    Mesh.Offsets[0]          = 0;

    if(Mesh.UseUVs && Mesh.UseNormals)
    {
      Mesh.FloatsPerVertex     = 8;
      Mesh.AttributesPerVertex = 3;

      Mesh.Offsets[1] = 3;
      Mesh.Offsets[2] = 5;
    }
    else
    {
      if(Mesh.UseUVs)
      {
        Mesh.FloatsPerVertex += 2;
        ++Mesh.AttributesPerVertex;
        Mesh.Offsets[1] = 3;
        Mesh.Offsets[2] = -10;
      }
      else if(Mesh.UseNormals)
      {
        Mesh.FloatsPerVertex += 3;
        ++Mesh.AttributesPerVertex;
        Mesh.Offsets[1] = -10;
        Mesh.Offsets[2] = 3;
      }
    }

    char LineBuffer[128];
    while(!feof(FileHandle))
    {
      const char* ReadLine = fgets(LineBuffer, 128, FileHandle);
      if(!ReadLine)
      {
        break;
      }

      char FirstChar = ReadLine[0];
      if(FirstChar == 'v')
      {
        char SecondChar = ReadLine[1];
        if(SecondChar == ' ')
        {
          sscanf(&ReadLine[2], "%f %f %f ", &Positions[3 * PositionCount],
                 &Positions[3 * PositionCount + 1], &Positions[3 * PositionCount + 2]);
          if(AttributeMask & MAM_FlipZ)
          {
            Positions[3 * PositionCount + 2] *= -1;
          }

          ++PositionCount;
        }
        else if(SecondChar == 't')
        {
          sscanf(&ReadLine[2], " %f %f ", &UVs[2 * UVCount], &UVs[2 * UVCount + 1]);
          ++UVCount;
        }
        else if(SecondChar == 'n')
        {
          sscanf(&ReadLine[2], " %f %f %f ", &Normals[3 * NormalCount],
                 &Normals[3 * NormalCount + 1], &Normals[3 * NormalCount + 2]);
          ++NormalCount;
        }
      }
      else if(FirstChar == 'f')
      {
        int V1[3];
        int V2[3];
        int V3[3];
        if(Mesh.UseUVs && Mesh.UseNormals)
        {
          sscanf(&ReadLine[2], " %d/%d/%d %d/%d/%d %d/%d/%d ", &V1[0], &V1[1], &V1[2], &V2[0],
                 &V2[1], &V2[2], &V3[0], &V3[1], &V3[2]);
        }
        else if(!Mesh.UseUVs && Mesh.UseNormals)
        {
          sscanf(&ReadLine[2], " %d//%d %d//%d %d//%d ", &V1[0], &V1[2], &V2[0], &V2[2], &V3[0],
                 &V3[2]);
        }
        else if(Mesh.UseUVs && !Mesh.UseNormals)
        {
          sscanf(&ReadLine[2], " %d/%d/ %d/%d/ %d/%d/ ", &V1[0], &V1[1], &V2[0], &V2[1], &V3[0],
                 &V3[1]);
        }
        else
        {
          sscanf(&ReadLine[2], " %d %d %d ", &V1[0], &V2[0], &V3[0]);
        }

        Indices[MaxAttributesPerIndex * IndiceCount + 0] = V1[0];
        Indices[MaxAttributesPerIndex * IndiceCount + 1] = V1[1];
        Indices[MaxAttributesPerIndex * IndiceCount + 2] = V1[2];

        Indices[MaxAttributesPerIndex * IndiceCount + 3] = V2[0];
        Indices[MaxAttributesPerIndex * IndiceCount + 4] = V2[1];
        Indices[MaxAttributesPerIndex * IndiceCount + 5] = V2[2];

        Indices[MaxAttributesPerIndex * IndiceCount + 6] = V3[0];
        Indices[MaxAttributesPerIndex * IndiceCount + 7] = V3[1];
        Indices[MaxAttributesPerIndex * IndiceCount + 8] = V3[2];

        IndiceCount += 3;
      }
    }

    Mesh.VerticeCount = IndiceCount;
    Mesh.IndiceCount  = IndiceCount;
    Mesh.Floats =
      (float*)PersistentMemAllocator->AlignedAlloc(Mesh.VerticeCount * Mesh.FloatsPerVertex *
                                                     sizeof(float),
                                                   4);
    Mesh.Indices =
      (uint32_t*)PersistentMemAllocator->AlignedAlloc(Mesh.IndiceCount * sizeof(uint32_t), 4);

    // Write vertices packed to final buffer i.e: ||p.x p.y p.z|u v|n.x n.y n.z||
    for(int i = 0; i < Mesh.IndiceCount; i++)
    {
      int IndexTripletIndex = MaxAttributesPerIndex * i;

      Mesh.Floats[i * Mesh.FloatsPerVertex + Mesh.Offsets[0] + 0] =
        Positions[3 * (Indices[IndexTripletIndex] - 1) + 0];
      Mesh.Floats[i * Mesh.FloatsPerVertex + Mesh.Offsets[0] + 1] =
        Positions[3 * (Indices[IndexTripletIndex] - 1) + 1];
      Mesh.Floats[i * Mesh.FloatsPerVertex + Mesh.Offsets[0] + 2] =
        Positions[3 * (Indices[IndexTripletIndex] - 1) + 2];
      if(Mesh.UseUVs)
      {
        Mesh.Floats[i * Mesh.FloatsPerVertex + Mesh.Offsets[1] + 0] =
          UVs[2 * (Indices[IndexTripletIndex + 1] - 1) + 0];
        Mesh.Floats[i * Mesh.FloatsPerVertex + Mesh.Offsets[1] + 1] =
          UVs[2 * (Indices[IndexTripletIndex + 1] - 1) + 1];
      }
      if(Mesh.UseNormals)
      {
        Mesh.Floats[i * Mesh.FloatsPerVertex + Mesh.Offsets[2] + 0] =
          Normals[3 * (Indices[IndexTripletIndex + 2] - 1) + 0];
        Mesh.Floats[i * Mesh.FloatsPerVertex + Mesh.Offsets[2] + 1] =
          Normals[3 * (Indices[IndexTripletIndex + 2] - 1) + 1];
        Mesh.Floats[i * Mesh.FloatsPerVertex + Mesh.Offsets[2] + 2] =
          Normals[3 * (Indices[IndexTripletIndex + 2] - 1) + 2];
      }
    }
    // Write indices to final buffer
    for(uint32_t i = 0; i < Mesh.IndiceCount; i++)
    {
      Mesh.Indices[i] = i;
    }

    fclose(FileHandle);

    return Mesh;
  }

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
