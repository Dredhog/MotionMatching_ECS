#pragma once

#include "collision.h"

void
CollisionTesting()
{
  vec3 Cube[8];
  vec3 Pyramid[5];

  Cube[0] = { 1.0f, 1.0f, 1.0f };
  Cube[1] = { 1.0f, 1.0f, -1.0f };
  Cube[2] = { 1.0f, -1.0f, -1.0f };
  Cube[3] = { 1.0f, -1.0f, 1.0f };
  Cube[4] = { -1.0f, -1.0f, 1.0f };
  Cube[5] = { -1.0f, -1.0f, -1.0f };
  Cube[6] = { -1.0f, 1.0f, -1.0f };
  Cube[7] = { -1.0f, 1.0f, 1.0f };

  Pyramid[0] = { 1.5f, -1.0f, 0.0f };
  Pyramid[1] = { -1.0f, 1.75f, 0.75f };
  Pyramid[2] = { 2.5f, 1.0f, 0.0f };
  Pyramid[3] = { 1.5f, -0.5f, 0.0f };
  Pyramid[4] = { 4.0f, 1.0f, 3.0f };

  vec3    Simplex[50];
  int32_t SimplexOrder;

  bool IsColliding = GJK(Simplex, &SimplexOrder, Cube, 8, Pyramid, 5);

  if(IsColliding)
  {
    printf("Collision detected!\n");

    vec3 SolutionVector = EPA(Simplex, 4, Cube, 8, Pyramid, 5);

    printf("SolutionVector = { %f, %f, %f }\n", SolutionVector.X, SolutionVector.Y, SolutionVector.Z);
  }
  else
  {
    printf("No collision detected.\n");
  }
}
