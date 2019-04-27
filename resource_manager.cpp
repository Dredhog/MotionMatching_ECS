#include "resource_manager.h"
#include "file_io.h"
#include "material_io.h"
#include "load_shader.h"
#include "profile.h"
#include "basic_data_structures.h"

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <cstring>

namespace Resource
{
  void
  resource_manager::Create(uint8_t* MemoryStart, uint32_t TotalMemorySize,
                           Memory::stack_allocator* TemporaryStack)
  {
    *this                         = {};
    uint32_t ModelHeapSize        = (uint32_t)((float)TotalMemorySize * 0.35f);
    uint32_t AnimationHeapSize    = (uint32_t)((float)TotalMemorySize * 0.3f);
    uint32_t MMControllerHeapSize = (uint32_t)((float)TotalMemorySize * 0.3f);
    uint32_t MaterialStackSize =
      TotalMemorySize - ModelHeapSize - AnimationHeapSize - MMControllerHeapSize;

    uint8_t* ModelHeapStart        = MemoryStart;
    uint8_t* AnimationHeapStart    = ModelHeapStart + ModelHeapSize;
    uint8_t* MMControllerHeapStart = AnimationHeapStart + AnimationHeapSize;
    uint8_t* MaterialStackStart    = MMControllerHeapStart + MMControllerHeapSize;

    this->ModelHeap.Create(ModelHeapStart, ModelHeapSize);
    this->ModelHeap.Clear();

    this->AnimationHeap.Create(AnimationHeapStart, AnimationHeapSize);
    this->ModelHeap.Clear();

    this->MMControllerHeap.Create(MMControllerHeapStart, MMControllerHeapSize);
    this->MMControllerHeap.Clear();

    this->MaterialStack.Create(MaterialStackStart, MaterialStackSize);
    this->MaterialStack.NullifyClear();

    // TODO(Lukas) remove this dependency on the global transient stack
    this->TemporaryStack = TemporaryStack;

    this->DefaultShaderID = 0;
  }

  void
  resource_manager::SetDefaultShaderID(GLuint ShaderID)
  {
    assert(DefaultShaderID == 0 && "Multiple DefaultShaderID assignemnts in resource manager");
    this->DefaultShaderID = ShaderID;
  }

  bool
  resource_manager::LoadModel(rid RID)
  {
    Render::model* Model;
    char*          Path;
    if(this->Models.Get(RID, &Model, &Path))
    {
      if(Model)
      {
        assert(0 && "Reloading model");
      }
      else
      {
        debug_read_file_result AssetReadResult = Platform::ReadEntireFile(&this->ModelHeap, Path);

        assert(AssetReadResult.Contents);
        if(AssetReadResult.ContentsSize <= 0)
        {
          return false;
        }

        Render::model* Model = (Render::model*)AssetReadResult.Contents;

        Asset::UnpackModel(Model);
        assert(Model);

        // Post load initialization
        for(int i = 0; i < Model->MeshCount; i++)
        {
          Render::SetUpMesh(Model->Meshes[i]);
        }

        this->Models.Set(RID, Model, Path);
      }
      return true;
    }
    return false;
  }

  bool
  resource_manager::LoadTexture(rid RID)
  {
    char* Path;
    if(this->Textures.Get(RID, 0, &Path))
    {
      uint32_t TextureID = Texture::LoadTexture(Path);
      this->Textures.Set(RID, TextureID, Path);
      return true;
    }
    return false;
  }

  bool
  resource_manager::LoadShader(rid RID)
  {
    char* Path;
    if(this->Shaders.Get(RID, 0, &Path))
    {
      GLuint ShaderID = Shader::CheckedLoadCompileFreeShader(this->TemporaryStack, Path);
      // Logic for assigning a default shader on failed load
      if(ShaderID == 0)
      {
        ShaderID = this->DefaultShaderID;
        assert(ShaderID != 0);
      }
      this->Shaders.Set(RID, ShaderID, Path);
      return true;
    }
    return false;
  }

