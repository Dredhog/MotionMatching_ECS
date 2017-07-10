#pragma once

#include <stdint.h>
#include "linear_math/vector.h"
#include "game.h"
#include "debug_drawing.h"
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

vec3 Support(Render::mesh* Mesh, vec3 Direction, mat4 ModelMatrix);
bool GJK(contact_point* Simplex, int32_t* SimplexOrder, Render::mesh* MeshA, Render::mesh* MeshB, mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount, int32_t* FoundInIterations, vec3* Direction);
vec3 EPA(vec3* CollisionPoint, contact_point* Simplex, Render::mesh* MeshA, Render::mesh* MeshB, mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount);
