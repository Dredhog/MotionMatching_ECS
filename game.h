#pragma once

#include <stdint.h>

#include "model.h"
#include "anim.h"
#include "skeleton.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "stack_allocator.h"
#include "edit_animation.h"

static const int32_t TEXTURE_MAX_COUNT = 20;

struct entity
{
  Render::model*              Model;
  Anim::transform             Transform;
  Anim::animation_controller* AnimController;
};

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

struct loaded_wav
{
  int16_t* AudioData;
  uint32_t AudioLength;
  uint32_t AudioSampleIndex;
};

enum engine_mode
{
  MODE_AnimationEditor,
  MODE_EntityCreation,
  MODE_EditorMenu,
  MODE_MainMenu,
  MODE_Gameplay,
  MODE_FlyCam,
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

  int32_t Textures[TEXTURE_MAX_COUNT];
  int32_t TextureCount;
  int32_t CollapsedTexture;
  int32_t ExpandedTexture;

  int32_t Shaders;
  int32_t ShaderSkeletalBoneColor;
  int32_t ShaderWireframe;
  int32_t ShaderSkeletalPhong;
  int32_t ShaderGizmo;
  int32_t ShaderQuad;
  int32_t ShaderTexturedQuad;

  vec3  LightPosition;
  vec3  LightColor;
  float AmbientStrength;
  float SpecularStrength;

  uint32_t        MagicChecksum;
  Anim::transform ModelTransform;

  bool DrawWireframe;
  bool DrawBoneWeights;
  bool DrawGizmos;
  bool DisplayText;
  bool IsModelSpinning;

  camera   Camera;
  uint32_t EngineMode;
  float    GameTime;

  loaded_wav AudioBuffer;
  bool       WAVLoaded;

  uint32_t TextTexture;
};
