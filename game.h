#pragma once

#include <SDL2/SDL_ttf.h>
#include <stdint.h>

#include "model.h"
#include "anim.h"
#include "skeleton.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "linear_math/quaternion.h"
#include "stack_alloc.h"
#include "heap_alloc.h"
#include "edit_animation.h"
#include "camera.h"
#include "render_data.h"
#include "text.h"
#include "entity.h"
#include "edit_animation.h"
#include "resource_manager.h"
#include "dynamics.h"
#include "motion_matching.h"
#include "ecs_management.h"

const int32_t ENTITY_MAX_COUNT           = 400;
const int32_t ENTITY_SELECTION_MAX_COUNT = 400;
const int32_t MESH_SELECTION_MAX_COUNT   = 400;
const int32_t MAX_MM_ANIMATION_COUNT     = 30;

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
  rid                             CurrentMaterialID;
  rid                             CurrentAnimationID;

  Resource::resource_manager Resources;
  physics_world              Physics;

  ecs_runtime* ECSRuntime;
  ecs_world*   ECSWorld;

  camera Camera;
  camera PreviewCamera;

  // Motion Matching
  mm_matching_params MMParams;
  mm_controller_data MMData;
  mm_debug_settings  MMDebug;
  float   PlayerSpeed;

  bool UseHotReloading;
  bool UpdatePathList;

  // Models
  rid SphereModelID;
  rid LowPolySphereModelID;
  rid UVSphereModelID;
  rid QuadModelID;
  rid GizmoModelID;
  rid BoneDiamondModelID;
  rid CubemapModelID;

  // Temp textures (not their place)
  int32_t CollapsedTextureID;
  int32_t ExpandedTextureID;

  // Entities
  entity  Entities[ENTITY_MAX_COUNT];
  int32_t EntityCount;
  int32_t SelectedEntityIndex;
  int32_t SelectedMeshIndex;
  int32_t PlayerEntityIndex;
  mat4    PrevFrameMVPMatrices[ENTITY_MAX_COUNT];

  // Fonts/text
  Text::font Font;

  // Switches/Flags
  bool  DrawCubemap;
  bool  DrawGizmos;
  bool  DrawDebugLines;
  bool  DrawDebugSpheres;
  bool  DrawShadowCascadeVolumes;
  bool  DrawTimeline;
	bool  DrawActorMeshes;
  bool  IsAnimationPlaying;
  bool  IsEntityCreationMode;
  float BoneSphereRadius;

  uint32_t MagicChecksum;
  uint32_t SelectionMode;

  // ID buffer (selection)
  uint32_t IndexFBO;
  uint32_t DepthRBO;
  uint32_t IDTexture;
};

void GetCubemapRIDs(rid* RIDs, Resource::resource_manager* Resources,
                    Memory::stack_allocator* const Allocator, char* CubemapPath, char* FileFormat);
void RegisterDebugModels(game_state* GameState);
uint32_t LoadCubemap(Resource::resource_manager* Resources, rid* RIDs);
void GenerateScreenQuad(uint32_t* VAO, uint32_t* VBO);
void GenerateFramebuffer(uint32_t* FBO, uint32_t* RBO, uint32_t* Texture, int Width = SCREEN_WIDTH,
                         int Height = SCREEN_HEIGHT);
void GenerateSSAOFrameBuffer(uint32_t* FBO, uint32_t* SSAOTextureID, int Width = SCREEN_WIDTH,
                             int Height = SCREEN_HEIGHT);
void GenerateFloatingPointFBO(uint32_t* FBO, uint32_t* ColorTextureID, uint32_t* DepthStencilRBO,
                              int Width = SCREEN_WIDTH, int Height = SCREEN_HEIGHT);
void GenerateGeometryDepthFrameBuffer(uint32_t* FBO, uint32_t* ViewSpacePositionTextureID,
                                      uint32_t* VelocityTextureID, uint32_t* NormalTextureID,
                                      uint32_t* DepthTextureID);
void GenerateDepthFramebuffer(uint32_t* FBO, uint32_t* Texture);
void GenerateShadowFramebuffer(uint32_t* FBO, uint32_t* Texture);
void BindNextFramebuffer(uint32_t* FBOs, uint32_t* CurrentFramebuffer);
void BindTextureAndSetNext(uint32_t* Textures, uint32_t* CurrentTexture);
void DrawTextureToFramebuffer(uint32_t VAO);
void     DrawSkeleton(const Anim::skeleton* Skeleton, const mat4* HierarchicalModelSpaceMatrices,
                      mat4 MatModel, float JointSphereRadius, bool UseBoneDiamonds = true);

//-----------------------ENTITY RELATED UTILITY FUNCTIONS---------------------------

void AddEntity(game_state* GameState, rid ModelID, rid* MaterialIDs, Anim::transform Transform);
bool DeleteEntity(game_state* GameState, int32_t Index);
void RemoveAnimationReferences(Resource::resource_manager* Resources,
                               Anim::animation_controller* Controller);
bool GetEntityAtIndex(game_state* GameState, entity** OutputEntity, int32_t EntityIndex);
void AttachEntityToAnimEditor(game_state* GameState, EditAnimation::animation_editor* Editor,
                              int32_t EntityIndex);
void DettachEntityFromAnimEditor(const game_state*                GameState,
                                 EditAnimation::animation_editor* Editor);
bool GetSelectedEntity(game_state* GameState, entity** OutputEntity);
bool GetSelectedMesh(game_state* GameState, Render::mesh** OutputMesh);
mat4 GetEntityModelMatrix(game_state* GameState, int32_t EntityIndex);
mat4 GetEntityMVPMatrix(game_state* GameState, int32_t EntityIndex);
