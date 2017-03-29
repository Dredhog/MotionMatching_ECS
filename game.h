#pragma once

#include <stdint.h>

#include "model.h"
#include "anim.h"
#include "skeleton.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "stack_allocator.h"
#include "edit_animation.h"

struct eul_transform
{
  vec3 Rotation;
  vec3 Translation;
  vec3 Scale;
};

struct entity
{
  Render::model*              Model;
  eul_transform               Transform;
  Anim::animation_controller* AnimController;
};

struct camera
{
  vec3 P;
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

struct game_state
{
  Memory::stack_allocator* PersistentMemStack;
  Memory::stack_allocator* TemporaryMemStack;

  EditAnimation::animation_editor AnimEditor;

  Anim::skeleton* Skeleton;
  Render::model*  QuadModel;
  Render::model*  PlaybackCursorModel;
  Render::model*  CharacterModel;
  Render::model*  GizmoModel;

  GLint   Texture;
  int32_t ShaderBoneColor;
  int32_t ShaderWireframe;
  int32_t ShaderDiffuse;
  int32_t ShaderGizmo;
  int32_t ShaderQuad;
  int32_t ShaderTexturedQuad;

    vec3 LightPosition;
  vec3   LightColor;
  float  AmbientStrength;
  float  SpecularStrength;

  uint32_t      MagicChecksum;
  eul_transform ModelTransform;

  bool DrawWireframe;
  bool DrawBoneWeights;
  bool DrawGizmos;

  camera Camera;
  float  GameTime;
};
