#pragma once

#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "linear_math/quaternion.h"

struct state
{
  // Yi(t)
  vec3 X;
  quat q;

  vec3 P;
  vec3 L;
};

struct state_derivative
{
  // dYi(t)
  vec3 v;
  quat qDot;
  vec3 Force;
  vec3 Torque;
};

struct rigid_body
{
  state         State;
  Render::mesh* Collider;

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
