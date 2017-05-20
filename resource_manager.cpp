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
          SetUpMesh(Model->Meshes[i]);
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

    rid RID = this->RegisterMaterial(FinalPath.Name);
    return RID;
  }

  bool
  resource_manager::GetModelPathRID(rid* RID, const char* Path)
  {
    return this->Models.GetPathRID(RID, Path);
  }

  bool
  resource_manager::GetTexturePathRID(rid* RID, const char* Path)
  {
    return this->Textures.GetPathRID(RID, Path);
  }

  bool
  resource_manager::GetAnimationPathRID(rid* RID, const char* Path)
  {
    return this->Animations.GetPathRID(RID, Path);
  }

  bool
  resource_manager::GetMaterialPathRID(rid* RID, const char* Path)
  {
    return this->Materials.GetPathRID(RID, Path);
  }

  rid
  resource_manager::RegisterModel(const char* Path)
  {
    assert(Path);
    rid RID;
    assert(this->Models.NewRID(&RID));
    this->Models.Set(RID, NULL, Path);
    printf("registered model: rid %d, %s\n", RID.Value, Path);
    return RID;
  }

  rid
  resource_manager::RegisterTexture(const char* Path)
  {
    assert(Path);
    rid RID;
    assert(this->Textures.NewRID(&RID));
    this->Textures.Set(RID, 0, Path);
    printf("registered texture: rid %d, %s\n", RID.Value, Path);
    return RID;
  }

  rid
  resource_manager::RegisterAnimation(const char* Path)
  {
    assert(Path);
    rid RID;
    assert(this->Animations.NewRID(&RID));
    this->Animations.Set(RID, NULL, Path);
    printf("registered animation: rid %d, %s\n", RID.Value, Path);
    return RID;
  }

  rid
  resource_manager::RegisterMaterial(const char* Path)
  {
    assert(Path);
    rid RID;
    assert(this->Materials.NewRID(&RID));
    this->Materials.Set(RID, NULL, Path);
    printf("registered material: rid %d, %s\n", RID.Value, Path);
    return RID;
  }

  bool
  resource_manager::AsociateModel(rid RID, char* Path)
  {
    assert(Path);
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    this->Models.Set(RID, NULL, Path);
    return true;
  }

  bool
  resource_manager::AsociateTexture(rid RID, char* Path)
  {
    assert(Path);
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    this->Textures.Set(RID, 0, Path);
    return true;
  }

  Render::model*
  resource_manager::GetModel(rid RID)
  {
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    Render::model* Model;
    char*          Path;
    if(this->Models.Get(RID, &Model, &Path))
    {
      if(strcmp(Path, "") == 0)
      {
        assert(0 && "assert: No path associated with rid");
      }
      else if(Model)
      {
        return Model;
      }
      else
      {
        if(this->LoadModel(RID))
        {
          this->Models.Get(RID, &Model, &Path);
          printf("loaded model: rid %d, %s\n", RID.Value, Path);
          assert(Model);
          return Model;
        }
        else
        {
          // Should be default model here
          printf("model: %s not found!", Path);
          assert(0 && "model not found");
        }
      }
    }
    else
    {
      assert(0 && "Invalid rid");
    }
    assert(0 && "Invalid codepath");
  }

  Anim::animation*
  resource_manager::GetAnimation(rid RID)
  {
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    Anim::animation* Animation;
    char*            Path;
    if(this->Animations.Get(RID, &Animation, &Path))
    {
      if(strcmp(Path, "") == 0)
      {
        assert(0 && "assert: No path associated with rid");
      }
      else if(Animation)
      {
        return Animation;
      }
      else
      {
        if(this->LoadAnimation(RID))
        {
          this->Animations.Get(RID, &Animation, &Path);
          printf("loaded animation: rid %d, %s\n", RID.Value, Path);
          assert(Animation);
          return Animation;
        }
        else
        {
          // Should be default animation here
          printf("animation: %s not found!", Path);
          assert(0 && "animation not found");
        }
      }
    }
    else
    {
      assert(0 && "Invalid rid");
    }
    assert(0 && "Invalid codepath");
  }

  material*
  resource_manager::GetMaterial(rid RID)
  {
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    material* Material;
    char*     Path;
    if(this->Materials.Get(RID, &Material, &Path))
    {
      if(strcmp(Path, "") == 0)
      {
        assert(0 && "assert: No path associated with rid");
      }
      else if(Material)
      {
        return Material;
      }
      else
      {
        if(this->LoadMaterial(RID))
        {
          this->Materials.Get(RID, &Material, &Path);
          printf("loaded material: rid %d, %s\n", RID.Value, Path);
          assert(Material);
          return Material;
        }
        else
        {
          // Should be default material here
          printf("material: %s not found!", Path);
          assert(0 && "material not found");
        }
      }
    }
    else
    {
      assert(0 && "Invalid rid");
    }
    assert(0 && "Invalid codepath");
  }

  uint32_t
  resource_manager::GetTexture(rid RID)
  {
    assert(0 < RID.Value && RID.Value <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
    uint32_t TextureID;
    char*    Path;
    if(this->Textures.Get(RID, &TextureID, &Path))
    {
      if(strcmp(Path, "") == 0)
      {
        printf("getting texture: rid %d\n", RID.Value);
        assert(0 && "assert: No path associated with rid");
      }
      else if(TextureID)
      {
        return TextureID;
      }
      else
      {
        printf("getting texture: rid %d, %s\n", RID.Value, Path);
        if(this->LoadTexture(RID))
        {
          this->Textures.Get(RID, &TextureID, &Path);
          printf("loaded texture: rid %d, TexID %u, %s\n", RID.Value, TextureID, Path);
          assert(TextureID);
          return TextureID;
        }
        else
        {
          printf("UNABLE TO LOAD: %s\n", Path);
          assert(0 && "failed to load texture");
          return -1; // Should be default model here
        }
      }
    }
    else
    {
      assert(0 && "assert: invalid rid");
    }
    assert(0 && "assert: invalid codepath");
  }

  int32_t
  resource_manager::GetTexturePathIndex(rid RID)
  {
    char* Path = {};
    this->Textures.Get(RID, 0, &Path);
    assert(this->Textures.GetPathRID(&RID, Path));
    for(int i = 0; i < this->TexturePathCount; i++)
    {
      if(strcmp(this->TexturePaths[i].Name, Path) == 0)
      {
        return i;
      }
    }
    printf("could not find texture: %s\n", Path);
    assert(0 && "texture path not found");
    return -1;
  }

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
