#pragma once

#include "collision.h"

void
CollisionTesting(game_state* GameState, const game_input* const Input, Render::mesh* MeshA, Render::mesh* MeshB, mat4 ModelAMatrix, mat4 ModelBMatrix)
{
  contact_point Simplex[4];

  bool IsColliding = GJK(Simplex, MeshA, MeshB, ModelAMatrix, ModelBMatrix);

  if(IsColliding)
  {
    printf("Collision detected!\n");

    vec3 SolutionVector;
    vec3 CollisionPoint;
    EPA(GameState, Input, &SolutionVector, &CollisionPoint, Simplex, MeshA, MeshB, ModelAMatrix, ModelBMatrix);

    printf("SolutionVector = { %f, %f, %f }\n", SolutionVector.X, SolutionVector.Y, SolutionVector.Z);
    printf("CollisionPoint = { %f, %f, %f }\n", CollisionPoint.X, CollisionPoint.Y, CollisionPoint.Z);
  }
  else
  {
    printf("No collision detected.\n");
  }
}
