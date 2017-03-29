#pragma once

#include <stdint.h>

#include "model.h"
#include "anim.h"
#include "skeleton.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "stack_allocator.h"
#include "edit_animation.h"

struct entity
{
  Render::model*              Model;
  Anim::transform             Transform;
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
  Anim::skeleton*              Skeleton;
  Render::model*               CharacterModel;
  Render::model*               GizmoModel;

  int ShaderBoneColor;
  int ShaderWireframe;
  int ShaderDiffuse;
  int ShaderTexture;
  int ShaderGizmo;

  uint32_t MagicChecksum;
  vec3     MeshEulerAngles;
  vec3     MeshScale;

  bool DrawWireframe;
  bool DrawBoneWeights;
  bool DrawGizmos;

  camera Camera;
  float  GameTime;
};