  bool
  resource_manager::LoadAnimation(rid RID)
  {
    Anim::animation* Animation;
    char*            Path;
    if(this->Animations.Get(RID, &Animation, &Path))
    {
      if(Animation)
      {
        assert(0 && "Reloading animation");
      }
      else
      {
        debug_read_file_result AssetReadResult =
          Platform::ReadEntireFile(&this->AnimationHeap, Path);

        assert(AssetReadResult.Contents);
        if(AssetReadResult.ContentsSize <= 0)
        {
          return false;
        }
        Anim::animation_group* AnimationGroup = (Anim::animation_group*)AssetReadResult.Contents;
        Asset::UnpackAnimationGroup(AnimationGroup);

        Animation = AnimationGroup->Animations[0];

        this->Animations.Set(RID, Animation, Path);
      }
      return true;
    }
    return false;
  }

  bool
  resource_manager::LoadMMController(rid RID)
  {
    mm_controller_data* Controller;
    char*               Path;
    if(this->MMControllers.Get(RID, &Controller, &Path))
    {
      if(Controller)
      {
        assert(0 && "Reloading MMController");
      }
      else
      {
        debug_read_file_result AssetReadResult =
          Platform::ReadEntireFile(&this->MMControllerHeap, Path);

        assert(AssetReadResult.Contents);
        if(AssetReadResult.ContentsSize <= 0 || AssetReadResult.Contents == NULL)
        {
          return false;
        }
				assert(0 && "Please implement remapping of paths to animation RIDs");
        Controller = (mm_controller_data*)AssetReadResult.Contents;
        this->MMControllers.Set(RID, Controller, Path);
      }
      return true;
    }
    return false;
  }

  bool
  resource_manager::LoadMaterial(rid RID)
  {
    material* Material;
    char*     Path;
    if(this->Materials.Get(RID, &Material, &Path))
    {
      if(Material)
      {
        assert(0 && "Reloading material");
      }
      else
      {
        Material  = PushStruct(&this->MaterialStack, material);
        *Material = ImportMaterial(&this->MaterialStack, this, Path);
        this->Materials.Set(RID, Material, Path);
      }
      return true;
    }
    return false;
  }

  rid
  resource_manager::CreateMaterial(material Material, const char* Path)
  {
    path FinalPath = {};
    if(!Path)
    {
      time_t     current_time;
      struct tm* time_info;
      time(&current_time);
      time_info = localtime(&current_time);
      strftime(FinalPath.Name, sizeof(FinalPath.Name), "data/materials/%H_%M_%S.mat", time_info);
    }
    else
    {
      sprintf(FinalPath.Name, "%s", Path);
    }

    ExportMaterial(this, &Material, FinalPath.Name);
    rid RID = this->RegisterMaterial(FinalPath.Name);
    return RID;
  }

  void
  resource_manager::WipeAllTextureData()
  {
    for(int i = 0; i < RESOURCE_MAX_COUNT; i++)
    {
      uint32_t Texture;
      char*    Path;
      if(this->Textures.Get({ i + 1 }, &Texture, &Path))
      {
        if(Texture)
        {
          glDeleteTextures(1, &Texture);
        }
      }
    }
    this->Textures.Reset();
  }

  void
  resource_manager::WipeAllShaderData()
  {
    for(int i = 0; i < RESOURCE_MAX_COUNT; i++)
    {
      this->FreeShader({ i + 1 });
    }
    this->Shaders.Reset();
  }

  void
  resource_manager::WipeAllMMControllerData()
  {
    for(int i = 0; i < RESOURCE_MAX_COUNT; i++)
    {
      this->FreeMMController({ i + 1 });
    }
    this->MMControllers.Reset();
  }

  void
  resource_manager::WipeAllModelData()
  {
    for(int i = 0; i < RESOURCE_MAX_COUNT; i++)
    {
      this->FreeModel({ i + 1 });
    }
    this->Models.Reset();
    this->ModelHeap.Clear();
  }

  void
  resource_manager::FreeModel(rid RID)
  {
    Render::model* Model;
    char*          Path;
    if(this->Models.Get(RID, &Model, &Path))
    {
      if(Model)
      {
        for(int m = 0; m < Model->MeshCount; m++)
        {
          Render::CleanUpMesh(Model->Meshes[m]);
        }
        this->ModelHeap.Dealloc((uint8_t*)Model);
        this->Models.SetAsset(RID, NULL);
      }
    }
  }

