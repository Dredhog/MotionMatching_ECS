#pragma once

#define FRAMEBUFFER_MAX_COUNT 4
#define HDR_FRAMEBUFFER_MAX_COUNT 4

#define BLUR_KERNEL_SIZE 11

#define SHADOWMAP_CASCADE_COUNT 3
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

#define FOR_ALL_NAMES(DO_FUNC)                                                                     \
  DO_FUNC(Phong)                                                                                   \
  DO_FUNC(Env) DO_FUNC(Toon)                                                                       \
  DO_FUNC(Test) DO_FUNC(Parallax) DO_FUNC(Color) DO_FUNC(Wavy) DO_FUNC(RimLight) DO_FUNC(LightWave)
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
  POST_FXAA         = 1 << 5,
  POST_HDRTonemap   = 1 << 6,
  POST_Bloom        = 1 << 7,
  POST_EdgeOutline  = 1 << 8,
  POST_SimpleFog    = 1 << 9,
  POST_Noise        = 1 << 10,
	POST_Test         = 1 << 11,

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
    vec3            AmbientColor;
    vec4            DiffuseColor;
    vec3            SpecularColor;
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

  struct
  {
    material_header Common;
    vec3            RimColor;
    float           RimStrength;
    float           HeightScale;
    rid             DiffuseID;
    rid             NormalID;
    rid             DepthID;
  } RimLight;

  struct
  {
    material_header Common;
    float           HeightScale;
    float           TimeFrequency;
    vec3            Phase;
    vec3            Frequency;
    vec3            AlbedoColor;
    rid             NormalID;
  } Wavy;

  struct
  {
    material_header Common;
    vec3            AmbientColor;
    vec4            DiffuseColor;
    vec3            SpecularColor;
    float           Shininess;
  } LightWave;
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

struct box_mesh
{
	vec3 Points[8];
};

struct frustum_def
{
	vec3 Forward;
	vec3 Right;
	vec3 Up;
	vec3 Origin;
	float Near;
	float Far;
	float ViewAngle;
	float Aspect;
};

struct obb_def
{
	vec3 Forward;
	vec3 Right;
	vec3 Up;
	vec3 NearCenter;
	float NearFarDist;
	float HalfWidth;
	float HalfHeight;
};

struct sun
{
	int CurrentCascadeIndex;

  float  RotationY;
  float  RotationZ;
  vec3  Direction;

	mat4 CascadeVP[SHADOWMAP_CASCADE_COUNT];
	float CascadeFarPlaneDistances[SHADOWMAP_CASCADE_COUNT];

  vec3 AmbientColor;
  vec3 DiffuseColor;
};

