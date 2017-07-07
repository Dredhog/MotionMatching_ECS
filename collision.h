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

bool GJK(contact_point* Simplex, Render::mesh* MeshA, Render::mesh* MeshB, mat4 ModelAMatrix, mat4 ModelBMatrix);

void EPA(game_state* GameState, const game_input* const Input, vec3* SolutionVector, vec3* CollisionPoint, contact_point* Simplex, Render::mesh* MeshA, Render::mesh* MeshB, mat4 ModelAMatrix,
         mat4 ModelBMatrix);
