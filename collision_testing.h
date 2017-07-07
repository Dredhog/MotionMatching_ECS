#pragma once

#include "collision.h"

#if 0
void
CollisionTesting(Render::model* ModelA, Render::model* ModelB)
{
  contact_point Simplex[4];

  for(int i = 0; i < ModelA->MeshCount; i++)
  {
    for(int j = 0; j < ModelB->MeshCount; j++)
    {
      bool IsColliding = GJK(Simplex, ModelA->Meshes[i], ModelB->Meshes[j]);

      if(IsColliding)
      {
        printf("Collision detected between ModelA.Mesh[%d] and ModelB.Mesh[%d]!\n", i, j);

        vec3 SolutionVector;
        vec3 CollisionPoint;
        EPA(&SolutionVector, &CollisionPoint, Simplex, ModelA->Meshes[i], ModelB->Meshes[j]);

        printf("SolutionVector = { %f, %f, %f }\n", SolutionVector.X, SolutionVector.Y, SolutionVector.Z);
        printf("CollisionPoint = { %f, %f, %f }\n", CollisionPoint.X, CollisionPoint.Y, CollisionPoint.Z);
      }
      else
      {
        // printf("No collision detected.\n");
      }
    }
  }
}
#else
void
CollisionTesting()
{
  contact_point Simplex[4];

  Render::mesh MeshA;
  MeshA.VerticeCount = 8;
  MeshA.Vertices     = (Render::vertex*)malloc(sizeof(Render::vertex) * MeshA.VerticeCount);
  MeshA.Vertices[0]  = { 1.0f, 1.0f, 1.0f };
  MeshA.Vertices[1]  = { 1.0f, 1.0f, -1.0f };
  MeshA.Vertices[2]  = { 1.0f, -1.0f, -1.0f };
  MeshA.Vertices[3]  = { 1.0f, -1.0f, 1.0f };
  MeshA.Vertices[4]  = { -1.0f, -1.0f, 1.0f };
  MeshA.Vertices[5]  = { -1.0f, -1.0f, -1.0f };
  MeshA.Vertices[6]  = { -1.0f, 1.0f, -1.0f };
  MeshA.Vertices[7]  = { -1.0f, 1.0f, 1.0f };

  Render::mesh MeshB;
#if 1
  MeshB.VerticeCount = 5;
  MeshB.Vertices     = (Render::vertex*)malloc(sizeof(Render::vertex) * MeshB.VerticeCount);
  MeshB.Vertices[0]  = { 1.5f, -1.0f, 0.0f };
  MeshB.Vertices[1]  = { -2.0f, 1.75f, -0.75f };
  MeshB.Vertices[2]  = { 2.5f, 1.0f, 0.0f };
  MeshB.Vertices[3]  = { 1.5f, -0.5f, 0.0f };
  MeshB.Vertices[4]  = { 4.0f, 1.0f, 3.0f };
#else
  MeshB.VerticeCount = 8;
  MeshB.Vertices     = (Render::vertex*)malloc(sizeof(Render::vertex) * MeshB.VerticeCount);
  MeshB.Vertices[0]  = { 1.5f, 1.0f, 1.0f };
  MeshB.Vertices[1]  = { 1.5f, 1.0f, -1.0f };
  MeshB.Vertices[2]  = { 1.5f, -1.0f, -1.0f };
  MeshB.Vertices[3]  = { 1.5f, -1.0f, 1.0f };
  MeshB.Vertices[4]  = { -0.5f, -1.0f, 1.0f };
  MeshB.Vertices[5]  = { -0.5f, -1.0f, -1.0f };
  MeshB.Vertices[6]  = { -0.5f, 1.0f, -1.0f };
  MeshB.Vertices[7]  = { -0.5f, 1.0f, 1.0f };
#endif

  bool IsColliding = GJK(Simplex, &MeshA, &MeshB);

  if(IsColliding)
  {
    printf("Collision detected between ModelA.Mesh[%d] and ModelB.Mesh[%d]!\n", 0, 0);

    vec3 SolutionVector = {};
    vec3 CollisionPoint = {};
    EPA(&SolutionVector, &CollisionPoint, Simplex, &MeshA, &MeshB);

    printf("SolutionVector = { %f, %f, %f }\n", SolutionVector.X, SolutionVector.Y, SolutionVector.Z);
    printf("CollisionPoint = { %f, %f, %f }\n", CollisionPoint.X, CollisionPoint.Y, CollisionPoint.Z);
  }
  else
  {
    printf("No collision detected.\n");
  }

  free(MeshA.Vertices);
  free(MeshB.Vertices);
}
#endif
