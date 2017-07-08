#pragma once

#include "collision.h"

#if 0
void
CollisionTesting(game_state* GameState, const game_input* const Input, Render::mesh* MeshA,
                 Render::mesh* MeshB, mat4 ModelAMatrix, mat4 ModelBMatrix)
{
  contact_point Simplex[4];

  bool IsColliding = GJK(Simplex, MeshA, MeshB, ModelAMatrix, ModelBMatrix);

  if(IsColliding)
  {
    printf("Collision detected!\n");

    vec3 SolutionVector;
    vec3 CollisionPoint;
    EPA(GameState, Input, &SolutionVector, &CollisionPoint, Simplex, MeshA, MeshB, ModelAMatrix,
        ModelBMatrix);

    printf("SolutionVector = { %f, %f, %f }\n", SolutionVector.X, SolutionVector.Y,
           SolutionVector.Z);
    printf("CollisionPoint = { %f, %f, %f }\n", CollisionPoint.X, CollisionPoint.Y,
           CollisionPoint.Z);
  }
  else
  {
    printf("No collision detected.\n");
  }
}
#endif

bool
AreColliding(game_state* GameState, const game_input* const Input, Render::mesh* MeshA,
             Render::mesh* MeshB, mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount)
{
  int32_t       SimplexOrder;
  vec3          Direction;
  contact_point Simplex[4];
  int           IterationsToFindSimplex;

  bool CollisionFound = GJK(Simplex, &SimplexOrder, MeshA, MeshB, ModelAMatrix, ModelBMatrix,
                            IterationCount, &IterationsToFindSimplex, &Direction);

  vec3 SimplexVertices[4];
  for(int i = 0; i <= SimplexOrder; i++)
  {
    SimplexVertices[i] = Simplex[i].P;
  }

  if(CollisionFound)
  {
    vec3 CollisionPoint;
    vec3 SolutionVector = EPA(&CollisionPoint, Simplex, MeshA, MeshB, ModelAMatrix, ModelBMatrix,
                              IterationCount - IterationsToFindSimplex);

    if(SolutionVector != vec3{})
    {
      printf("SolutionVector = { %f, %f, %f }\n", SolutionVector.X, SolutionVector.Y,
             SolutionVector.Z);
      printf("CollisionPoint = { %f, %f, %f }\n", CollisionPoint.X, CollisionPoint.Y,
             CollisionPoint.Z);
    }
  }
  else
  {
    for(int i = 0; i <= SimplexOrder; i++)
    {
      Debug::PushWireframeSphere(SimplexVertices[i], 0.05f, { 1, 0, 1, 1 });
      for(int j = 1; j <= SimplexOrder; j++)
      {
        Debug::PushLine(SimplexVertices[i], SimplexVertices[j]);
      }
    }
    Debug::PushLine({ 0.0f, 0.0f, 0.0f }, Math::Normalized(Direction), { 0.0f, 0.0f, 1.0f, 1.0f });
  }
  return CollisionFound;
}