#include "stack_allocator.h"
#include "load_texture.h"
#include "asset.h"
#include "model.h"
#include "mesh.h"
#include "anim.h"
#include "text.h"
#include "game.h"

#define RESOURCE_MANAGER_RESOURCE_CAPACITY 30

/*For every type Will need:
 * Mapping from RID to path
 * Mapping from path to RID
 * Mapping from RID to asset pointer
 */

namespace Resource
{
  struct path
  {
    char Name[TEXT_LINE_MAX_LENGTH];
  };

  typedef int32_t rid;

  class resource_hash_table // Disregard the current implementation (currently array)
  {
    path  Paths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    void* Assets[RESOURCE_MANAGER_RESOURCE_CAPACITY];

  public:
    bool Get(rid, void** Asset, char** Path);
    bool Set(rid, void* Asset, const char* Path);
    bool GetRID(rid* RID, const char* Path);
    bool NewRID(rid* RID);
    void Remove(rid RID);
  };

  class resource_manager
  {
    path AvailableModlePaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    path AvailableTexturePaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    path AvailableAnimationGroupPaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    path AvailableMaterialsPaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];

    resource_hash_table Models;
    resource_hash_table Textures;
    resource_hash_table AnimationGroups;
    resource_hash_table Materials;

    Memory::stack_allocator ModelStack;
    Memory::stack_allocator AnimationStack;
    Memory::stack_allocator MaterialStack;

    bool LoadModel(rid* RID);
    bool LoadTexture(rid* RID);
    bool LoadAnimationGroup(rid* RID);
    bool LoadMaterial(rid* RID);

  public:
    bool LoadModel(rid* RID, const char* Path);
    bool LoadTexture(rid* RID, const char* Path);
    bool LoadAnimationGroup(rid* RID, const char* Path);
    bool LoadMaterial(rid* RID, const char* Path);

    bool RegisterModel(rid RID, char* Path);
    bool RegisterTexture(rid RID, char* Path);
    bool RegisterAnimationGroup(rid RID, char* Path);
    bool RegisterMaterial(rid RID, char* Path);

    Render::model*   GetModel(rid);
    uint32_t         GetTexture(rid);
    Anim::animation* GetAnimations(int* Count, rid);
    material* GetMaterial(rid);
  };
}
