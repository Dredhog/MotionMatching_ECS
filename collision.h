#pragma once

#include <stdint.h>
#include "linear_math/vector.h"
#include "mesh.h"

struct collider
{
  vec3    Vertices;
  int32_t VertexCount;
};

struct contact_point
{
  vec3 P;
  vec3 SupportA;
};

bool GJK(contact_point* Simplex, Render::mesh* MeshA, Render::mesh* MeshB);

void EPA(vec3* SolutionVector, vec3* CollisionPoint, contact_point* Simplex, Render::mesh* MeshA, Render::mesh* MeshB);
