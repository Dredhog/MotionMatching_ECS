#pragma once

#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "linear_math/quaternion.h"
#include "mesh.h"
#include <stdint.h>

struct rigid_body
{
  Render::mesh* Collider;

  // Yi(t)
  vec3 X;
  quat q;

  vec3 v;
  vec3 w;

  // Constant state
  float Mass;
  float MassInv;
  mat3  InertiaBody;
  mat3  InertiaBodyInv;

  // Compute on iteration
  mat3 InertiaInv;
  mat3 R;
  mat4 Mat4Scale;
  bool RegardGravity;
};