  void
  resource_manager::FreeShader(rid RID)
  {
    GLuint Shader;
    char*  Path;
    if(this->Shaders.Get(RID, &Shader, &Path))
    {
      if(Shader)
      {
        glDeleteShader(Shader);
      }
      else
      {
        assert(0 && "A shader with id 0 was attempted to be freed");
      }
    }
  }

  void
  resource_manager::FreeAnimation(rid RID)
  {
    Anim::animation* Animation;
    char*            Path;
    if(this->Animations.Get(RID, &Animation, &Path))
    {
      if(Animation)
      {
        // TODO(LUKAS) TOTAL HACK will work if only one animation in anim file
        this->AnimationHeap.Dealloc((uint8_t*)Animation - sizeof(Anim::animation_group) -
                                    sizeof(Anim::animation*));
        this->Animations.SetAsset(RID, NULL);
      }
    }
  }

  void
  resource_manager::FreeMMController(rid RID)
  {
    mm_controller_data* Controller;
    char*            Path;
    if(this->MMControllers.Get(RID, &Controller, &Path))
    {
      if(Controller)
      {
				for(int i = 0; i < Controller->Params.AnimRIDs.Count; i++)
				{
          this->Animations.RemoveReference(Controller->Params.AnimRIDs[i]);
        }

        this->MMControllerHeap.Dealloc((uint8_t*)Controller);
        this->MMControllers.SetAsset(RID, NULL);
      }
    }
  }

  void
  resource_manager::WipeAllMaterialData()
  {
    this->Materials.Reset();
    this->MaterialStack.NullifyClear();
  }

