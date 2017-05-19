#pragma once

#include "stack_allocator.h"
#include "load_texture.h"
#include "asset.h"
#include "model.h"
#include "mesh.h"
#include "anim.h"
#include "render_data.h"
#include "text.h"

#include <sys/stat.h>
#define RESOURCE_MANAGER_RESOURCE_CAPACITY 200

/*For every type Will need:
 * Mapping from RID to path
 * Mapping from path to RID
 * Mapping from RID to asset pointer
 */

#define INVALID_RID 0

struct path
{
  char Name[TEXT_LINE_MAX_LENGTH];
};

enum asset_diff_type
{
  DIFF_Added,
  DIFF_Modified,
  DIFF_Deleted,

  DIFF_EnumCount
};

struct asset_diff
{
  asset_diff_type Type;
  path            Path;
};

namespace Resource
{

  class resource_hash_table // Disregard the current implementation (currently array)
  {
    path  Paths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    void* Assets[RESOURCE_MANAGER_RESOURCE_CAPACITY];

  public:
    bool Get(rid, void** Asset, char** Path);
    void Set(rid RID, const void* Asset, const char* Path);
    bool GetPathRID(rid* RID, const char* Path);
    bool NewRID(rid* RID);
  };

  class resource_manager
  {
    asset_diff  DiffedAssets[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    struct stat ModelStats[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    struct stat TextureStats[RESOURCE_MANAGER_RESOURCE_CAPACITY];

    resource_hash_table Models;
    resource_hash_table Textures;

  public:
    path    ModelPaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    path    TexturePaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    int32_t ModelPathCount;
    int32_t TexturePathCount;

    Memory::stack_allocator ModelStack;
    Memory::stack_allocator AnimationStack;
    Memory::stack_allocator MaterialStack;

  private:
    bool LoadModel(rid RID);
    bool LoadTexture(rid RID);

  public:
    rid RegisterModel(const char* Path);
    rid RegisterTexture(const char* Path);

    bool AsociateModel(rid RID, char* Path);
    bool AsociateTexture(rid RID, char* Path);

    bool GetModelPathRID(rid* RID, const char* Path);
    bool GetTexturePathRID(rid* RID, const char* Path);

    Render::model* GetModel(rid RID);
    uint32_t       GetTexture(rid RID);
    void           UpdateHardDriveDisplay();
  };
}