//obb horizontals are always parallel to XZ plane
inline obb_def GetFrustumOBBFromDir(vec3 LightDir, box_mesh Frustum)
{
	vec3 Forward = Math::Normalized(LightDir);
	vec3 Right = Math::Normalized(Math::Cross(LightDir, vec3{0, 1, 0}));
	vec3 Up = Math::Cross(Right, Forward);

	vec3 MinRight = Frustum.Points[0];
	vec3 MaxRight = Frustum.Points[0];
	vec3 MinUp = Frustum.Points[0];
	vec3 MaxUp = Frustum.Points[0];
	vec3 MinForward = Frustum.Points[0];
	vec3 MaxForward = Frustum.Points[0];
	for(int i = 1; i < 8; i++)
	{
		vec3 CurrentPoint = Frustum.Points[i];
		float ProjRight = Math::Dot(CurrentPoint, Right);
		float ProjUp = Math::Dot(CurrentPoint, Up);
		float ProjForward = Math::Dot(CurrentPoint, Forward);

		//Right
		if(ProjRight < Math::Dot(MinRight, Right))
		{
			MinRight = CurrentPoint;
		}
		if(Math::Dot(MaxRight, Right) < ProjRight)
		{
			MaxRight = CurrentPoint;
		}

		//Up
		if(ProjUp < Math::Dot(MinUp, Up))
		{
			MinUp = CurrentPoint;
		}
		if(Math::Dot(MaxUp, Up) < ProjUp)
		{
			MaxUp = CurrentPoint;
		}

		//Forward
		if(ProjForward < Math::Dot(MinForward, Forward))
		{
			MinForward = CurrentPoint;
		}
		if(Math::Dot(MaxForward, Forward) < ProjForward)
		{
			MaxForward = CurrentPoint;
		}
	}
	float MaxProjRight = Math::Dot(MaxRight, Right);
	float MinProjRight = Math::Dot(MinRight, Right);
	float MaxProjUp = Math::Dot(MaxUp, Up);
	float MinProjUp = Math::Dot(MinUp, Up);
	float MaxProjForward = Math::Dot(MaxForward, Forward);
	float MinProjForward = Math::Dot(MinForward, Forward);

	obb_def Result = {};
	Result.Forward = Forward;
	Result.Right = Right;
	Result.Up = Up;
	Result.NearCenter = Right * ((MinProjRight + MaxProjRight)/2.0f) +
											   Up * ((MinProjUp + MaxProjUp)/2.0f) + 
				  				  Forward * MinProjForward;
	Result.NearFarDist = MaxProjForward - MinProjForward;
	Result.HalfWidth = (MaxProjRight - MinProjRight)/2.0f;
	Result.HalfHeight = (MaxProjUp - MinProjUp)/2.0f;

	return Result;
}

inline box_mesh GetFrustumBoxMesh(frustum_def Frustum)
{
	float near = Frustum.Near;
	float far = Frustum.Far;
	float tan_fov_over_2 = tanf((3.14159f / 180.0f) * (Frustum.ViewAngle/2.0f));
	float hw_near = tan_fov_over_2 * near;
	float hw_far  = tan_fov_over_2 * far;
	float hh_near = hw_near / Frustum.Aspect;
	float hh_far  = hw_far / Frustum.Aspect;
	box_mesh Result= {vec3{-hw_near, -hh_near,  near}, vec3{hw_near, -hh_near,  near}, vec3{hw_near, hh_near,  near}, vec3{-hw_near, hh_near,  near},
	                  vec3{-hw_far, -hh_far, far}, vec3{hw_far, -hh_far, far}, vec3{hw_far, hh_far, far}, vec3{-hw_far, hh_far, far}};
  mat4 ObjectToWorld = Math::MulMat4(Math::Mat4Translate(Frustum.Origin), Math::Mat3ToMat4(Math::Mat3Basis(Frustum.Right, Frustum.Up, Frustum.Forward)));

	for(int i = 0; i < 8; i++)
	{
		vec3 CurrentPoint = Result.Points[i];
		vec4 CurrentPointHomog;
		CurrentPointHomog.XYZ = CurrentPoint;
		CurrentPointHomog.W = 1.0f;
		CurrentPointHomog = Math::MulMat4Vec4(ObjectToWorld, CurrentPointHomog);

		Result.Points[i] = CurrentPointHomog.XYZ;
	}

	return Result;
	return Result;
}

