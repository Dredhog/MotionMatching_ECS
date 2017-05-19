#pragma once

#include <SDL2/SDL_ttf.h>
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
#include "text.h"
#include "entity.h"
#include "edit_animation.h"
#include "resource_manager.h"

struct loaded_wav
{
  int16_t* AudioData;
  uint32_t AudioLength;
  uint32_t AudioSampleIndex;
};

const int32_t ENTITY_MAX_COUNT           = 400;
const int32_t ENTITY_SELECTION_MAX_COUNT = 400;
const int32_t MESH_SELECTION_MAX_COUNT   = 400;

#define FOR_ALL_NAMES(DO_FUNC) DO_FUNC(Entity) DO_FUNC(Mesh) DO_FUNC(Bone)
#define GENERATE_ENUM(Name) SELECT_##Name,
#define GENERATE_STRING(Name) #Name,
enum selection_mode
{
  FOR_ALL_NAMES(GENERATE_ENUM) SELECT_EnumCount
};

static const char* g_SelectionEnumStrings[SELECT_EnumCount] = { FOR_ALL_NAMES(GENERATE_STRING) };
#undef FOR_ALL_NAMES
#undef GENERATE_ENUM
#undef GENERATE_STRING

struct game_state
{

  Memory::stack_allocator* PersistentMemStack;
  Memory::stack_allocator* TemporaryMemStack;

  render_data                     R;
  EditAnimation::animation_editor AnimEditor;
  rid                             CurrentModelID;
  int32_t                         CurrentMaterialIndex;

  Resource::resource_manager Resources;

  // material EditableMaterial;

  camera Camera;
  camera PreviewCamera;

  // Models
  rid SphereModelID;
  rid UVSphereModelID;
  rid QuadModelID;
  rid GizmoModelID;
  rid CubemapModelID;

  // Temp textures (not their place)
  int32_t CollapsedTextureID;
  int32_t ExpandedTextureID;
  int32_t CubemapTexture;

  // Entities
  entity  Entities[ENTITY_MAX_COUNT];
  int32_t EntityCount;
  int32_t SelectedEntityIndex;
  int32_t SelectedMeshIndex;

  // Animation Test
  rid TestAnimationID;

  // Fonts/text
  Text::font Font;

  // Switches/Flags
  bool  DrawCubemap;
  bool  DrawGizmos;
  bool  DrawDebugSpheres;
  bool  DrawTimeline;
  bool  IsAnimationPlaying;
  float EditorBoneRotationSpeed;
  bool  IsEntityCreationMode;

  // Audio
  loaded_wav AudioBuffer;
  bool       WAVLoaded;

  uint32_t MagicChecksum;
  uint32_t SelectionMode;

  // ID buffer (selection)
  uint32_t IndexFBO;
  uint32_t DepthRBO;
  uint32_t IDTexture;
};

inline bool
GetEntityAtIndex(game_state* GameState, entity** OutputEntity, int32_t EntityIndex)
{
  if(GameState->EntityCount > 0)
  {
    if(0 <= EntityIndex && EntityIndex < GameState->EntityCount)
    {
      *OutputEntity = &GameState->Entities[EntityIndex];
      return true;
    }
    return false;
  }
  return false;
}

inline bool
GetSelectedEntity(game_state* GameState, entity** OutputEntity)
{
  return GetEntityAtIndex(GameState, OutputEntity, GameState->SelectedEntityIndex);
}

inline bool
GetSelectedMesh(game_state* GameState, Render::mesh** OutputMesh)
{
  entity* Entity = NULL;
  if(GetSelectedEntity(GameState, &Entity))
  {
    Render::model* Model = GameState->Resources.GetModel(Entity->ModelID);
    if(Model->MeshCount > 0)
    {
      if(0 <= GameState->SelectedMeshIndex && GameState->SelectedMeshIndex < Model->MeshCount)
      {
        *OutputMesh = Model->Meshes[GameState->SelectedMeshIndex];
        return true;
      }
    }
  }
  return false;
}

inline mat4
TransformToMat4(const Anim::transform* Transform)
{
  mat4 Result = Math::MulMat4(Math::Mat4Translate(Transform->Translation),
                              Math::MulMat4(Math::Mat4Rotate(Transform->Rotation),
                                            Math::Mat4Scale(Transform->Scale)));
  return Result;
}

inline bool
DeleteEntity(game_state* GameState, int32_t Index)
{
  if(0 <= Index && GameState->EntityCount)
  {
    GameState->Entities[Index] = GameState->Entities[GameState->EntityCount - 1];
    --GameState->EntityCount;
    return true;
  }
  return false;
}

inline void
AttachEntityToAnimEditor(game_state* GameState, EditAnimation::animation_editor* Editor,
                         int32_t EntityIndex)
{
  entity* AddedEntity = {};
  if(GetEntityAtIndex(GameState, &AddedEntity, EntityIndex))
  {
    Render::model* Model = GameState->Resources.GetModel(AddedEntity->ModelID);
    assert(Model->Skeleton);
    Editor->Skeleton    = Model->Skeleton;
    Editor->Transform   = &AddedEntity->Transform;
    Editor->EntityIndex = EntityIndex;
  }
}

inline void
DettachEntityFromAnimEditor(const game_state* GameState, EditAnimation::animation_editor* Editor)
{
  assert(GameState->Entities[Editor->EntityIndex].AnimController);

  assert(Editor->Skeleton);
  Editor->Skeleton    = 0;
  Editor->Transform   = 0;
  Editor->EntityIndex = 0;
}
