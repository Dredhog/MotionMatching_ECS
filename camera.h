#pragma once

#include "misc.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "common.h"

struct camera
{
  vec3 Position;
  vec3 Up;
  vec3 Right;
  vec3 Forward;

  float NearClipPlane;
  float FarClipPlane;
  float FieldOfView;

  float Speed;
  float MaxTiltAngle;

  vec3 Rotation;
  mat4 ViewMatrix;
  mat4 ProjectionMatrix;
  mat4 VPMatrix;
};

void UpdateCamera(camera* Camera, const game_input* Input);
