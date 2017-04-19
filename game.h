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

struct loaded_wav
{
  int16_t* AudioData;
  uint32_t AudioLength;
  uint32_t AudioSampleIndex;
};

const int32_t ENTITY_MAX_COUNT           = 400;
const int32_t ENTITY_SELECTION_MAX_COUNT = 400;
const int32_t MESH_SELECTION_MAX_COUNT   = 400;

struct entity
{
  Anim::transform Transform;
  Render::model*  Model;
  int32_t*        MaterialIndices;
};

#if 0
struct entiity_mesh
{
  int32_t EntityIndex;
  int32_t MeshIndex;
};

struct Selection
{
  // ArraysAreMutuallyExclussive
  int32_t      EntityIndices[ENTITY_SELECTION_MAX_COUNT];
  entiity_mesh Meshes[MESH_SELECTION_MAX_COUNT];
};
#endif

struct game_state
{
  Memory::stack_allocator* PersistentMemStack;
  Memory::stack_allocator* TemporaryMemStack;

  render_data                     R;
  EditAnimation::animation_editor AnimEditor;
  int32_t                         CurrentModel;
  int32_t                         CurrentMaterial;

  // material EditableMaterial;

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
  int32_t CollapsedTextureID;
  int32_t ExpandedTextureID;
  int32_t CubemapTexture;

  // Entities
  entity  Entities[ENTITY_MAX_COUNT];
  int32_t EntityCount;
  int32_t SelectedEntityIndex;
  int32_t SelectedMeshIndex;

  // Fonts/text
  Text::font Font;

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

  // ID buffer (selection)
  uint32_t IndexFBO;
  uint32_t DepthRBO;
  uint32_t IDTexture;
};

inline bool
GetSelectedEntity(game_state* GameState, entity** OutputEntity)
{
  // GameState->SelectedEntityIndex = 0;
  if(GameState->EntityCount > 0)
  {
    if(0 <= GameState->SelectedEntityIndex &&
       GameState->SelectedEntityIndex < GameState->EntityCount)
    {
      *OutputEntity = &GameState->Entities[GameState->SelectedEntityIndex];
      return true;
    }
    return false;
  }
  return false;
}

inline bool
GetSelectedMesh(game_state* GameState, Render::mesh** OutputMesh)
{
  // GameState->SelectedMeshIndex = 3;
  entity* Entity = NULL;
  if(GetSelectedEntity(GameState, &Entity))
  {

    if(Entity->Model->MeshCount > 0)
    {
      if(0 <= GameState->SelectedMeshIndex &&
         GameState->SelectedMeshIndex < Entity->Model->MeshCount)
      {
        *OutputMesh = GameState->Entities[GameState->SelectedEntityIndex]
                        .Model->Meshes[GameState->SelectedMeshIndex];
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
