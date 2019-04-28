#include "ecs_management.h"
#include "component_table.h"
#include "component_table.h"
#include "misc.h"

#define CACHE_LINE_SIZE 64

void
InitializeECS(Memory::stack_allocator* PersistentMemStack, ecs_runtime** OutRuntime,
              ecs_world** OutWorld, size_t TotalECSMemorySize)
{
  assert(OutRuntime && OutWorld);
  int32_t ChunkMemorySize =
    (int32_t)(TotalECSMemorySize - (sizeof(ecs_runtime) + sizeof(ecs_world)));
  uint8_t*     ChunkMemory = PersistentMemStack->AlignedAlloc(ChunkMemorySize, CACHE_LINE_SIZE);
  ecs_runtime* Runtime     = PushStruct(PersistentMemStack, ecs_runtime);
  ecs_world*   World       = PushStruct(PersistentMemStack, ecs_world);

  InitializeChunkHeap(Runtime, ChunkMemory, ChunkMemorySize);
  InitializeArchetypeAndComponentTables(Runtime, g_ComponentStructInfoTable, g_ComponentNameTable,
                                        (int32_t)COMPONENT_Count);
  InitializeWorld(World, Runtime);

  *OutRuntime = Runtime;
  *OutWorld   = World;
}

void
PartitionMemoryInitAllocators(game_memory* GameMemory, game_state* GameState)
{
  TIMED_BLOCK(PartitionMemory);
  GameState->TemporaryMemStack =
    Memory::CreateStackAllocatorInPlace(GameMemory->TemporaryMemory,
                                        GameMemory->TemporaryMemorySize);
  assert(GameMemory->PersistentMemorySize > sizeof(game_state));

  uint32_t TotalSubsystemMemorySize = GameMemory->PersistentMemorySize - sizeof(game_state);
  uint32_t PersistentStackSize      = (uint32_t)((float)TotalSubsystemMemorySize * 0.3);
  uint8_t* PersistentStackStart     = (uint8_t*)GameMemory->PersistentMemory + sizeof(game_state);

  uint32_t ResourceMemorySize = TotalSubsystemMemorySize - PersistentStackSize;
  uint8_t* ResouceMemoryStart = PersistentStackStart + PersistentStackSize;

  GameState->PersistentMemStack =
    Memory::CreateStackAllocatorInPlace(PersistentStackStart, PersistentStackSize);
  GameState->Resources.Create(ResouceMemoryStart, ResourceMemorySize, GameState->TemporaryMemStack);
}

