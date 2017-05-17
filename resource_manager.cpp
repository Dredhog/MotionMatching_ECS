#include "resource_manager.h"
#include "file_io.h"

#include <stdio.h>

namespace Resource
{
  bool
  resource_hash_table::Set(rid RID, void* Asset, const char* Path)
  {
    if(RID < 0 || this->Paths[RID].Name[0] == '\0')
    {
      return false;
    }

    if(Asset)
    {
      this->Assets[RID] = Asset;
    }

    if(Path)
    {
      size_t PathLength = strlen(Path);
      assert(PathLength <= RESOURCE_MANAGER_RESOURCE_CAPACITY);
      memcpy(this->Paths[RID].Name, Path, PathLength * sizeof(char));
    }
    return true;
  }

  bool
  resource_hash_table::Get(rid RID, void** Asset, char** Path)
  {
    if(RID < 0 || this->Paths[RID].Name[0] == '\0')
    {
      return false;
    }

    if(Asset)
    {
      *Asset = this->Assets[RID];
    }

    if(Path)
    {
      *Path = this->Paths[RID].Name;
    }
    return true;
  }

  bool
  resource_hash_table::GetRID(rid* RID, const char* Path)
  {
    for(int i = 0; i < RESOURCE_MANAGER_RESOURCE_CAPACITY; i++)
    {
      if(strcmp(Path, this->Paths[i].Name) == 0)
      {
        *RID = { i };
        return true;
      }
    }
    return false;
  }

  bool
  resource_hash_table::NewRID(rid* RID)
  {
    for(int i = 0; i < RESOURCE_MANAGER_RESOURCE_CAPACITY; i++)
    {
      if(this->Paths[i].Name[0] != 0)
      {
        rid NewRID = { i };
        *RID       = NewRID;
        return true;
      }
    }
    return false;
  }

  bool
  resource_manager::LoadModel(rid* RID, const char* Path)
  {
    assert(RID);
    if(this->Models.GetRID(RID, Path))
    {
      Render::model* NewModel = {};

      debug_read_file_result AssetReadResult = ReadEntireFile(&this->ModelStack, Path);

      assert(AssetReadResult.Contents);
      Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

      UnpackAsset(AssetHeader);

      NewModel = (Render::model*)AssetHeader->Model;
      assert(NewModel);

      for(int i = 0; i < NewModel->MeshCount; i++)
      {
        SetUpMesh(NewModel->Meshes[i]);
      }

      this->Models.Set(*RID, NewModel, Path);
      return true;
    }
    return false;
  }

  bool
  resource_manager::LoadTexture(rid* RID, const char* Path)
  {
    if(this->Textures.GetRID(RID, Path))
    {
      uint32_t TextureID = Texture::LoadTexture(Path);
      this->Textures.Set(*RID, (void*)(uintptr_t)TextureID, Path);
      return true;
    }
    return false;
  }
}
