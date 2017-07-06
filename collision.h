#pragma once

#include <stdint.h>
#include "linear_math/vector.h"
#include "mesh.h"

bool GJK(vec3* Simplex, Render::mesh* MeshA, Render::mesh* MeshB);

vec3 EPA(vec3* Simplex, int32_t SimplexLength, Render::mesh* MeshA, Render::mesh* MeshB);