  void
  resource_manager::WipeAllAnimationData()
  {
    this->Animations.Reset();
    this->AnimationHeap.Clear();
    // this->AnimationStack.NullifyClear();
  }

#define _STRING(X) #X

#define CREATE_ASSOCIATE_FUNCTION(STORED_TYPE, TYPE_NAME)                                          \
  bool resource_manager::Associate##TYPE_NAME##IDToPath(rid RID, const char* Path)                 \
  {                                                                                                \
    assert(0 < RID.Value && RID.Value <= RESOURCE_MAX_COUNT);                                      \
    STORED_TYPE Old##TYPE_NAME;                                                                    \
    char*       OldPath;                                                                           \
    this->TYPE_NAME##s.Get(RID, &Old##TYPE_NAME, &OldPath);                                        \
    assert(!Old##TYPE_NAME);                                                                       \
    this->TYPE_NAME##s.Set(RID, 0, Path);                                                          \
    return true;                                                                                   \
  }

  CREATE_ASSOCIATE_FUNCTION(Render::model*, Model);
  CREATE_ASSOCIATE_FUNCTION(Anim::animation*, Animation);
  CREATE_ASSOCIATE_FUNCTION(uint32_t, Texture);
  CREATE_ASSOCIATE_FUNCTION(material*, Material);

#define CREATE_GET_PATH_ID_FUNCTION(TYPE_NAME)                                                     \
  bool resource_manager::Get##TYPE_NAME##PathRID(rid* RID, const char* Path)                       \
  {                                                                                                \
    return this->TYPE_NAME##s.GetPathRID(RID, Path);                                               \
  }

  CREATE_GET_PATH_ID_FUNCTION(Model);
  CREATE_GET_PATH_ID_FUNCTION(Texture);
  CREATE_GET_PATH_ID_FUNCTION(Animation);
  CREATE_GET_PATH_ID_FUNCTION(Material);

#define CREATE_OBTAIN_PATH_ID_FUNCTION(TYPE_NAME)                                                  \
  rid resource_manager::Obtain##TYPE_NAME##PathRID(const char* Path)                               \
  {                                                                                                \
    rid RID;                                                                                       \
    if(!this->TYPE_NAME##s.GetPathRID(&RID, Path))                                                 \
    {                                                                                              \
      RID = Register##TYPE_NAME(Path);                                                             \
      assert(RID.Value > 0);                                                                       \
    }                                                                                              \
    return RID;                                                                                    \
  }

  CREATE_OBTAIN_PATH_ID_FUNCTION(Model);
  CREATE_OBTAIN_PATH_ID_FUNCTION(Texture);
  CREATE_OBTAIN_PATH_ID_FUNCTION(Animation);
  CREATE_OBTAIN_PATH_ID_FUNCTION(Material);

#define CREATE_REGISTER_FUNCTION(TYPE_NAME)                                                        \
  rid resource_manager::Register##TYPE_NAME(const char* Path)                                      \
  {                                                                                                \
    assert(Path);                                                                                  \
    rid RID;                                                                                       \
    assert(this->TYPE_NAME##s.NewRID(&RID));                                                       \
    this->TYPE_NAME##s.Set(RID, 0, Path);                                                          \
    printf("%-10s %-10s: rid %d, %s\n", "registered", _STRING(TYPE_NAME), RID.Value, Path);        \
    return RID;                                                                                    \
  }

  CREATE_REGISTER_FUNCTION(Model);
  CREATE_REGISTER_FUNCTION(Texture);
  CREATE_REGISTER_FUNCTION(Animation);
  CREATE_REGISTER_FUNCTION(Material);
  CREATE_REGISTER_FUNCTION(Shader);

#define CREATE_GET_FUNCTION(STORED_TYPE, TYPE_NAME)                                                \
  STORED_TYPE resource_manager::Get##TYPE_NAME(rid RID)                                            \
  {                                                                                                \
    if(!(0 < RID.Value && RID.Value <= RESOURCE_MAX_COUNT))                                        \
    {                                                                                              \
      printf("FOR RID: %d\n", RID.Value);                                                          \
    }                                                                                              \
    assert(0 < RID.Value && RID.Value <= RESOURCE_MAX_COUNT);                                      \
    STORED_TYPE TYPE_NAME;                                                                         \
    char*       Path;                                                                              \
    if(this->TYPE_NAME##s.Get(RID, &TYPE_NAME, &Path))                                             \
    {                                                                                              \
      if(strcmp(Path, "") == 0)                                                                    \
      {                                                                                            \
        printf("failed to find path for " _STRING(TYPE_NAME) ": rid %d\n", RID.Value);             \
        assert(0 && "assert: No path associated with rid");                                        \
      }                                                                                            \
      else if(TYPE_NAME)                                                                           \
      {                                                                                            \
        return TYPE_NAME;                                                                          \
      }                                                                                            \
      else                                                                                         \
      {                                                                                            \
        /*printf("%-10s %-10s: rid %d, %s\n", "seeking", _STRING(TYPE_NAME), RID.Value, Path);*/   \
        if(this->Load##TYPE_NAME(RID))                                                             \
        {                                                                                          \
          this->TYPE_NAME##s.Get(RID, &TYPE_NAME, &Path);                                          \
          printf("%-10s %-10s: rid %d, %s\n", "loaded", _STRING(TYPE_NAME), RID.Value, Path);      \
          assert(TYPE_NAME);                                                                       \
          return TYPE_NAME;                                                                        \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
          printf("UNABLE TO LOAD: %s\n", Path);                                                    \
          assert(0 && "failed to load" _STRING(TYPE_NAME));                                        \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      assert(0 && "assert: invalid rid");                                                          \
    }                                                                                              \
    assert(0 && "assert: invalid codepath");                                                       \
    return TYPE_NAME;                                                                              \
  }

  CREATE_GET_FUNCTION(uint32_t, Texture);
  CREATE_GET_FUNCTION(GLuint, Shader);
  CREATE_GET_FUNCTION(Render::model*, Model);
  CREATE_GET_FUNCTION(Anim::animation*, Animation);
  CREATE_GET_FUNCTION(material*, Material);

#define CREATE_GET_PATH_INDEX_FUNCTION(TYPE_NAME)                                                  \
  int32_t resource_manager::Get##TYPE_NAME##PathIndex(rid RID)                                     \
  {                                                                                                \
    char* Path = {};                                                                               \
    this->TYPE_NAME##s.Get(RID, 0, &Path);                                                         \
    assert(this->TYPE_NAME##s.GetPathRID(&RID, Path));                                             \
    for(int i = 0; i < this->TYPE_NAME##PathCount; i++)                                            \
    {                                                                                              \
      if(strcmp(this->TYPE_NAME##Paths[i].Name, Path) == 0)                                        \
      {                                                                                            \
        return i;                                                                                  \
      }                                                                                            \
    }                                                                                              \
    printf("could not find " _STRING(TYPE_NAME) ": %s\n", Path);                                   \
    assert(0 && _STRING(texture) " path not found");                                               \
    return -1;                                                                                     \
  }

  CREATE_GET_PATH_INDEX_FUNCTION(Texture);
  CREATE_GET_PATH_INDEX_FUNCTION(Material);
  CREATE_GET_PATH_INDEX_FUNCTION(Animation);
  // CREATE_GET_PATH_INDEX_FUNCTION(Model);

  void
  resource_manager::UpdateHardDriveAssetPathLists()
  {
    TIMED_BLOCK(UpdateAssetPathLists);
    // Update models paths
    this->DiffedModelCount =
      Platform::ReadPaths(this->DiffedModels, this->ModelPaths, this->ModelStats,
                          RESOURCE_MAX_COUNT, &this->ModelPathCount, "data/built", NULL);
    // Update texture paths
    this->DiffedTextureCount =
      Platform::ReadPaths(this->DiffedTextures, this->TexturePaths, this->TextureStats,
                          RESOURCE_MAX_COUNT, &this->TexturePathCount, "data/textures", NULL);
    // Update animation paths
    this->DiffedAnimationCount =
      Platform::ReadPaths(this->DiffedAnimations, this->AnimationPaths, this->AnimationStats,
                          RESOURCE_MAX_COUNT, &this->AnimationPathCount, "data/animations", "anim");
    // Update scene paths
    this->DiffedSceneCount =
      Platform::ReadPaths(this->DiffedScenes, this->ScenePaths, this->SceneStats,
                          RESOURCE_MAX_COUNT, &this->ScenePathCount, "data/scenes", "scene");
    // Update material paths
    this->DiffedMaterialCount =
      Platform::ReadPaths(this->DiffedMaterials, this->MaterialPaths, this->MaterialStats,
                          RESOURCE_MAX_COUNT, &this->MaterialPathCount, "data/materials", "mat");
    // Update shader paths
    this->DiffedShaderCount =
      Platform::ReadPaths(this->DiffedShaders, this->ShaderPaths, this->ShaderStats,
                          RESOURCE_MAX_COUNT, &this->ShaderPathCount, "shaders", NULL);
		
    // Update shader paths
    this->DiffedShaderCount =
      Platform::ReadPaths(this->DiffedMMParams, this->MMParamPaths, this->MMParamStats,
                          RESOURCE_MAX_COUNT, &this->MMParamPathCount, "data/matching_params", "params");

    this->SortAllAssetDiffsPathsStats();
  }
	struct diff_path_stat
	{
		asset_diff Diff;
    path       Path;
    file_stat  Stat;
  };

  int
  DiffPathStatCmpFunc(const void* A, const void* B)
  {
    diff_path_stat* TupleA = (diff_path_stat*)A;
    diff_path_stat* TupleB = (diff_path_stat*)B;

    return strncmp(TupleA->Path.Name, TupleB->Path.Name, sizeof(path));
  }

  void
  SortAssetInfoByPath(asset_diff* Diffs, path* Paths, file_stat* Stats, int32_t AssetCount)
  {
    fixed_stack<diff_path_stat, RESOURCE_MAX_COUNT> DiffPathStatArray;
    DiffPathStatArray.Clear();
    for(int i = 0; i < AssetCount; i++)
    {
      DiffPathStatArray.Push(diff_path_stat{ Diffs[i], Paths[i], Stats[i] });
    }
    qsort(DiffPathStatArray.Elements, AssetCount, sizeof(diff_path_stat), DiffPathStatCmpFunc);
    for(int i = 0; i < AssetCount; i++)
    {
      Diffs[i] = DiffPathStatArray[i].Diff;
      Paths[i] = DiffPathStatArray[i].Path;
      Stats[i] = DiffPathStatArray[i].Stat;
    }
  }

  void
  resource_manager::SortAllAssetDiffsPathsStats()
  {
    SortAssetInfoByPath(this->DiffedModels, this->ModelPaths, this->ModelStats,
                        this->ModelPathCount);
    SortAssetInfoByPath(this->DiffedAnimations, this->AnimationPaths, this->AnimationStats,
                        this->AnimationPathCount);
    SortAssetInfoByPath(this->DiffedMaterials, this->MaterialPaths, this->MaterialStats,
                        this->MaterialPathCount);
    SortAssetInfoByPath(this->DiffedTextures, this->TexturePaths, this->TextureStats,
                        this->TexturePathCount);
    SortAssetInfoByPath(this->DiffedScenes, this->ScenePaths, this->SceneStats,
                        this->ScenePathCount);
  }

  void
  resource_manager::ReloadModified()
  {
    TIMED_BLOCK(ReloadModified);
    for(int i = 0; i < this->DiffedAnimationCount; i++)
    {
      // printf("diffed path: %s found to be registered\n", DiffedAnimations[i].Path.Name);
      if(this->DiffedAnimations[i].Type == DIFF_Modified)
      {
        // printf("modified path: %s found to be registered\n", DiffedAnimations[i].Path.Name);
        rid RID;
        if(this->Animations.GetPathRID(&RID, DiffedAnimations[i].Path.Name))
        {
          // printf("diffed modified registered path: %s\n", DiffedAnimations[i].Path.Name);
          Anim::animation* Animation;
          char*            Path;
          if(this->Animations.Get(RID, &Animation, &Path))
          {
            if(Animation)
            {
              printf("Reloading animation: %s\n", DiffedAnimations[i].Path.Name);
              FreeAnimation(RID);
              LoadAnimation(RID);
            }
          }
        }
      }
    }

    for(int i = 0; i < this->DiffedModelCount; i++)
    {
      if(this->DiffedModels[i].Type == DIFF_Modified)
      {
        rid RID;
        if(this->Models.GetPathRID(&RID, DiffedModels[i].Path.Name))
        {
          Render::model* Model;
          char*          Path;
          if(this->Models.Get(RID, &Model, &Path))
          {
            if(Model)
            {
              printf("Reloading model: %s\n", DiffedModels[i].Path.Name);
              FreeModel(RID);
              LoadModel(RID);
            }
          }
        }
      }
    }

    for(int i = 0; i < this->DiffedShaderCount; i++)
    {
      if(this->DiffedShaders[i].Type == DIFF_Modified)
      {
        rid  RID;
        path ShaderPathWithoutExtension;
        {
          char* LastDotInString = strrchr(DiffedShaders[i].Path.Name, '.');
          assert(LastDotInString && "Shader path is missing extension");
          size_t PathLengthWithoutExtension = LastDotInString - DiffedShaders[i].Path.Name;

          strncpy(ShaderPathWithoutExtension.Name, DiffedShaders[i].Path.Name,
                  PathLengthWithoutExtension);
          ShaderPathWithoutExtension.Name[PathLengthWithoutExtension] = '\0';
        }
        if(this->Shaders.GetPathRID(&RID, ShaderPathWithoutExtension.Name))
        {
          GLuint Shader;
          char*  Path;
          if(this->Shaders.Get(RID, &Shader, &Path))
          {
            if(Shader != 0)
            {
              printf("Reloading shader: %s\n", ShaderPathWithoutExtension.Name);
              if(Shader != this->DefaultShaderID)
              {
                FreeShader(RID);
              }
              LoadShader(RID);
            }
          }
        }
      }
    }
  }

  void
  resource_manager::DeleteUnused()
  {
    TIMED_BLOCK(DeleteUnused);
    for(int i = 1; i <= RESOURCE_MAX_COUNT; i++)
    {
      Render::model* Model;
      char*          Path;
      rid            RID = { i };
      if(this->Models.Get(RID, &Model, &Path))
      {
        if(Model)
        {
          int32_t RefCount = this->Models.QueryReferences(RID);
          if(RefCount <= 0)
          {
            // printf("deleting model rid: %d, refs: %d\n", RID.Value, RefCount);
            FreeModel(RID);
          }
        }
      }
    }
#if 1
    for(int i = 1; i <= RESOURCE_MAX_COUNT; i++)
    {
      Anim::animation* Animation;
      char*            Path;
      rid              RID = { i };
      if(this->Animations.Get(RID, &Animation, &Path))
      {
        if(Animation)
        {
          int32_t RefCount = this->Animations.QueryReferences(RID);
          if(RefCount <= 0)
          {
            printf("deleting animation rid: %d, refs: %d\n", RID.Value, RefCount);
            FreeAnimation(RID);
          }
        }
      }
    }
#endif
  }
}