void
RegisterLoadInitialResources(game_state* GameState)
{
  TIMED_BLOCK(LoadInitialResources);
  GameState->MagicChecksum = 123456;

  // Register debug models
  {
    RegisterDebugModels(GameState);
  }

  // Load unmanaged textures (only used in ui)
  {
    GameState->CollapsedTextureID  = Texture::LoadTexture("./data/textures/collapsed.bmp");
    GameState->ExpandedTextureID   = Texture::LoadTexture("./data/textures/expanded.bmp");
    GameState->R.PostTestTextureID = Texture::LoadTexture("./data/textures/collapsed.bmp");
    assert(GameState->CollapsedTextureID);
    assert(GameState->ExpandedTextureID);
    assert(GameState->R.PostTestTextureID);
  }

  // Load fonts
  {
    GameState->Font = Text::LoadFont("data/UbuntuMono.ttf", 18, 1, 2);
  }

  // Load shaders
  {
    // TODO(2-tuple) Make mesh shader loading management automatic
    GameState->R.ShaderPhong   = GameState->Resources.RegisterShader("shaders/phong");
    GameState->R.ShaderEnv     = GameState->Resources.RegisterShader("shaders/environment");
    GameState->R.ShaderCubemap = GameState->Resources.RegisterShader("shaders/cubemap");
    GameState->R.ShaderGizmo   = GameState->Resources.RegisterShader("shaders/gizmo");
    GameState->R.ShaderQuad    = GameState->Resources.RegisterShader("shaders/debug_quad");
    GameState->R.ShaderColor   = GameState->Resources.RegisterShader("shaders/color");
    GameState->R.ShaderTexturedQuad =
      GameState->Resources.RegisterShader("shaders/debug_textured_quad");
    GameState->R.ShaderID       = GameState->Resources.RegisterShader("shaders/id");
    GameState->R.ShaderToon     = GameState->Resources.RegisterShader("shaders/toon");
    GameState->R.ShaderTest     = GameState->Resources.RegisterShader("shaders/test");
    GameState->R.ShaderParallax = GameState->Resources.RegisterShader("shaders/parallax");
    GameState->R.ShaderRimLight = GameState->Resources.RegisterShader("shaders/rim_lit_parallax");
    GameState->R.ShaderSSAO     = GameState->Resources.RegisterShader("shaders/ssao");
    GameState->R.ShaderWavy     = GameState->Resources.RegisterShader("shaders/wavy");
    GameState->R.ShaderVolumetricScattering =
      GameState->Resources.RegisterShader("shaders/volumetric_scattering");
    GameState->R.ShaderLightWave = GameState->Resources.RegisterShader("shaders/light_wave");

    GameState->R.PostDefaultShader = GameState->Resources.RegisterShader("shaders/post_default");
    GameState->R.PostGrayscale     = GameState->Resources.RegisterShader("shaders/post_grayscale");
    GameState->R.PostNightVision = GameState->Resources.RegisterShader("shaders/post_night_vision");
    GameState->R.PostBlurH = GameState->Resources.RegisterShader("shaders/post_blur_horizontal");
    GameState->R.PostBlurV = GameState->Resources.RegisterShader("shaders/post_blur_vertical");
    GameState->R.RenderDepthMap  = GameState->Resources.RegisterShader("shaders/render_depth_map");
    GameState->R.RenderShadowMap = GameState->Resources.RegisterShader("shaders/render_shadow_map");
    GameState->R.PostDepthOfField = GameState->Resources.RegisterShader("shaders/depth_of_field");
    GameState->R.PostMotionBlur   = GameState->Resources.RegisterShader("shaders/motion_blur");
    GameState->R.PostEdgeOutline  = GameState->Resources.RegisterShader("shaders/edge_outline");
    GameState->R.PostEdgeBlend    = GameState->Resources.RegisterShader("shaders/edge_blend");
    GameState->R.PostBrightRegion = GameState->Resources.RegisterShader("shaders/bright_region");
    GameState->R.PostBloomBlur    = GameState->Resources.RegisterShader("shaders/bloom_blur");
    GameState->R.PostBloomTonemap =
      GameState->Resources.RegisterShader("shaders/bloom_combine_tonemap");
    GameState->R.PostFXAA       = GameState->Resources.RegisterShader("shaders/fxaa");
    GameState->R.PostSimpleFog  = GameState->Resources.RegisterShader("shaders/post_simple_fog");
    GameState->R.PostNoise      = GameState->Resources.RegisterShader("shaders/post_noise");
    GameState->R.PostShaderTest = GameState->Resources.RegisterShader("shaders/post_test");

    GameState->R.ShaderGeomPreePass = GameState->Resources.RegisterShader("shaders/geom_pre_pass");
    GameState->R.ShaderSunDepth     = GameState->Resources.RegisterShader("shaders/sun_depth");

    GLuint MissingShaderID =
      Shader::CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "shaders/missing_default");
    assert(MissingShaderID);
    GameState->Resources.SetDefaultShaderID(MissingShaderID);
  }

  // Set up shader parameter definitions (can be made automatic by parsing associated shader)
  {
    // TODO(2-tuple) adhere to a common, more appropriate shader variable naming standard
    // TODO(Lukas) rename all variables with "coord" to "coords"
    {
      struct shader_def* EnvShaderDef =
        AddShaderDef(SHADER_Env, GameState->R.ShaderEnv, "environment");
      AddParamDef(EnvShaderDef, "flags", "flags",
                  { SHADER_PARAM_TYPE_Int, offsetof(material, Env.Flags) });
      AddParamDef(EnvShaderDef, "map_normal", "normalMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Env.NormalMapID) });
      AddParamDef(EnvShaderDef, "refractive_index", "refractive_index",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, Env.RefractiveIndex) });
    }

    {
      struct shader_def* ToonShaderDef = AddShaderDef(SHADER_Toon, GameState->R.ShaderToon, "toon");
      AddParamDef(ToonShaderDef, "vec3_ambient", "material.ambientColor",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, Toon.AmbientColor) });
      AddParamDef(ToonShaderDef, "vec4_diffuse", "material.diffuseColor",
                  { SHADER_PARAM_TYPE_Vec4, offsetof(material, Toon.DiffuseColor) });
      AddParamDef(ToonShaderDef, "vec3_specular", "material.specularColor",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, Toon.SpecularColor) });
      AddParamDef(ToonShaderDef, "shininess", "material.shininess",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, Toon.Shininess) });
      AddParamDef(ToonShaderDef, "level_count", "levelCount",
                  { SHADER_PARAM_TYPE_Int, offsetof(material, Toon.LevelCount) });
    }

    {
      struct shader_def* TestShaderDef = AddShaderDef(SHADER_Test, GameState->R.ShaderTest, "test");
      AddParamDef(TestShaderDef, "sth_float", "uniform_float",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, Test.Float) });
      AddParamDef(TestShaderDef, "sth_int", "uniform_int",
                  { SHADER_PARAM_TYPE_Int, offsetof(material, Test.Int) });
      AddParamDef(TestShaderDef, "sth_vec3", "uniform_vec3",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, Test.Vec3) });
      AddParamDef(TestShaderDef, "map_diffuse", "material.diffuseMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Test.DiffuseID) });
      AddParamDef(TestShaderDef, "map_normal", "material.normalMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Test.NormalID) });
      AddParamDef(TestShaderDef, "map_specular", "material.specularMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Test.SpecularID) });
    }

    {
      struct shader_def* ParallaxShaderDef =
        AddShaderDef(SHADER_Parallax, GameState->R.ShaderParallax, "parallax");
      AddParamDef(ParallaxShaderDef, "height_scale", "height_scale",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, Parallax.HeightScale) });
      AddParamDef(ParallaxShaderDef, "map_diffuse", "material.diffuseMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Parallax.DiffuseID) });
      AddParamDef(ParallaxShaderDef, "map_normal", "material.normalMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Parallax.NormalID) });
      AddParamDef(ParallaxShaderDef, "map_depth", "material.depthMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Parallax.DepthID) });
    }
    {
      struct shader_def* RimShaderDef =
        AddShaderDef(SHADER_RimLight, GameState->R.ShaderRimLight, "rim_light");
      AddParamDef(RimShaderDef, "rim_strength", "material.rimStrength",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, RimLight.RimStrength) });
      AddParamDef(RimShaderDef, "rim_color", "material.rimColor",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, RimLight.RimColor) });
      AddParamDef(RimShaderDef, "height_scale", "height_scale",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, RimLight.HeightScale) });
      AddParamDef(RimShaderDef, "map_diffuse", "material.diffuseMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, RimLight.DiffuseID) });
      AddParamDef(RimShaderDef, "map_normal", "material.normalMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, RimLight.NormalID) });
      AddParamDef(RimShaderDef, "map_depth", "material.depthMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, RimLight.DepthID) });
    }

    {
      struct shader_def* WavyShaderDef = AddShaderDef(SHADER_Wavy, GameState->R.ShaderWavy, "Wavy");
      AddParamDef(WavyShaderDef, "height_scale", "u_HeightScale",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, Wavy.HeightScale) });
      AddParamDef(WavyShaderDef, "time_frequency", "u_TimeFrequency",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, Wavy.TimeFrequency) });
      AddParamDef(WavyShaderDef, "Frequency", "u_Frequency",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, Wavy.Frequency) });
      AddParamDef(WavyShaderDef, "Phase", "u_Phase",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, Wavy.Phase) });
      AddParamDef(WavyShaderDef, "albedo_color", "u_AlbedoColor",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, Wavy.AlbedoColor) });
      AddParamDef(WavyShaderDef, "map_normal", "u_NormalMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Wavy.NormalID) });
    }

    {
      struct shader_def* LightWaveShaderDef =
        AddShaderDef(SHADER_LightWave, GameState->R.ShaderLightWave, "LightWave");
      AddParamDef(LightWaveShaderDef, "vec3_ambient", "material.ambientColor",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, LightWave.AmbientColor) });
      AddParamDef(LightWaveShaderDef, "vec4_diffuse", "material.diffuseColor",
                  { SHADER_PARAM_TYPE_Vec4, offsetof(material, LightWave.DiffuseColor) });
      AddParamDef(LightWaveShaderDef, "vec3_specular", "material.specularColor",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, LightWave.SpecularColor) });
      AddParamDef(LightWaveShaderDef, "shininess", "material.shininess",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, LightWave.Shininess) });
    }
  }

  // TODO(2-tuple) Move all buffer creation code to the same place. Reuse buffers more effectively
  // TODO(Lukas) Make a designated helper function for versitile and safe frame buffer creation
  if(true) // Conditional used to preserve clang format indentation when using nested scopes
  {
    // FRAME BUFFER CREATION FOR ENTITY SELECTION
    {
      glGenFramebuffers(1, &GameState->IndexFBO);
      glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
      glGenTextures(1, &GameState->IDTexture);
      glBindTexture(GL_TEXTURE_2D, GameState->IDTexture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA,
                   GL_UNSIGNED_INT, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glBindTexture(GL_TEXTURE_2D, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             GameState->IDTexture, 0);
      glGenRenderbuffers(1, &GameState->DepthRBO);
      glBindRenderbuffer(GL_RENDERBUFFER, GameState->DepthRBO);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCREEN_WIDTH, SCREEN_HEIGHT);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                GameState->DepthRBO);
      glBindRenderbuffer(GL_RENDERBUFFER, 0);
      if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      {
        assert(0 && "error: incomplete framebuffer!\n");
      }
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // FRAME BUFFER CREATION FOR GEOMETRY/DEPTH PRE PASS
    {
      GenerateGeometryDepthFrameBuffer(&GameState->R.GBufferFBO, &GameState->R.GBufferPositionTexID,
                                       &GameState->R.GBufferVelocityTexID,
                                       &GameState->R.GBufferNormalTexID,
                                       &GameState->R.GBufferDepthTexID);
    }

    // FRAME BUFFER CREATION FOR SSAO
    {
      GenerateSSAOFrameBuffer(&GameState->R.SSAOFBO, &GameState->R.SSAOTexID);
    }

    // FRAMEBUFFER CREATION FOR DEPTH BUFFER RENDERING AND EDGE OUTLINE
    {
      GameState->R.DrawDepthBuffer = false;
      GenerateFramebuffer(&GameState->R.DepthTextureFBO, &GameState->R.DepthTextureRBO,
                          &GameState->R.DepthTexture, 2 * SCREEN_WIDTH, 2 * SCREEN_HEIGHT);
      GenerateFramebuffer(&GameState->R.EdgeOutlineFBO, &GameState->R.EdgeOutlineRBO,
                          &GameState->R.EdgeOutlineTexture);
    }

    // FRAMEBUFFER CREATION FOR SHADOW MAPPING
    {
      GameState->R.DrawShadowMap = false;
      for(int i = 0; i < ArrayCount(GameState->R.ShadowMapFBOs); i++)
      {
        GenerateShadowFramebuffer(&GameState->R.ShadowMapFBOs[i],
                                  &GameState->R.ShadowMapTextures[i]);
      }
    }

    // FRAMEBUFFER FOR VOLUMETRIC LIGHT SCATTERING
    {
      GenerateSSAOFrameBuffer(&GameState->R.LightScatterFBOs[0],
                              &GameState->R.LightScatterTextures[0], SCREEN_WIDTH, SCREEN_HEIGHT);
      GenerateSSAOFrameBuffer(&GameState->R.LightScatterFBOs[1],
                              &GameState->R.LightScatterTextures[1], SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    // FRAMEBUFFERS FOR HDF/BLOOM
    {

      GenerateFloatingPointFBO(&GameState->R.HdrFBOs[0], &GameState->R.HdrTextures[0],
                               &GameState->R.HdrRBOs[0]);
      GenerateFloatingPointFBO(&GameState->R.HdrFBOs[1], &GameState->R.HdrTextures[1],
                               &GameState->R.HdrRBOs[1]);
      GenerateFloatingPointFBO(&GameState->R.HdrFBOs[2], &GameState->R.HdrTextures[2],
                               &GameState->R.HdrRBOs[2]);
    }

    // FRAMEBUFFER GENERATION FOR POST-PROCESSING EFFECTS
    {
      GenerateScreenQuad(&GameState->R.ScreenQuadVAO, &GameState->R.ScreenQuadVBO);

      for(uint32_t i = 0; i < FRAMEBUFFER_MAX_COUNT; ++i)
      {
        GenerateFramebuffer(&GameState->R.ScreenFBOs[i], &GameState->R.ScreenRBOs[i],
                            &GameState->R.ScreenTextures[i]);
      }

      GameState->R.CurrentFramebuffer = 0;
      GameState->R.CurrentTexture     = 0;

      // Default blur parameters
      GameState->R.PostBlurLastStdDev = 10.0f;
      GameState->R.PostBlurStdDev     = GameState->R.PostBlurLastStdDev;
      GenerateGaussianBlurKernel(GameState->R.PostBlurKernel, BLUR_KERNEL_SIZE,
                                 GameState->R.PostBlurStdDev);
    }
  }

  // SET MISC GL STATE
  {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
  }
}

void
SetGameStatePODFields(game_state* GameState)
{
  GameState->SelectionMode = SELECT_Mesh;

  // CAMERA FIELD INITIALIZATION
  {
    GameState->Camera.Position      = { 0, 2, 0 };
    GameState->Camera.Up            = { 0, 1, 0 };
    GameState->Camera.Forward       = { 0, 0, -1 };
    GameState->Camera.Right         = { 1, 0, 0 };
    GameState->Camera.Rotation      = { 0 };
    GameState->Camera.NearClipPlane = 0.01f;
    GameState->Camera.FarClipPlane  = 1000.0f;
    GameState->Camera.FieldOfView   = 90.0f;
    GameState->Camera.MaxTiltAngle  = 90.0f;
    GameState->Camera.Speed         = 5.0f;
  }

  {
    GameState->PreviewCamera          = GameState->Camera;
    GameState->PreviewCamera.Position = { 0, 0, 2.2f };
    GameState->PreviewCamera.Rotation = {};
    UpdateCameraDerivedFields(&GameState->PreviewCamera);
  }

  // Post Processing
  {
    GameState->R.PPEffects |= POST_FXAA;
  }

  {
    // SUN DATA
    {
      GameState->R.Sun.RotationZ                   = 75;
      GameState->R.Sun.CurrentCascadeIndex         = 0;
      GameState->R.RealTimeDirectionalShadows      = true;
      GameState->R.RecomputeDirectionalShadows     = false;
      GameState->R.ClearDirectionalShadows         = false;
      GameState->R.Sun.CascadeFarPlaneDistances[0] = 3;
      GameState->R.Sun.CascadeFarPlaneDistances[1] = 8;
      GameState->R.Sun.CascadeFarPlaneDistances[2] = 20;
      GameState->R.Sun.CascadeFarPlaneDistances[3] = 40;

      GameState->R.Sun.AmbientColor = { 0.3f, 0.3f, 0.3f };
      GameState->R.Sun.DiffuseColor = { 0.7f, 0.7f, 0.7f };
    }

    GameState->R.LightPosition        = { 0.7f, 1, 1 };
    GameState->R.PreviewLightPosition = { 0.7f, 0, 2 };

    GameState->R.LightSpecularColor = { 0.5f, 0.5f, 0.5f };
    GameState->R.LightDiffuseColor  = { 0.5f, 0.5f, 0.5f };
    GameState->R.LightAmbientColor  = { 0.2f, 0.2f, 0.2f };
    GameState->R.ShowLightPosition  = false;

    GameState->R.GizmoScaleFactor = 0.13f;

    // SSAO
    {
      GameState->R.RenderSSAO         = false;
      GameState->R.SSAOSamplingRadius = 0.05f;
#if 0
			// hemispherical vector kernel initialization
			/*
			for(int i = 0; i < SSAO_SAMPLE_VECTOR_COUNT; i++)
			{
				GameState->R.SSAOSampleVectors[i] = vec3{()};
			}
			*/
#else
      assert(SSAO_SAMPLE_VECTOR_COUNT == 9);
      GameState->R.SSAOSampleVectors[0] = vec3{ 0, 0, 1 };
      GameState->R.SSAOSampleVectors[1] = vec3{ -1, 0, 1 };
      GameState->R.SSAOSampleVectors[2] = vec3{ 1, 0, 1 };
      GameState->R.SSAOSampleVectors[3] = vec3{ 0, 1, 1 };
      GameState->R.SSAOSampleVectors[4] = vec3{ 0, -1, 1 };
      GameState->R.SSAOSampleVectors[5] = vec3{ 1, 1, 1 };
      GameState->R.SSAOSampleVectors[6] = vec3{ 1, -1, 1 };
      GameState->R.SSAOSampleVectors[7] = vec3{ -1, 1, 1 };
      GameState->R.SSAOSampleVectors[8] = vec3{ -1, -1, 1 };
      for(int i = 0; i < SSAO_SAMPLE_VECTOR_COUNT; i++)
      {
        GameState->R.SSAOSampleVectors[i] = Math::Normalized(GameState->R.SSAOSampleVectors[i]);
      }
#endif
    }
    GameState->R.ExposureHDR             = 1.84f;
    GameState->R.BloomLuminanceThreshold = 1.2f;
    GameState->R.BloomBlurIterationCount = 1;

    // Initial fog values
    {
      GameState->R.FogDensity  = 0.1f;
      GameState->R.FogGradient = 3.0f;
      GameState->R.FogColor    = 0.5f;
    }
  }

  // Trajectory system
  {
    GameState->TrajectorySystem.Splines.HardClear();
    GameState->TrajectorySystem.IsWaypointPlacementMode = false;
    GameState->TrajectorySystem.SelectedSplineIndex     = -1;
    GameState->TrajectorySystem.SelectedWaypointIndex   = -1;
  }

  GameState->UseHotReloading = true;
  GameState->UpdatePathList  = false;

  GameState->DrawCubemap              = true;
  GameState->DrawDebugSpheres         = true;
  GameState->DrawTimeline             = true;
  GameState->DrawGizmos               = true;
  GameState->DrawDebugLines           = true;
  GameState->DrawDebugSpheres         = true;
  GameState->DrawActorMeshes          = true;
  GameState->DrawActorSkeletons       = true;
  GameState->DrawShadowCascadeVolumes = false;
  GameState->BoneSphereRadius         = 0.01f;

  GameState->PreviewAnimationsInRootSpace = false;
  GameState->IsAnimationPlaying = false;
  GameState->CurrentMaterialID  = { 0 };
  //GameState->PlayerEntityIndex  = -1;

  // Animation Editor
  GameState->AnimEditor.EntityIndex = -1;

  // Motion Matching
  {
    GameState->MMDebug                       = {};
    GameState->MMDebug.TrajectoryDuration    = 1;
    GameState->MMDebug.TrajectorySampleCount = 20;
    GameState->MMDebug.ApplyRootMotion       = true;
    GameState->MMDebug.ShowRootTrajectories  = false;
    GameState->MMDebug.ShowHipTrajectories   = false;

    GameState->MMDebug.CurrentGoal.ShowTrajectory       = true;
    GameState->MMDebug.MatchedGoal.ShowTrajectory       = true;
    GameState->MMDebug.MatchedGoal.ShowTrajectoryAngles = false;
    GameState->MMDebug.MatchedGoal.ShowTrajectoryAngles = false;
    GameState->MMDebug.CurrentGoal.ShowBonePositions    = true;
    GameState->MMDebug.MatchedGoal.ShowBonePositions    = true;
  }

  GameState->Physics.Params.Beta                       = (1.0f / (FRAME_TIME_MS / 1000.0f)) / 2.0f;
  GameState->Physics.Params.Mu                         = 1.0f;
  GameState->Physics.Params.PGSIterationCount          = 50;
  GameState->Physics.Switches.UseGravity               = true;
  GameState->Physics.Switches.VisualizeOmega           = false;
  GameState->Physics.Switches.VisualizeV               = false;
  GameState->Physics.Switches.VisualizeFriction        = false;
  GameState->Physics.Switches.VisualizeFc              = false;
  GameState->Physics.Switches.VisualizeContactPoints   = false;
  GameState->Physics.Switches.VisualizeContactManifold = false;
  GameState->Physics.Switches.SimulateDynamics         = false;
  GameState->Physics.Switches.SimulateFriction         = true;
}
