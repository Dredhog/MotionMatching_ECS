#pragma once

#include <stdint.h>

#include "model.h"
#include "anim.h"
#include "skeleton.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "stack_allocator.h"
#include "edit_animation.h"
#include "camera.h"
#include "render_data.h"

const int32_t        ENTITY_MAX_COUNT      = 400;
static const int32_t ENTITY_MAX_MESH_COUNT = 100;

struct entity
{
  Anim::transform Transform;
  Render::model*  Model;
  int32_t*        MaterialIndices;
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
  MODE_MaterialEditor,
  MODE_Gameplay,
  MODE_FlyCam,
};

struct game_state
{
  Memory::stack_allocator* PersistentMemStack;
  Memory::stack_allocator* TemporaryMemStack;

  render_data                     R;
  EditAnimation::animation_editor AnimEditor;
  int32_t                         CurrentModel;
  int32_t                         CurrentMaterial;

  camera Camera;
  camera PreviewCamera;

  // Models
  Render::model* SphereModel;
  Render::model* UVSphereModel;
  Render::model* QuadModel;
  Render::model* CharacterModel;
  Render::model* GizmoModel;
  Render::model* CubemapModel;

  // Temp textures (not their place)
  int32_t CollapsedTexture;
  int32_t ExpandedTexture;
  int32_t TextTexture;
  int32_t CubemapTexture;

  // Entities
  entity  Entities[ENTITY_MAX_COUNT];
  int32_t EntityCount;
  int32_t SelectedEntityIndex;
  int32_t SelectedMeshIndex;

  // Switches/Flags
  bool  DrawWireframe;
  bool  DrawCubemap;
  bool  DrawBoneWeights;
  bool  DrawTimeline;
  bool  DrawGizmos;
  bool  DrawText;
  bool  IsModelSpinning;
  bool  IsAnimationPlaying;
  float EditorBoneRotationSpeed;
  bool  IsEntityCreationMode;

  // Audio
  loaded_wav AudioBuffer;
  bool       WAVLoaded;

  uint32_t MagicChecksum;
  uint32_t EngineMode;

  // ID buffer (selection)
  uint32_t IndexFBO;
  uint32_t DepthRBO;
  uint32_t IDTexture;
};
