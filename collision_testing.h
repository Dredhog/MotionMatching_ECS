#pragma once

#include "collision.h"

bool
TestHullvsHull(sat_contact_manifold* Manifold, Render::mesh* MeshA, Render::mesh* MeshB,
               mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount = 50)
{
  int32_t SimplexOrder;
  vec3    Direction;
  vec3    Simplex[4];
  int     IterationsToFindSimplex;

  bool CollisionFound = GJK(Simplex, &SimplexOrder, MeshA, MeshB, ModelAMatrix, ModelBMatrix,
                            IterationCount, &IterationsToFindSimplex, &Direction);

  if(CollisionFound)
  {
    vec3 CollisionPoint;
    vec3 PenetrationVector = EPA(&CollisionPoint, Simplex, MeshA, MeshB, ModelAMatrix, ModelBMatrix,
                                 IterationCount - IterationsToFindSimplex);
    Manifold->Points[0].Position    = CollisionPoint;
    Manifold->Points[0].Penetration = Math::Length(PenetrationVector);
    Manifold->PointCount            = 1;
    Manifold->Normal                = Math::Normalized(PenetrationVector);
  }
  else
  {
    Manifold->PointCount = 0;
    for(int i = 0; i <= SimplexOrder; i++)
    {
      Debug::PushWireframeSphere(Simplex[i], 0.05f, { 1, 0, 1, 1 });
      for(int j = 1; j <= SimplexOrder; j++)
      {
        Debug::PushLine(Simplex[i], Simplex[j]);
      }
    }
    Debug::PushLine({ 0.0f, 0.0f, 0.0f }, Math::Normalized(Direction), { 0.0f, 0.0f, 1.0f, 1.0f });
  }
  return CollisionFound;
}
