#pragma once

enum phong_flags
{
  PHONG_UseDiffuseMap  = 1,
  PHONG_UseSpecularMap = 2,
  PHONG_UseNormalMap   = 4,
  PHONG_UseSkeleton    = 8,
};

enum shader_type
{
  SHADER_Phong,
  SHADER_Color,

  SHADER_EnumCount,
};

union material {
  struct material_header
  {
    uint32_t ShaderType;
    bool     IsSkeletal;
    bool     UseBlending;
  } Common;

  struct
  {
    material_header Common;
    uint32_t        Flags;
    vec4            DiffuseColor;
    uint32_t        DiffuseMapIndex;
    uint32_t        SpecularMapIndex;
    uint32_t        NormalMapIndex;
    float           Shininess;
  } Phong;
  struct
  {
    material_header Common;
    vec4            Color;
  } Color;
};

const int32_t MESH_INSTANCE_MAX_COUNT = 10000;
const int32_t TEXTURE_MAX_COUNT       = 20;
const int32_t MATERIAL_MAX_COUNT      = 20;
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
  rid    Models[MODEL_MAX_COUNT];
  int32_t ModelCount;

  // Textures
  uint32_t     Textures[TEXTURE_MAX_COUNT];
  texture_name TextureNames[TEXTURE_MAX_COUNT];
  int32_t      TextureCount;

  // Shaders
  uint32_t ShaderPhong;
  uint32_t ShaderColor;
  uint32_t ShaderGizmo;
  uint32_t ShaderQuad;
  uint32_t ShaderTexturedQuad;
  uint32_t ShaderCubemap;
  uint32_t ShaderID;

  // Light
  vec3 LightPosition;
  bool ShowLightPosition;

  vec3 LightAmbientColor;
  vec3 LightDiffuseColor;
  vec3 LightSpecularColor;
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
  material Material           = {};
  Material.Common.ShaderType  = SHADER_Phong;
  Material.Phong.DiffuseColor = { 0.5f, 0.5f, 0.5f, 1 };
  Material.Phong.Shininess    = 60.0f;
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
AddModel(render_data* RenderData, rid ModelID)
{
  assert(0 <= RenderData->ModelCount && RenderData->ModelCount < MODEL_MAX_COUNT);
  RenderData->Models[RenderData->ModelCount++] = ModelID;
}
