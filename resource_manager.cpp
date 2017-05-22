#include "resource_manager.h"
#include "file_io.h"
#include "material_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>

int32_t ReadPaths(asset_diff* AssedDiffs, path* Paths, struct stat* Stats, int32_t* ElementCount,
                  const char* StartPath, const char* Extension);
namespace Resource
{
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
        debug_read_file_result AssetReadResult = ReadEntireFile(&this->ModelStack, Path);

        assert(AssetReadResult.Contents);
        if(AssetReadResult.ContentsSize <= 0)
        {
          return false;
        }
        Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

        UnpackAsset(AssetHeader);

        Model = (Render::model*)AssetHeader->Model;
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
  resource_manager::LoadAnimation(rid RID)
  {
    Anim::animation* Animation;
    char*            Path;
    if(this->Animations.Get(RID, &Animation, &Path))
    {
      if(Animation)
      {
        assert(0 && "Reloading model");
      }
      else
      {
        debug_read_file_result AssetReadResult = ReadEntireFile(&this->AnimationStack, Path);

        assert(AssetReadResult.Contents);
        if(AssetReadResult.ContentsSize <= 0)
        {
          return false;
        }
        Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

        UnpackAsset(AssetHeader);

        Animation =
          (Anim::animation*)((Anim::animation_group*)AssetHeader->AnimationGroup)->Animations[0];
        assert(Animation);

        this->Animations.Set(RID, Animation, Path);
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
        *Material = ImportMaterial(this, Path);
        this->Materials.Set(RID, Material, Path);
      }
      return true;
    }
    return false;
  }

  rid
  resource_manager::CreateMaterial(material Material, const char* Name)
  {
    path FinalPath = {};
    if(!Name)
    {
      time_t     current_time;
      struct tm* time_info;
      char       MaterialName[30];
      time(&current_time);
      time_info = localtime(&current_time);
      strftime(MaterialName, sizeof(MaterialName), "%H_%M_%S", time_info);
      ExportMaterial(this, &Material, "data/materials/", MaterialName);
      strftime(FinalPath.Name, sizeof(FinalPath.Name), "data/materials/%H_%M_%S.mat", time_info);
    }
    else
    {
      ExportMaterial(this, &Material, "data/materials/", Name);
      sprintf(FinalPath.Name, "data/materials/%s.mat", Name);
    }

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
    this->Textures.Clear();
  }

  void
  resource_manager::WipeAllModelData()
  {
    for(int i = 0; i < RESOURCE_MAX_COUNT; i++)
    {
      Render::model* Model;
      char*          Path;
      if(this->Models.Get({ i + 1 }, &Model, &Path))
      {
        if(Model)
        {
          for(int m = 0; m < Model->MeshCount; m++)
          {
            Render::CleanUpMesh(Model->Meshes[m]);
          }
        }
      }
    }
    this->Models.Clear();
    this->ModelStack.NullifyClear();
  }

  void
  resource_manager::WipeAllMaterialData()
  {
    this->Materials.Clear();
    this->MaterialStack.NullifyClear();
  }

  void
  resource_manager::WipeAllAnimationData()
  {
    this->Animations.Clear();
    this->AnimationStack.NullifyClear();
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
        printf("%-10s %-10s: rid %d, %s\n", "seeking", _STRING(TYPE_NAME), RID.Value, Path);       \
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
  }

  CREATE_GET_FUNCTION(uint32_t, Texture);
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
    printf("could not find " _STRING(texture) ": %s\n", Path);                                     \
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
    // Update models paths
    int DiffCount = ReadPaths(this->DiffedAssets, this->ModelPaths, this->ModelStats,
                              &this->ModelPathCount, "data/built", NULL);
    // Update texture paths
    DiffCount = ReadPaths(this->DiffedAssets, this->TexturePaths, this->TextureStats,
                          &this->TexturePathCount, "data/textures", NULL);
    // Update animation paths
    DiffCount = ReadPaths(this->DiffedAssets, this->AnimationPaths, this->AnimationStats,
                          &this->AnimationPathCount, "data/animations", "anim");
    // Update material paths
    DiffCount = ReadPaths(this->DiffedAssets, this->MaterialPaths, this->MaterialStats,
                          &this->MaterialPathCount, "data/materials", "mat");
#define LOG_HARD_DRIVE_CHANGES 0
#if LOG_HARD_DRIVE_CHANGES
    if(DiffCount > 0)
    {
      printf("DIFF COUNT: %d\n", DiffCount);
      for(int i = 0; i < DiffCount; i++)
      {
        switch(this->DiffedAssets[i].Type)
        {
          case DIFF_Added:
            printf("Added: ");
            break;
          case DIFF_Modified:
            printf("Modified: ");
            break;
          case DIFF_Deleted:
            printf("Deleted: ");
            break;
          default:
            assert(0 && "assert: overflowed stat enum");
            break;
        }
        printf("%s\n", DiffedAssets[i].Path.Name);
      }
    }
#endif
#undef LOG_HARD_DRIVE_CHANGES
  }
}

//-----------------------------------FILE QUERIES--------------------------------------

#include "file_queries.h"
