#pragma once

enum material_type
{
  MATERIAL_Color    = 1,
  MATERIAL_Diffuse  = 2,
  MATERIAL_Specular = 4,
  MATERIAL_Normal   = 8,
};

enum shader_type
{
  SHADER_MaterialPhong,
  SHADER_Phong,
  SHADER_Color,
  //  SHADER_ID,

  SHADER_EnumCount,
};

#if 0
enum texture_type
{
  TEXTURE_Diffuse,
  TEXTURE_Specular,
  TEXTURE_Normal,

  TEXTURE_EnumCount,
};
#endif

union material {
  // material_header;
  struct material_header
  {
    uint32_t ShaderType;
    bool     UseBlending;
  } Common;

  struct
  {
    material_header Common;
    uint32_t        DiffuseMapIndex;
    float           AmbientStrength;
    float           SpecularStrength;
    bool            UseBlinn;
  } Phong;
  struct
  {
    material_header Common;
    int32_t         Flags;
    uint32_t        DiffuseMapIndex;
    uint32_t        SpecularMapIndex;
    uint32_t        NormalMapIndex;
    float           AmbientStrength;
    float           SpecularStrength;
    float           Shininess;
  } MaterialPhong;
  struct
  {
    material_header Common;
    vec4            Color;
  } Color;
};

const int32_t TEXTURE_MAX_COUNT       = 20;
const int32_t MATERIAL_MAX_COUNT      = 20;
const int32_t MESH_INSTANCE_MAX_COUNT = 10000;
const int32_t MODEL_MAX_COUNT         = 10;
const int32_t TEXTURE_NAME_MAX_LENGTH = 50;

struct mesh_instance
{
  Render::mesh* Mesh;
  material*     Material;
  int32_t       EntityIndex;
};

struct texture_name
{
  char Name[TEXTURE_NAME_MAX_LENGTH];
};

struct render_data
{
  // Materials
  material      Materials[MATERIAL_MAX_COUNT];
  int32_t       MaterialCount;
  mesh_instance MeshInstances[MESH_INSTANCE_MAX_COUNT]; // Filled every frame
  int32_t       MeshInstanceCount;

  // Models
  Render::model* Models[MODEL_MAX_COUNT];
  int32_t        ModelCount;

  // Textures
  int32_t      Textures[TEXTURE_MAX_COUNT];
  texture_name TextureNames[TEXTURE_MAX_COUNT];
  int32_t      TextureCount;

  // Shaders
  uint32_t ShaderPhong;
  uint32_t ShaderMaterialPhong;
  uint32_t ShaderSkeletalPhong;
  uint32_t ShaderSkeletalBoneColor;
  uint32_t ShaderColor;
  uint32_t ShaderGizmo;
  uint32_t ShaderQuad;
  uint32_t ShaderTexturedQuad;
  uint32_t ShaderCubemap;
  uint32_t ShaderID;

  // Light
  vec3 LightPosition;
  //-----Testing Purposes-----
  vec3 LightAmbientColor;
  vec3 LightDiffuseColor;
  vec3 LightSpecularColor;
  //--------------------------
  vec3 LightColor;
};

inline uint32_t
AddTexture(render_data* RenderData, uint32_t TextureID, char* TextureName)
{
  assert(TextureID);
  assert(0 <= RenderData->TextureCount && RenderData->TextureCount < TEXTURE_MAX_COUNT);
  int32_t NameLength = (int32_t)strlen(TextureName);
  assert(NameLength < TEXTURE_NAME_MAX_LENGTH);
  strcpy(RenderData->TextureNames[RenderData->TextureCount].Name, TextureName);
  RenderData->Textures[RenderData->TextureCount++] = TextureID;
  return TextureID;
}

inline material
NewPhongMaterial()
{
  material Material               = {};
  Material.Common.ShaderType      = SHADER_Phong;
  Material.Common.UseBlending     = true;
  Material.Phong.DiffuseMapIndex  = 0;
  Material.Phong.AmbientStrength  = 0.8f;
  Material.Phong.SpecularStrength = 0.6f;
  Material.Phong.UseBlinn         = true;
  return Material;
}

inline material
NewMaterialPhongMaterial()
{
  material Material                       = {};
  Material.Common.ShaderType              = SHADER_MaterialPhong;
  Material.Common.UseBlending             = true;
  Material.MaterialPhong.Flags            = 0;
  Material.MaterialPhong.DiffuseMapIndex  = 0;
  Material.MaterialPhong.SpecularMapIndex = 0;
  Material.MaterialPhong.DiffuseMapIndex  = 0;
  Material.MaterialPhong.AmbientStrength  = 0;
  Material.MaterialPhong.SpecularStrength = 0;
  Material.MaterialPhong.Shininess        = 0;
  return Material;
}

inline material
NewColorMaterial()
{
  material Material           = {};
  Material.Common.ShaderType  = SHADER_Color;
  Material.Common.UseBlending = true;
  Material.Color.Color        = vec4{ 1, 1, 0, 1 };
  return Material;
}

inline int32_t
AddMaterial(render_data* RenderData, material Material)
{
  assert(0 <= RenderData->MaterialCount && RenderData->MaterialCount < MATERIAL_MAX_COUNT);
  int32_t LastMaterialIndex = RenderData->MaterialCount;

  RenderData->Materials[RenderData->MaterialCount++] = Material;
  return LastMaterialIndex;
}

inline void
AddMeshInstance(render_data* RenderData, mesh_instance MeshInstance)
{
  assert(0 <= RenderData->MeshInstanceCount &&
         RenderData->MeshInstanceCount < MESH_INSTANCE_MAX_COUNT);
  RenderData->MeshInstances[RenderData->MeshInstanceCount++] = MeshInstance;
}

inline void
AddModel(render_data* RenderData, Render::model* Model)
{
  assert(0 <= RenderData->ModelCount && RenderData->ModelCount < MODEL_MAX_COUNT);
  RenderData->Models[RenderData->ModelCount++] = Model;
}
