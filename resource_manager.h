#pragma once

#include "stack_alloc.h"
#include "heap_alloc.h"
#include "load_texture.h"
#include "asset.h"
#include "model.h"
#include "mesh.h"
#include "anim.h"
#include "render_data.h"
#include "text.h"
#include "rid.h"

#include "resource_hash_table.h"
static const int RESOURCE_MAX_COUNT = 200;

const int MODEL_MAX_COUNT     = RESOURCE_MAX_COUNT;
const int ANIMATION_MAX_COUNT = RESOURCE_MAX_COUNT;
const int MATERIAL_MAX_COUNT  = RESOURCE_MAX_COUNT;
const int TEXTURE_MAX_COUNT   = RESOURCE_MAX_COUNT;
const int SHADER_MAX_COUNT    = RESOURCE_MAX_COUNT;

namespace Resource
{
  typedef resource_hash_table<Render::model*, MODEL_MAX_COUNT>       model_hash_table;
  typedef resource_hash_table<Anim::animation*, ANIMATION_MAX_COUNT> animation_group_hash_table;
  typedef resource_hash_table<material*, MATERIAL_MAX_COUNT>         material_hash_table;
  typedef resource_hash_table<uint32_t, TEXTURE_MAX_COUNT>           texture_hash_table;
  typedef resource_hash_table<GLuint, SHADER_MAX_COUNT>              shader_hash_table;

  class resource_manager
  {
    asset_diff DiffedModels[RESOURCE_MAX_COUNT];
    asset_diff DiffedAnimations[RESOURCE_MAX_COUNT];
    asset_diff DiffedMaterials[RESOURCE_MAX_COUNT];
    asset_diff DiffedTextures[RESOURCE_MAX_COUNT];
    asset_diff DiffedShaders[RESOURCE_MAX_COUNT];

    int32_t DiffedModelCount;
    int32_t DiffedAnimationCount;
    int32_t DiffedMaterialCount;
    int32_t DiffedTextureCount;
    int32_t DiffedShaderCount;

    file_stat ModelStats[RESOURCE_MAX_COUNT];
    file_stat TextureStats[RESOURCE_MAX_COUNT];
    file_stat AnimationStats[RESOURCE_MAX_COUNT];
    file_stat MaterialStats[RESOURCE_MAX_COUNT];
    file_stat SceneStats[RESOURCE_MAX_COUNT];
    file_stat ShaderStats[RESOURCE_MAX_COUNT];

    bool LoadModel(rid RID);
    bool LoadTexture(rid RID);
    bool LoadAnimation(rid RID);
    bool LoadMaterial(rid RID);
    bool LoadShader(rid RID);

    void FreeModel(rid RID);
    void FreeAnimation(rid RID);
    void FreeShader(rid RID);

    GLuint DefaultShaderID;

  public:
    Memory::heap_allocator   ModelHeap;
    Memory::heap_allocator   AnimationHeap;
    Memory::stack_allocator  MaterialStack;
    Memory::stack_allocator* TemporaryStack;

    model_hash_table           Models;
    texture_hash_table         Textures;
    animation_group_hash_table Animations;
    material_hash_table        Materials;
    shader_hash_table          Shaders;

    path ModelPaths[RESOURCE_MAX_COUNT];
    path TexturePaths[RESOURCE_MAX_COUNT];
    path AnimationPaths[RESOURCE_MAX_COUNT];
    path MaterialPaths[RESOURCE_MAX_COUNT];
    path ShaderPaths[RESOURCE_MAX_COUNT];
    path ScenePaths[RESOURCE_MAX_COUNT];

    int32_t ModelPathCount;
    int32_t TexturePathCount;
    int32_t AnimationPathCount;
    int32_t MaterialPathCount;
    int32_t ShaderPathCount;
    int32_t ScenePathCount;

    rid CreateMaterial(material Material, const char* Path);

    rid RegisterModel(const char* Path);
    rid RegisterTexture(const char* Path);
    rid RegisterAnimation(const char* Path);
    rid RegisterMaterial(const char* Path);
    rid RegisterShader(const char* Path);

    bool AssociateModelIDToPath(rid RID, const char* Path);
    bool AssociateAnimationIDToPath(rid RID, const char* Path);
    bool AssociateTextureIDToPath(rid RID, const char* Path);
    bool AssociateMaterialIDToPath(rid RID, const char* Path);

    bool GetModelPathRID(rid* RID, const char* Path);
    bool GetTexturePathRID(rid* RID, const char* Path);
    bool GetAnimationPathRID(rid* RID, const char* Path);
    bool GetMaterialPathRID(rid* RID, const char* Path);

    int32_t GetTexturePathIndex(rid RID);
    int32_t GetMaterialPathIndex(rid RID);
    int32_t GetAnimationPathIndex(rid RID);

    Render::model*   GetModel(rid RID);
    uint32_t         GetTexture(rid RID);
    GLuint           GetShader(rid RID);
    Anim::animation* GetAnimation(rid RID);
    material*        GetMaterial(rid RID);

    void WipeAllModelData();
    void WipeAllMaterialData();
    void WipeAllAnimationData();
    void WipeAllTextureData();
    void WipeAllShaderData();

    void UpdateHardDriveAssetPathLists();
    void DeleteUnused();
    void ReloadModified();

    void Create(uint8_t* Memory, uint32_t TotalMemorySize, Memory::stack_allocator* TemporaryStack);
    void SetDefaultShaderID(GLuint ShaderID);
  };
}
