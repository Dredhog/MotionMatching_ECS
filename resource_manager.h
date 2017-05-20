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

struct path
{
  char Name[TEXT_LINE_MAX_LENGTH];
};
#include "resource_hash_table.h"

typedef Resource::resource_hash_table<Render::model*, 200>         model_hash_table;
typedef Resource::resource_hash_table<Anim::animation*, 200> animation_group_hash_table;
typedef Resource::resource_hash_table<material*, 200>              material_hash_table;
typedef Resource::resource_hash_table<uint32_t, 200>               texture_hash_table;

static const int RESOURCE_MANAGER_RESOURCE_CAPACITY = 200;

enum asset_diff_type
{
  DIFF_Added,
  DIFF_Modified,
  DIFF_Deleted,

  DIFF_EnumCount
};

struct asset_diff
{
  uint32_t Type;
  path     Path;
};

namespace Resource
{
  class resource_manager
  {
    asset_diff  DiffedAssets[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    struct stat ModelStats[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    struct stat TextureStats[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    struct stat AnimationStats[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    struct stat MaterialStats[RESOURCE_MANAGER_RESOURCE_CAPACITY];

    model_hash_table           Models;
    texture_hash_table         Textures;
    animation_group_hash_table Animations;
    material_hash_table        Materials;

  public:
    path    ModelPaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    path    TexturePaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    path    AnimationPaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    path    MaterialPaths[RESOURCE_MANAGER_RESOURCE_CAPACITY];
    int32_t ModelPathCount;
    int32_t TexturePathCount;
    int32_t AnimationPathCount;
    int32_t MaterialPathCount;

    Memory::stack_allocator ModelStack;
    Memory::stack_allocator AnimationStack;
    Memory::stack_allocator MaterialStack;

  private:
    bool LoadModel(rid RID);
    bool LoadTexture(rid RID);
    bool LoadAnimation(rid RID);
    bool LoadMaterial(rid RID);

  public:
    rid RegisterModel(const char* Path);
    rid RegisterTexture(const char* Path);
    rid RegisterAnimation(const char* Path);
    rid RegisterMaterial(const char* Path);

    rid CreateMaterial(material Material, const char* Path);

    bool AsociateModel(rid RID, char* Path);
    bool AsociateTexture(rid RID, char* Path);
    bool AsociateAnimation(rid RID, char* Path);
    bool AsociateMaterial(rid RID, char* Path);

    bool GetModelPathRID(rid* RID, const char* Path);
    bool GetTexturePathRID(rid* RID, const char* Path);
    bool GetAnimationPathRID(rid* RID, const char* Path);
    bool GetMaterialPathRID(rid* RID, const char* Path);

    int32_t GetTexturePathIndex(rid RID);

    Render::model*   GetModel(rid RID);
    uint32_t         GetTexture(rid RID);
    Anim::animation* GetAnimation(rid RID);
    material*        GetMaterial(rid RID);

    void UpdateHardDriveAssetPathLists();
  };
}
