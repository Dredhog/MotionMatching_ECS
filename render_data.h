#pragma once

#define BLUR_KERNEL_SIZE 11

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
  SHADER_Test,
  SHADER_Parallax,
  SHADER_Color,

  SHADER_EnumCount,
};

enum pp_type
{
  POST_Default     = 0,
  POST_Blur        = 1 << 0,
  POST_Grayscale   = 1 << 1,
  POST_NightVision = 1 << 2,

  POST_EnumCount,
};

union material {
  struct material_header
  {
    int32_t ShaderType;
    bool    IsSkeletal;
    bool    UseBlending;
  } Common;

  struct
  {
    material_header Common;
    uint32_t        Flags;
    vec3            AmbientColor;
    vec4            DiffuseColor;
    vec3            SpecularColor;
    rid             DiffuseMapID;
    rid             SpecularMapID;
    rid             NormalMapID;
    float           Shininess;
  } Phong;

  struct
  {
    material_header Common;
    vec4            Color;
  } Color;

  struct
  {
    material_header Common;
    vec3            Vec3;
    int32_t         Int;
    float           Float;
    rid             NormalID;
    rid             SpecularID;
    rid             DiffuseID;
  } Test;

  struct
  {
    material_header Common;
    float           HeightScale;
    rid             DiffuseID;
    rid             NormalID;
    rid             DepthID;
  } Parallax;
};

const int32_t MESH_INSTANCE_MAX_COUNT = 10000;

struct mesh_instance
{
  Render::mesh* Mesh;
  material*     Material;
  int32_t       EntityIndex;
};

struct render_data
{
  mesh_instance MeshInstances[MESH_INSTANCE_MAX_COUNT]; // Filled every frame
  int32_t       MeshInstanceCount;

  // Shaders
  rid ShaderPhong;
  rid ShaderColor;
  rid ShaderGizmo;
  rid ShaderQuad;
  rid ShaderTexturedQuad;
  rid ShaderCubemap;
  rid ShaderID;
  rid ShaderToon;
  rid ShaderTest;
  rid ShaderParallax;

  // Post-processing shaders
  rid PostDefaultShader;
  rid PostBlurH;
  rid PostBlurV;
  rid PostGrayscale;
  rid PostNightVision;

  int32_t PPEffects;

  float PostBlurLastStdDev;
  float PostBlurStdDev;
  float PostBlurKernel[BLUR_KERNEL_SIZE];

  // Light
  vec3 LightPosition;
  bool ShowLightPosition;

  vec3 PreviewLightPosition;

  vec3 LightAmbientColor;
  vec3 LightDiffuseColor;
  vec3 LightSpecularColor;
};

inline material
NewPhongMaterial()
{
  material Material            = {};
  Material.Common.ShaderType   = SHADER_Phong;
  Material.Phong.AmbientColor  = { 0.5f, 0.5f, 0.5f };
  Material.Phong.DiffuseColor  = { 0.5f, 0.5f, 0.5f, 1.0f };
  Material.Phong.SpecularColor = { 1.0f, 1.0f, 1.0f };
  Material.Phong.Shininess     = 60.0f;
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

inline void
AddMeshInstance(render_data* RenderData, mesh_instance MeshInstance)
{
  assert(0 <= RenderData->MeshInstanceCount &&
         RenderData->MeshInstanceCount < MESH_INSTANCE_MAX_COUNT);
  RenderData->MeshInstances[RenderData->MeshInstanceCount++] = MeshInstance;
}
