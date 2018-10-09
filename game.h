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

#define FRAMEBUFFER_MAX_COUNT 5

struct cubemap
{
  char     Name[TEXT_LINE_MAX_LENGTH];
  char     Format[TEXT_LINE_MAX_LENGTH];
  rid      FaceIDs[6];
  uint32_t CubemapTexture;
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
  Memory::heap_allocator   HeapAllocator;

  render_data                     R;
  EditAnimation::animation_editor AnimEditor;
  rid                             CurrentModelID;
  rid                             CurrentMaterialID;
  rid                             CurrentAnimationID;

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
  cubemap Cubemap;

  // Entities
  entity  Entities[ENTITY_MAX_COUNT];
  int32_t EntityCount;
  int32_t SelectedEntityIndex;
  int32_t SelectedMeshIndex;
  int32_t PlayerEntityIndex;

  // Fonts/text
  Text::font Font;

  // Temporary stuff for post-processing
  uint32_t ScreenQuadVAO;
  uint32_t ScreenQuadVBO;
  uint32_t ScreenFBO[FRAMEBUFFER_MAX_COUNT];
  uint32_t ScreenRBO[FRAMEBUFFER_MAX_COUNT];
  uint32_t ScreenTexture[FRAMEBUFFER_MAX_COUNT];
  uint32_t CurrentFramebuffer;
  uint32_t CurrentTexture;

  // Switches/Flags
  bool  DrawCubemap;
  bool  DrawGizmos;
  bool  DrawDebugSpheres;
  bool  DrawTimeline;
  bool  IsAnimationPlaying;
  float EditorBoneRotationSpeed;
  bool  IsEntityCreationMode;

  uint32_t MagicChecksum;
  uint32_t SelectionMode;

  // ID buffer (selection)
  uint32_t IndexFBO;
  uint32_t DepthRBO;
  uint32_t IDTexture;

  // Physics
  bool    SimulateDynamics;
  bool    PerformDynamicsStep;
  bool    SimulateFriction;
  int32_t PGSIterationCount;
  vec3    ForceStart;
  vec3    Force;
  float   Restitution;
  float   Beta;
  float   Slop;
  float   Mu;

  bool ApplyingForce;
  bool ApplyingTorque;
  bool UseGravity;

  bool VisualizeOmega;
  bool VisualizeV;
  bool VisualizeFriction;
  bool VisualizeFc;
  bool VisualizeFcComponents;

  bool VisualizeContactPoints;
  bool VisualizeContactManifold;
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
    GameState->Resources.Models.RemoveReference(GameState->Entities[Index].ModelID);
    GameState->Entities[Index] = GameState->Entities[GameState->EntityCount - 1];
    --GameState->EntityCount;
    if(GameState->PlayerEntityIndex == Index)
    {
      GameState->PlayerEntityIndex = -1;
    }
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
    *Editor             = {};
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
  *Editor = {};
}

inline void
GetCubemapRIDs(rid* RIDs, Resource::resource_manager* Resources,
               Memory::stack_allocator* const Allocator, char* CubemapPath, char* FileFormat)
{
  char* CubemapFaces[6];
  CubemapFaces[0] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_right.") + 1));
  CubemapFaces[1] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_left.") + 1));
  CubemapFaces[2] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_top.") + 1));
  CubemapFaces[3] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_bottom.") + 1));
  CubemapFaces[4] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_back.") + 1));
  CubemapFaces[5] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_front.") + 1));
  strcpy(CubemapFaces[0], "_right.\0");
  strcpy(CubemapFaces[1], "_left.\0");
  strcpy(CubemapFaces[2], "_top.\0");
  strcpy(CubemapFaces[3], "_bottom.\0");
  strcpy(CubemapFaces[4], "_back.\0");
  strcpy(CubemapFaces[5], "_front.\0");

  char* FileNames[6];
  FileNames[0] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[0]) + strlen(FileFormat) + 1)));
  FileNames[1] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[1]) + strlen(FileFormat) + 1)));
  FileNames[2] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[2]) + strlen(FileFormat) + 1)));
  FileNames[3] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[3]) + strlen(FileFormat) + 1)));
  FileNames[4] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[4]) + strlen(FileFormat) + 1)));
  FileNames[5] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[5]) + strlen(FileFormat) + 1)));

  for(int i = 0; i < 6; i++)
  {
    strcpy(FileNames[i], CubemapPath);
    strcat(FileNames[i], CubemapFaces[i]);
    strcat(FileNames[i], FileFormat);
    strcat(FileNames[i], "\0");
    if(!Resources->GetTexturePathRID(&RIDs[i], FileNames[i]))
    {
      RIDs[i] = Resources->RegisterTexture(FileNames[i]);
    }
  }
}

