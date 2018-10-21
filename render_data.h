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

enum toon_flags
{
  TOON_UseDiffuseMap  = 1,
  TOON_UseSpecularMap = 2,
  TOON_UseNormalMap   = 4,
  TOON_UseSkeleton    = 8,
};

#define FOR_ALL_NAMES(DO_FUNC)                                                                     \
  DO_FUNC(Phong) DO_FUNC(Env) DO_FUNC(Toon) DO_FUNC(Test) DO_FUNC(Parallax) DO_FUNC(Color)
#define GENERATE_ENUM(Name) SHADER_##Name,
#define GENERATE_STRING(Name) #Name,
enum shader_type
{
  FOR_ALL_NAMES(GENERATE_ENUM) SHADER_EnumCount
};

static const char* g_ShaderTypeEnumStrings[SHADER_EnumCount] = { FOR_ALL_NAMES(GENERATE_STRING) };
#undef FOR_ALL_NAMES
#undef GENERATE_ENUM
#undef GENERATE_STRING

enum pp_type
{
  POST_Default      = 0,
  POST_Blur         = 1 << 0,
  POST_DepthOfField = 1 << 1,
  POST_Grayscale    = 1 << 2,
  POST_NightVision  = 1 << 3,
  POST_MotionBlur   = 1 << 4,

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
    uint32_t        Flags;
    vec3            AmbientColor;
    vec4            DiffuseColor;
    vec3            SpecularColor;
    rid             DiffuseMapID;
    rid             SpecularMapID;
    rid             NormalMapID;
    float           Shininess;
    int             LevelCount;
  } Toon;

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

const int32_t MESH_INSTANCE_MAX_COUNT  = 10000;
const int32_t SSAO_SAMPLE_VECTOR_COUNT = 9;

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

  // TODO(2-tuple) rename shader variables in a way that signifies them being resource handles
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
  rid ShaderSSAO;

  // Post-processing shaders
  rid PostDefaultShader;
  rid PostBlurH;
  rid PostBlurV;
  rid PostGrayscale;
  rid PostNightVision;
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
  uint32_t GBufferNormalTexID;
  uint32_t GBufferDepthTexID;

  // SSAO
  uint32_t SSAOFBO;
  uint32_t SSAOTexID;
  bool     RenderSSAO;
  vec3     SSAOSampleVectors[SSAO_SAMPLE_VECTOR_COUNT];
  float    SSAOSamplingRadius;

  // Depth Framebuffer for shadow mapping
  rid      RenderDepthMap;
  bool     DrawDepthMap;
  uint32_t DepthMapFBO;
  uint32_t DepthMapTexture;

  // TODO(Rytis by Lukas' recommendation :D) make array names plural
  // Temporary stuff for post-processing
  // framebuffers Screen;
  uint32_t ScreenQuadVAO;
  uint32_t ScreenQuadVBO;
  uint32_t ScreenFBO[FRAMEBUFFER_MAX_COUNT];
  uint32_t ScreenRBO[FRAMEBUFFER_MAX_COUNT];
  uint32_t ScreenTexture[FRAMEBUFFER_MAX_COUNT];
  uint32_t CurrentFramebuffer;
  uint32_t CurrentTexture;

  // Directional shadow mapping
  bool RealTimeDirectionalShadows;
  bool RecomputeDirectionalShadows;
  bool ClearDirectionalShadows;

  // Sun
  vec3  SunPosition;
  vec3  SunDirection;
  float SunNearClipPlane;
  float SunFarClipPlane;
  mat4  SunVPMatrix;
  bool  ShowSun;

  vec3 SunAmbientColor;
  vec3 SunDiffuseColor;
  vec3 SunSpecularColor;

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
