#pragma once

#include "linear_math/vector.h"
#include "linear_math/quaternion.h"

struct transform
{
  quat R;
  vec3 T;
  vec3 S;
};

inline mat4
TransformToMat4(const transform& Transform)
{
  mat4 Result =
    Math::MulMat4(Math::Mat4Translate(Transform.T),
                  Math::MulMat4(Math::Mat4Rotate(Transform.R), Math::Mat4Scale(Transform.S)));
  return Result;
}

inline transform IdentityTransform()
{
  return { .R = Math::QuatIdent(), .T = {}, .S = { 1, 1, 1 } };
}
