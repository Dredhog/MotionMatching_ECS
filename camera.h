#pragma once

#include "misc.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "common.h"

struct camera
{
  vec3 Position;
  vec3 Right;
  vec3 Forward;

  float NearClipPlane;
  float FarClipPlane;
  float FieldOfView;

  float Speed;
  float MaxTiltAngle;

  bool  OrbitSelected;
  float OrbitRadius;
  vec2  OrbitRotation;

  vec3 Rotation;
  mat4 ViewMatrix;
  mat4 ProjectionMatrix;
  mat4 VPMatrix;
  mat4 InvVPMatrix;
};

void UpdateCamera(camera* Camera, vec3 FollowPoint, const game_input* Input);
void UpdateCamera(camera* Camera, const game_input* Input);
void UpdateCameraDerivedFields(camera* Camera);
