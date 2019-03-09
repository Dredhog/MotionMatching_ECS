#include "game.h"
#include "debug_drawing.h"

void
AttachEntityToAnimEditor(game_state* GameState, EditAnimation::animation_editor* Editor,
                         int32_t EntityIndex)
{
  entity* AddedEntity = {};
  if(GetEntityAtIndex(GameState, &AddedEntity, EntityIndex))
  {
    Render::model* Model = GameState->Resources.GetModel(AddedEntity->ModelID);
    assert(Model->Skeleton);
		
    /* #1 */ //*Editor = {};
		
		/* #2 */ memset(Editor, 0, sizeof(EditAnimation::animation_editor));

    Editor->Skeleton    = Model->Skeleton;
    Editor->Transform   = &AddedEntity->Transform;
    Editor->EntityIndex = EntityIndex;
  }
}

void
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

void
RegisterDebugModels(game_state* GameState)
{
  GameState->GizmoModelID       = GameState->Resources.RegisterModel("data/built/gizmo1.model");
  GameState->BoneDiamondModelID = GameState->Resources.RegisterModel("data/built/bone_diamond.model");
  GameState->QuadModelID     = GameState->Resources.RegisterModel("data/built/debug_meshes.model");
  GameState->CubemapModelID  = GameState->Resources.RegisterModel("data/built/inverse_cube.model");
  GameState->SphereModelID   = GameState->Resources.RegisterModel("data/built/sphere.model");
  GameState->LowPolySphereModelID   = GameState->Resources.RegisterModel("data/built/low_poly_sphere.model");
  GameState->UVSphereModelID = GameState->Resources.RegisterModel("data/built/uv_sphere.model");
  GameState->Resources.Models.AddReference(GameState->GizmoModelID);
  GameState->Resources.Models.AddReference(GameState->BoneDiamondModelID);
  GameState->Resources.Models.AddReference(GameState->QuadModelID);
  GameState->Resources.Models.AddReference(GameState->CubemapModelID);
  GameState->Resources.Models.AddReference(GameState->SphereModelID);
  GameState->Resources.Models.AddReference(GameState->LowPolySphereModelID);
  GameState->Resources.Models.AddReference(GameState->UVSphereModelID);
  strcpy(GameState->R.Cubemap.Name, "data/textures/skybox/morning");
  strcpy(GameState->R.Cubemap.Format, "tga");
  GetCubemapRIDs(GameState->R.Cubemap.FaceIDs, &GameState->Resources, GameState->TemporaryMemStack,
                 GameState->R.Cubemap.Name, GameState->R.Cubemap.Format);
  GameState->R.Cubemap.CubemapTexture = -1;
  for(int i = 0; i < 6; i++)
  {
    GameState->Resources.Textures.AddReference(GameState->R.Cubemap.FaceIDs[i]);
  }
}

uint32_t
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

void
GenerateScreenQuad(uint32_t* VAO, uint32_t* VBO)
{
  // Create VAO and VBO for screen quad
  // Position   // TexCoord
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

void
AddEntity(game_state* GameState, rid ModelID, rid* MaterialIDs, Anim::transform Transform)
{
  assert(0 <= GameState->EntityCount && GameState->EntityCount < ENTITY_MAX_COUNT);

  entity NewEntity      = {};
  NewEntity.ModelID     = ModelID;
  NewEntity.MaterialIDs = MaterialIDs;
  NewEntity.Transform   = Transform;
  GameState->Resources.Models.AddReference(ModelID);

  GameState->Entities[GameState->EntityCount++] = NewEntity;
}

// Note(Lukas): Currently should not be used as no references are actually added
void
RemoveAnimationReferences(Resource::resource_manager* Resources,
                          Anim::animation_controller* Controller)
{
  for(int i = 0; i < Controller->AnimStateCount; i++)
  {
    Resources->Animations.RemoveReference(Controller->AnimationIDs[i]);
    Controller->AnimationIDs[i] = {};
    Controller->Animations[i] = {};
  }
}

bool
DeleteEntity(game_state* GameState, int32_t Index)
{
  if(0 <= Index && GameState->EntityCount)
  {
    GameState->Resources.Models.RemoveReference(GameState->Entities[Index].ModelID);
		if(GameState->Entities[Index].AnimController)
		{
				RemoveAnimationReferences(&GameState->Resources, GameState->Entities[Index].AnimController);
		}

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

bool
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

void AttachEntityToAnimEditor(game_state* GameState, EditAnimation::animation_editor* Editor,
                              int32_t EntityIndex);

void
DettachEntityFromAnimEditor(const game_state* GameState, EditAnimation::animation_editor* Editor)
{
  assert(GameState->Entities[Editor->EntityIndex].AnimController);
  assert(Editor->Skeleton);
  memset(Editor, 0, sizeof(EditAnimation::animation_editor));
}

bool
GetSelectedEntity(game_state* GameState, entity** OutputEntity)
{
  return GetEntityAtIndex(GameState, OutputEntity, GameState->SelectedEntityIndex);
}

bool
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

mat4
GetEntityModelMatrix(game_state* GameState, int32_t EntityIndex)
{
  mat4 ModelMatrix = TransformToMat4(GameState->Entities[EntityIndex].Transform);
  return ModelMatrix;
}

mat4
GetEntityMVPMatrix(game_state* GameState, int32_t EntityIndex)
{
  mat4 ModelMatrix = GetEntityModelMatrix(GameState, EntityIndex);
  mat4 MVPMatrix   = Math::MulMat4(GameState->Camera.VPMatrix, ModelMatrix);
  return MVPMatrix;
}

void
DrawSkeleton(const Anim::animation_controller* C, mat4 MatModel, float JointSphereRadius,
             bool UseBoneDiamonds)
{
  const mat4 Mat4Root = Math::MulMat4(MatModel, Math::MulMat4(C->HierarchicalModelSpaceMatrices[0],
                                                              C->Skeleton->Bones[0].BindPose));
  for(int b = 0; b < C->Skeleton->BoneCount; b++)
  {
    mat4 Mat4Bone = Math::MulMat4(MatModel, Math::MulMat4(C->HierarchicalModelSpaceMatrices[b],
                                                          C->Skeleton->Bones[b].BindPose));
    vec3 Position = Math::GetMat4Translation(Mat4Bone);

    if(0 < b)
    {
      int  ParentIndex = C->Skeleton->Bones[b].ParentIndex;
      mat4 Mat4Parent =
        Math::MulMat4(MatModel, Math::MulMat4(C->HierarchicalModelSpaceMatrices[ParentIndex],
                                              C->Skeleton->Bones[ParentIndex].BindPose));
      vec3 ParentPosition = Math::GetMat4Translation(Mat4Parent);

      if(UseBoneDiamonds)
      {
        float BoneLength    = Math::Length(Position - ParentPosition);
        vec3  ParentToChild = Math::Normalized(Position - ParentPosition);

        vec3 Forward = Math::Normalized(
          Math::Cross(ParentToChild, { Mat4Root._11, Mat4Root._12, Mat4Root._13 }));
        vec3 Right = Math::Normalized(Math::Cross(ParentToChild, Forward));

        mat4 DiamondMatrix = Mat4Parent;

        DiamondMatrix.X = Right;
        DiamondMatrix.Y = ParentToChild;
        DiamondMatrix.Z = Forward;

        Debug::PushShadedBone(DiamondMatrix, BoneLength);
      }
      else
      {
        Debug::PushLine(Position, ParentPosition);
      }
    }
    Debug::PushWireframeSphere(Position, JointSphereRadius);
  }
}
