#pragma once

#include "stack_allocator.h"
#include "load_texture.h"
#include "asset.h"
#include "model.h"
#include "mesh.h"
#include "anim.h"
#include "text.h"
#include "render_data.h"

#define RESOURCE_MANAGER_RESOURCE_CAPACITY 30

/*For every type Will need:
 * Mapping from RID to path
 * Mapping from path to RID
 * Mapping from RID to asset pointer
 */

#define INVALID_RID 0

namespace Resource
{
  struct path
  {
    char Name[TEXT_LINE_MAX_LENGTH];
  };

  class resource_hash_table // Disregard the current implementation (currently array)
  {
    path  Paths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    void* Assets[RESOURCE_MANAGER_RESOURCE_CAPACITY];

  public:
    bool Get(rid, void** Asset, char** Path);
    void Set(rid RID, const void* Asset, const char* Path);
    bool FindPathRID(rid* RID, const char* Path);
    bool NewRID(rid* RID);
  };

  class resource_manager
  {
    //path AvailableModlePaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    //path AvailableTexturePaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    //path AvailableAnimationGroupPaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    //path AvailableMaterialsPaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];

    resource_hash_table Models;
    resource_hash_table Textures;

  public:
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

    Render::model* GetModel(rid RID);
    uint32_t       GetTexture(rid RID);
  };
}