inline box_mesh GetOBBBoxMesh(obb_def OBB)
{
	float hw = OBB.HalfWidth;
	float hh = OBB.HalfHeight;
	float depth = OBB.NearFarDist;
	box_mesh Result= {vec3{-hw, -hh,     0}, vec3{hw, -hh,     0}, vec3{hw, hh,     0}, vec3{-hw, hh,     0},
	                  vec3{-hw, -hh, depth}, vec3{hw, -hh, depth}, vec3{hw, hh, depth}, vec3{-hw, hh, depth}};
  mat4 ObjectToWorld = Math::MulMat4(Math::Mat4Translate(OBB.NearCenter), Math::Mat3ToMat4(Math::Mat3Basis(OBB.Right, OBB.Up, OBB.Forward)));

	for(int i = 0; i < 8; i++)
	{
		vec3 CurrentPoint = Result.Points[i];
		vec4 CurrentPointHomog;
		CurrentPointHomog.XYZ = CurrentPoint;
		CurrentPointHomog.W = 1.0f;
		CurrentPointHomog = Math::MulMat4Vec4(ObjectToWorld, CurrentPointHomog);

		Result.Points[i] = CurrentPointHomog.XYZ;
	}

	return Result;
}

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
  rid ShaderRimLight;
  rid ShaderSunDepth;
  rid ShaderSSAO;
  rid ShaderWavy;
  rid ShaderLightWave;
  rid ShaderVolumetricScattering;

  // Post-processing shaders
  rid PostDefaultShader;
  rid PostBlurH;
  rid PostBlurV;
  rid PostGrayscale;
  rid PostNightVision;
  rid PostDepthOfField;
  rid PostMotionBlur;
  rid PostEdgeOutline;
  rid PostEdgeBlend;
  rid PostBrightRegion;
  rid PostBloomBlur;
  rid PostBloomTonemap;
  rid PostFXAA;
  rid PostSimpleFog;
  rid PostNoise;
  rid PostShaderTest;

  cubemap Cubemap;

  int32_t PPEffects;

  float PostBlurLastStdDev;
  float PostBlurStdDev;
  float PostBlurKernel[BLUR_KERNEL_SIZE];

  float FogDensity;
  float FogGradient;
  float FogColor;

  // Geometry/Depth FrameBuffer
  uint32_t GBufferFBO;
  uint32_t GBufferVelocityTexID;
  uint32_t GBufferPositionTexID;
  uint32_t GBufferNormalTexID;
  uint32_t GBufferDepthTexID;

  float CumulativeTime;

  // Volumetric Light Scattering framebuffer
  bool     RenderVolumetricScattering;
  uint32_t LightScatterFBOs[2];
  uint32_t LightScatterTextures[2];

  // HDR tonemapping
  float ExposureHDR;

  // Bloom
  float   BloomLuminanceThreshold;
  int32_t BloomBlurIterationCount;

  // SSAO
  uint32_t SSAOFBO;
  uint32_t SSAOTexID;
  bool     RenderSSAO;
  vec3     SSAOSampleVectors[SSAO_SAMPLE_VECTOR_COUNT];
  float    SSAOSamplingRadius;

  rid RenderDepthMap;
  rid RenderShadowMap;

  // Screen space depth buffer
  bool     DrawDepthBuffer;
  uint32_t DepthTextureFBO;
  uint32_t DepthTextureRBO;
  uint32_t DepthTexture;
  uint32_t EdgeOutlineFBO;
  uint32_t EdgeOutlineRBO;
  uint32_t EdgeOutlineTexture;

  // Depth Framebuffer for shadow mapping
  bool     DrawShadowMap;
  uint32_t ShadowMapFBOs[SHADOWMAP_CASCADE_COUNT];
  uint32_t ShadowMapTextures[SHADOWMAP_CASCADE_COUNT];

	// Post processing additional input texture
	int32_t PostTestTextureID;

  // Temporary stuff for post-processing
  // framebuffers Screen;
  uint32_t ScreenQuadVAO;
  uint32_t ScreenQuadVBO;
  uint32_t ScreenFBOs[FRAMEBUFFER_MAX_COUNT];
  uint32_t ScreenRBOs[FRAMEBUFFER_MAX_COUNT];
  uint32_t ScreenTextures[FRAMEBUFFER_MAX_COUNT];
  uint32_t CurrentFramebuffer;
  uint32_t CurrentTexture;

  // HDR framebuffers
  uint32_t HdrFBOs[HDR_FRAMEBUFFER_MAX_COUNT];
  uint32_t HdrTextures[HDR_FRAMEBUFFER_MAX_COUNT];
  uint32_t HdrRBOs[HDR_FRAMEBUFFER_MAX_COUNT];

  // Directional shadow mapping
  bool  RealTimeDirectionalShadows;
  bool  RecomputeDirectionalShadows;
  bool  ClearDirectionalShadows;

  // Sun
  sun Sun;

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
