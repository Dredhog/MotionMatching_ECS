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
AreColliding(vec3* SimplexVertices, int32_t* SimplexOrder, vec3* Direction, game_state* GameState,
             const game_input* const Input, Render::mesh* MeshA, Render::mesh* MeshB,
             mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount)
{
  contact_point Simplex[4];
  bool          CollisionFound =
    GJK(Simplex, SimplexOrder, MeshA, MeshB, ModelAMatrix, ModelBMatrix, IterationCount, Direction);
  for(int i = 0; i <= *SimplexOrder; i++)
  {
    SimplexVertices[i] = Simplex[i].P;
  }
  return CollisionFound;
}
