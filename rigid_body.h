#pragma once

#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "linear_math/quaternion.h"

enum constraint_type
{
  CONSTRAINT_Distance,
  CONSTRAINT_Point,
  CONSTRAINT_Contact,
  CONSTRAINT_Friction,
  CONSTRAINT_Count,
};

struct constraint
{
  uint32_t Type;
  int32_t  IndA;
  int32_t  IndB;

  float L;
  vec3  BodyRa;
  vec3  BodyRb;
  float Penetration;
  vec3  n;
  vec3  P;
  vec3  Tangent;
  // For friction
  int32_t ContactIndex;
};

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