inline void
RegisterDebugModels(game_state* GameState)
{
  GameState->GizmoModelID    = GameState->Resources.RegisterModel("data/built/gizmo1.model");
  GameState->QuadModelID     = GameState->Resources.RegisterModel("data/built/debug_meshes.model");
  GameState->CubemapModelID  = GameState->Resources.RegisterModel("data/built/inverse_cube.model");
  GameState->SphereModelID   = GameState->Resources.RegisterModel("data/built/sphere.model");
  GameState->UVSphereModelID = GameState->Resources.RegisterModel("data/built/uv_sphere.model");
  GameState->Resources.Models.AddReference(GameState->GizmoModelID);
  GameState->Resources.Models.AddReference(GameState->QuadModelID);
  GameState->Resources.Models.AddReference(GameState->CubemapModelID);
  GameState->Resources.Models.AddReference(GameState->SphereModelID);
  GameState->Resources.Models.AddReference(GameState->UVSphereModelID);

  strcpy(GameState->Cubemap.Name, "data/textures/skybox/morning");
  strcpy(GameState->Cubemap.Format, "tga");
  GetCubemapRIDs(GameState->Cubemap.FaceIDs, &GameState->Resources, GameState->TemporaryMemStack,
                 GameState->Cubemap.Name, GameState->Cubemap.Format);
  GameState->Cubemap.CubemapTexture = -1;
  for(int i = 0; i < 6; i++)
  {
    GameState->Resources.Textures.AddReference(GameState->Cubemap.FaceIDs[i]);
  }
}

inline uint32_t
LoadCubemap(Resource::resource_manager* Resources, rid* RIDs)
{
  uint32_t Texture;
  glGenTextures(1, &Texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, Texture);

  for(int i = 0; i < 6; i++)
  {
    SDL_Surface* ImageSurface =
      IMG_Load(Resources->TexturePaths[Resources->GetTexturePathIndex(RIDs[i])].Name);
    if(ImageSurface)
    {
      SDL_Surface* DestSurface =
        SDL_ConvertSurfaceFormat(ImageSurface, SDL_PIXELFORMAT_ABGR8888, 0);
      SDL_FreeSurface(ImageSurface);

      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, DestSurface->w, DestSurface->h,
                   0, GL_RGBA, GL_UNSIGNED_BYTE, DestSurface->pixels);
      SDL_FreeSurface(DestSurface);
    }
    else
    {
      printf("Platform: texture load from image error: %s\n", SDL_GetError());
      glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
      return 0;
    }
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  return Texture;
}

inline void
GenerateScreenQuad(uint32_t* VAO, uint32_t* VBO)
{
  // Create VAO and VBO for screen quad
  float QuadVertices[] = { -1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                           1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 1.0f,
                           1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f };

  glGenVertexArrays(1, VAO);
  glGenBuffers(1, VBO);
  glBindVertexArray(*VAO);
  glBindBuffer(GL_ARRAY_BUFFER, *VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), &QuadVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

inline void
GenerateFramebuffer(uint32_t* FBO, uint32_t* RBO, uint32_t* Texture)
{
  // Screen framebuffer, renderbuffer and texture setup
  glGenFramebuffers(1, FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, *FBO);

  glGenTextures(1, Texture);
  glBindTexture(GL_TEXTURE_2D, *Texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT,
               NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *Texture, 0);
  glGenRenderbuffers(1, RBO);
  glBindRenderbuffer(GL_RENDERBUFFER, *RBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *RBO);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    assert(0 && "error: incomplete framebuffer!\n");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
