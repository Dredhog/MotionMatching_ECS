#pragma once

#include "collision.h"

void
CollisionTesting(Render::model* ModelA, Render::model* ModelB)
{
  vec3 Simplex[50];

  for(int i = 0; i < ModelA->MeshCount; i++)
  {
    for(int j = 0; j < ModelB->MeshCount; j++)
    {
      bool IsColliding = GJK(Simplex, ModelA->Meshes[i], ModelB->Meshes[j]);

      if(IsColliding)
      {
        printf("Collision detected between ModelA.Mesh[%d] and ModelB.Mesh[%d]!\n", i, j);

        vec3 SolutionVector = EPA(Simplex, 4, ModelA->Meshes[i], ModelB->Meshes[j]);

        printf("SolutionVector = { %f, %f, %f }\n", SolutionVector.X, SolutionVector.Y, SolutionVector.Z);
      }
      else
      {
        // printf("No collision detected.\n");
      }
    }
  }
}
