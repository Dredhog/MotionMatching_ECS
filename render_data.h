#pragma once

#define FRAMEBUFFER_MAX_COUNT 3

#define BLUR_KERNEL_SIZE 11

#define SHADOW_WIDTH 2048
#define SHADOW_HEIGHT 2048
#define FIGHT_PETER_PAN 0

enum phong_flags
{
  PHONG_UseDiffuseMap  = 1,
  PHONG_UseSpecularMap = 2,
  PHONG_UseNormalMap   = 4,
  PHONG_UseSkeleton    = 8,
};

enum env_flags
{
  ENV_UseRefraction = 1 << 0,
  ENV_UseNormalMap  = 1 << 1,
  ENV_UseSkeleton   = 1 << 2,
};

enum shader_type
{
  SHADER_Phong,
  SHADER_Env,
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
  POST_DepthMap    = 1 << 3,
  POST_MotionBlur  = 1 << 4,

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
    uint32_t        Flags;
    rid             NormalMapID;
    float           RefractiveIndex;
  } Env;

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

struct cubemap
{
  char     Name[64];
  char     Format[64];
  rid      FaceIDs[6];
  uint32_t CubemapTexture;
};

#if 0
// TODO(rytis): Should we rather use this???
struct framebuffers
{
  uint32_t VAO;
  uint32_t VBO;
  uint32_t FBOs[FRAMEBUFFER_MAX_COUNT];
  uint32_t RBOs[FRAMEBUFFER_MAX_COUNT];
  uint32_t Textures[FRAMEBUFFER_MAX_COUNT];
  uint32_t CurrentFBO;
  uint32_t CurrentTexture;
};
#endif

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

  // Pre-Pass shader
  rid ShaderGeomPreePass;

  // Shaders
  rid ShaderPhong;
  rid ShaderEnv;
  rid ShaderColor;
  rid ShaderGizmo;
  rid ShaderQuad;
  rid ShaderTexturedQuad;
  rid ShaderCubemap;
  rid ShaderID;
  rid ShaderToon;
  rid ShaderTest;
  rid ShaderParallax;
  rid ShaderSimpleDepth;

  // Post-processing shaders
  rid PostDefaultShader;
  rid PostBlurH;
  rid PostBlurV;
  rid PostGrayscale;
  rid PostNightVision;
  rid PostDepthMap;
  rid PostDepthOfField;
  rid PostMotionBlur;

  cubemap Cubemap;

  int32_t PPEffects;

  float PostBlurLastStdDev;
  float PostBlurStdDev;
  float PostBlurKernel[BLUR_KERNEL_SIZE];

  // Geometry/Depth FrameBuffer
  uint32_t GBufferFBO;
  uint32_t GBufferVelocityTexID;
  uint32_t GBufferPositionTexID;
  uint32_t GBufferDepthTexID;

  // Depth Framebuffer for shadow mapping
  uint32_t DepthMapFBO;
  uint32_t DepthMapTexture;

  // Temporary stuff for post-processing
  // framebuffers Screen;
  uint32_t ScreenQuadVAO;
  uint32_t ScreenQuadVBO;
  uint32_t ScreenFBO[FRAMEBUFFER_MAX_COUNT];
  uint32_t ScreenRBO[FRAMEBUFFER_MAX_COUNT];
  uint32_t ScreenTexture[FRAMEBUFFER_MAX_COUNT];
  uint32_t CurrentFramebuffer;
  uint32_t CurrentTexture;

  // Light
  vec3 LightPosition;
  bool ShowLightPosition;

  mat4 LightVPMatrix;

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
NewEnvMaterial()
{
  material Material            = {};
  Material.Common.ShaderType   = SHADER_Env;
  Material.Env.RefractiveIndex = 1.52f;
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
