#include "load_shader.h"

#include "linear_math/matrix.h"
#include "linear_math/vector.h"
#include "linear_math/distribution.h"

#include "game.h"
#include "mesh.h"
#include "model.h"
#include "asset.h"
#include "load_texture.h"
#include "misc.h"
#include "intersection_testing.h"
#include "render_data.h"
#include "material_upload.h"
#include "material_io.h"
#include "collision_testing.h"

#include "text.h"

#include "debug_drawing.h"
#include "camera.h"
#include "player_controller.h"

#include "gui_testing.h"
#include "dynamics.h"
#include "shader_def.h"

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  game_state* GameState = (game_state*)GameMemory.PersistentMemory;
  assert(GameMemory.HasBeenInitialized);

  // GAME STATE INITIALIZATION (ONLY RUN ON FIRST FRAME)
  if(GameState->MagicChecksum != 123456)
  {
    GameState->MagicChecksum = 123456;

    // INITIALIZE MEMORY
    {
      GameState->TemporaryMemStack =
        Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory,
                                            GameMemory.TemporaryMemorySize);
      assert(GameMemory.PersistentMemorySize > sizeof(game_state));

      uint32_t AvailableSubsystemMemory = GameMemory.PersistentMemorySize - sizeof(game_state);
      uint32_t PersistentStackSize      = (uint32_t)((float)AvailableSubsystemMemory * 0.3);
      uint8_t* PersistentStackStart = (uint8_t*)GameMemory.PersistentMemory + sizeof(game_state);

      uint32_t ResourceMemorySize = AvailableSubsystemMemory - PersistentStackSize;
      uint8_t* ResouceMemoryStart = PersistentStackStart + PersistentStackSize;

      GameState->PersistentMemStack =
        Memory::CreateStackAllocatorInPlace(PersistentStackStart, PersistentStackSize);
      GameState->Resources.Create(ResouceMemoryStart, ResourceMemorySize,
                                  GameState->TemporaryMemStack);
    }

    // REGISTER DEBUG MODELS
    {
      RegisterDebugModels(GameState);
    }

    // LOAD UNMANAGED TEXTURES (ONLY USED IN UI)
    {
      GameState->CollapsedTextureID = Texture::LoadTexture("./data/textures/collapsed.bmp");
      GameState->ExpandedTextureID  = Texture::LoadTexture("./data/textures/expanded.bmp");
      assert(GameState->CollapsedTextureID);
      assert(GameState->ExpandedTextureID);
    }

    // LOAD FONTS
    {
      GameState->Font = Text::LoadFont("data/UbuntuMono.ttf", 18, 1, 2);
    }

    // HAND LOAD SHADERS
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

      GameState->R.PostDefaultShader = GameState->Resources.RegisterShader("shaders/post_default");
      GameState->R.PostGrayscale = GameState->Resources.RegisterShader("shaders/post_grayscale");
      GameState->R.PostNightVision =
        GameState->Resources.RegisterShader("shaders/post_night_vision");
      GameState->R.PostBlurH = GameState->Resources.RegisterShader("shaders/post_blur_horizontal");
      GameState->R.PostBlurV = GameState->Resources.RegisterShader("shaders/post_blur_vertical");
      GameState->R.RenderDepthMap = GameState->Resources.RegisterShader("shaders/render_depth_map");
      GameState->R.PostDepthOfField = GameState->Resources.RegisterShader("shaders/depth_of_field");
      GameState->R.PostMotionBlur   = GameState->Resources.RegisterShader("shaders/motion_blur");

      GameState->R.ShaderGeomPreePass =
        GameState->Resources.RegisterShader("shaders/geom_pre_pass");
      GameState->R.ShaderSimpleDepth = GameState->Resources.RegisterShader("shaders/simple_depth");

      GLuint MissingShaderID = Shader::CheckedLoadCompileFreeShader(GameState->TemporaryMemStack,
                                                                    "shaders/missing_default");
      assert(MissingShaderID);
      GameState->Resources.SetDefaultShaderID(MissingShaderID);
    }

    // SET UP SHADER PARAMETER DEFINITIONS (CAN BE MADE AUTOMATIC BY PARSING ASSOCIATED SHADER)
    {
      struct shader_def* EnvShaderDef =
        AddShaderDef(SHADER_Env, GameState->R.ShaderEnv, "environment");
      AddParamDef(EnvShaderDef, "flags", "flags",
                  { SHADER_PARAM_TYPE_Int, offsetof(material, Env.Flags) });
      AddParamDef(EnvShaderDef, "map_normal", "normalMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Env.NormalMapID) });
      AddParamDef(EnvShaderDef, "refractive_index", "refractive_index",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, Env.RefractiveIndex) });

      struct shader_def* ToonShaderDef =
        AddShaderDef(SHADER_Toon, GameState->R.ShaderToon, "toon");
      AddParamDef(ToonShaderDef, "flags", "flags",
                  { SHADER_PARAM_TYPE_Int, offsetof(material, Toon.Flags) });
      AddParamDef(ToonShaderDef, "vec3_ambient", "material.ambientColor",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, Toon.AmbientColor) });
      AddParamDef(ToonShaderDef, "vec4_diffuse", "material.diffuseColor",
                  { SHADER_PARAM_TYPE_Vec4, offsetof(material, Toon.DiffuseColor) });
      AddParamDef(ToonShaderDef, "vec3_specular", "material.specularColor",
                  { SHADER_PARAM_TYPE_Vec3, offsetof(material, Toon.SpecularColor) });
      AddParamDef(ToonShaderDef, "map_diffuse", "material.diffuseMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Toon.DiffuseMapID) });
      AddParamDef(ToonShaderDef, "map_specular", "material.specularMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Toon.SpecularMapID) });
      AddParamDef(ToonShaderDef, "map_normal", "material.normalMap",
                  { SHADER_PARAM_TYPE_Map, offsetof(material, Toon.NormalMapID) });
      AddParamDef(ToonShaderDef, "shininess", "material.shininess",
                  { SHADER_PARAM_TYPE_Float, offsetof(material, Toon.Shininess) });
      AddParamDef(ToonShaderDef, "level_count", "levelCount",
                  { SHADER_PARAM_TYPE_Int, offsetof(material, Toon.LevelCount) });

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
                                       &GameState->R.GBufferDepthTexID);
    }

    // FRAMEBUFFER CREATION FOR SHADOW MAPPING
    {
      GameState->R.DrawDepthMap = false;
      GenerateDepthFramebuffer(&GameState->R.DepthMapFBO, &GameState->R.DepthMapTexture);
    }

    // FRAMEBUFFER GENERATION FOR POST-PROCESSING EFFECTS
    {
      GenerateScreenQuad(&GameState->R.ScreenQuadVAO, &GameState->R.ScreenQuadVBO);

      for(uint32_t i = 0; i < FRAMEBUFFER_MAX_COUNT; ++i)
      {
        GenerateFramebuffer(&GameState->R.ScreenFBO[i], &GameState->R.ScreenRBO[i],
                            &GameState->R.ScreenTexture[i]);
      }

      GameState->R.CurrentFramebuffer = 0;
      GameState->R.CurrentTexture     = 0;

      // Default blur parameters
      GameState->R.PostBlurLastStdDev = 10.0f;
      GameState->R.PostBlurStdDev     = GameState->R.PostBlurLastStdDev;
      GenerateGaussianBlurKernel(GameState->R.PostBlurKernel, BLUR_KERNEL_SIZE,
                                 GameState->R.PostBlurStdDev);
    }

    // SET MISC GL STATE
    {
      glEnable(GL_DEPTH_TEST);
      glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
      glEnable(GL_CULL_FACE);
      glDepthFunc(GL_LEQUAL);
    }

    // GAME STATE FIELD INITIALIZATION
    {
      // CAMERA FIELD INITIALIZATION
      {
        GameState->Camera.Position      = { 0, 1.6f, 2 };
        GameState->Camera.Up            = { 0, 1, 0 };
        GameState->Camera.Forward       = { 0, 0, -1 };
        GameState->Camera.Right         = { 1, 0, 0 };
        GameState->Camera.Rotation      = { 0 };
        GameState->Camera.NearClipPlane = 0.01f;
        GameState->Camera.FarClipPlane  = 1000.0f;
        GameState->Camera.FieldOfView   = 70.0f;
        GameState->Camera.MaxTiltAngle  = 90.0f;
        GameState->Camera.Speed         = 2.0f;
      }

      GameState->PreviewCamera          = GameState->Camera;
      GameState->PreviewCamera.Position = { 0, 0, 2 };
      GameState->PreviewCamera.Rotation = {};
      UpdateCamera(&GameState->PreviewCamera, Input);

      // SHADOW MAPPING INITIALIZATION
      {
        GameState->R.RealTimeDirectionalShadows  = true;
        GameState->R.RecomputeDirectionalShadows = false;
        GameState->R.ClearDirectionalShadows     = false;

        GameState->R.SunPosition      = { 0.0f, 10.0f, -10.0f };
        GameState->R.SunDirection     = -GameState->R.SunPosition;
        GameState->R.SunNearClipPlane = 0.01f;
        GameState->R.SunFarClipPlane  = 50.0f;
        UpdateSun(&GameState->R.SunVPMatrix, &GameState->R.SunDirection, GameState->R.SunPosition,
                  GameState->R.SunNearClipPlane, GameState->R.SunFarClipPlane);
        GameState->R.ShowSun = false;

        GameState->R.SunAmbientColor = { 0.3f, 0.3f, 0.3f };
        GameState->R.SunDiffuseColor = { 0.0f, 0.0f, 0.0f };
        GameState->R.SunSpecularColor = { 0.0f, 0.0f, 0.0f };
      }

      GameState->R.LightPosition        = { 0.7f, 1, 1 };
      GameState->R.PreviewLightPosition = { 0.7f, 0, 2 };

      GameState->R.LightSpecularColor = { 1, 1, 1 };
      GameState->R.LightDiffuseColor  = { 1, 1, 1 };
      GameState->R.LightAmbientColor  = { 0.3f, 0.3f, 0.3f };
      GameState->R.ShowLightPosition  = false;

      GameState->DrawCubemap      = true;
      GameState->DrawDebugSpheres = true;
      GameState->DrawTimeline     = true;
      GameState->DrawGizmos       = true;

      GameState->IsAnimationPlaying      = false;
      GameState->EditorBoneRotationSpeed = 45.0f;
      GameState->CurrentMaterialID       = { 0 };
      GameState->PlayerEntityIndex       = -1;

      GameState->Restitution              = 0.5f;
      GameState->Beta                     = (1.0f / (FRAME_TIME_MS / 1000.0f)) / 2.0f;
      GameState->Slop                     = 0.1f;
      GameState->Mu                       = 1.0f;
      GameState->PGSIterationCount        = 10;
      GameState->UseGravity               = true;
      GameState->VisualizeOmega           = false;
      GameState->VisualizeV               = false;
      GameState->VisualizeFriction        = false;
      GameState->VisualizeFc              = false;
      GameState->VisualizeContactPoints   = false;
      GameState->VisualizeContactManifold = false;
      GameState->SimulateDynamics         = false;
      GameState->SimulateFriction         = false;
    }

    SetUpCubeHull(&g_CubeHull);
  }
  //---------------------END INIT -------------------------

  GameState->Resources.UpdateHardDriveAssetPathLists();
  GameState->Resources.DeleteUnused();
  GameState->Resources.ReloadModified();

  if(GameState->CurrentMaterialID.Value > 0 && GameState->Resources.MaterialPathCount <= 0)
  {
    GameState->CurrentMaterialID = {};
  }
  if(GameState->CurrentMaterialID.Value <= 0)
  {
    if(GameState->Resources.MaterialPathCount > 0)
    {
      GameState->CurrentMaterialID =
        GameState->Resources.RegisterMaterial(GameState->Resources.MaterialPaths[0].Name);
    }
    else
    {
      GameState->CurrentMaterialID =
        GameState->Resources.CreateMaterial(NewPhongMaterial(), "data/materials/default.mat");
    }
  }

  if(Input->IsMouseInEditorMode)
  {
    // GUI
    UI::TestGui(GameState, Input);

    /* // ANIMATION TIMELINE
    if(GameState->SelectionMode == SELECT_Bone && GameState->DrawTimeline &&
       GameState->AnimEditor.Skeleton)
    {
      VisualizeTimeline(GameState);
    }
    */

    // TODO(Lukas) Fix selection swapping bug,
    // Recreation steps:
    // Create sponza, enter mesh selection mode, create multimesh soldier, and two cubes,
    // then select the fisrt cube and continue clicking on sponza
    // Selection changes between first cube andsponza
    // Selection
    if(Input->MouseRight.EndedDown && Input->MouseRight.Changed)
    {
      // Draw entities to ID buffer
      // SORT_MESH_INSTANCES(ByEntity);
      glDisable(GL_BLEND);
      glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
      glClearColor(0.9f, 0.9f, 0.9f, 1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      GLuint EntityIDShaderID = GameState->Resources.GetShader(GameState->R.ShaderID);
      glUseProgram(EntityIDShaderID);
      for(int e = 0; e < GameState->EntityCount; e++)
      {
        entity* CurrentEntity;
        assert(GetEntityAtIndex(GameState, &CurrentEntity, e));
        glUniformMatrix4fv(glGetUniformLocation(EntityIDShaderID, "mat_mvp"), 1, GL_FALSE,
                           GetEntityMVPMatrix(GameState, e).e);
        if(CurrentEntity->AnimController)
        {
          glUniformMatrix4fv(glGetUniformLocation(EntityIDShaderID, "g_boneMatrices"),
                             CurrentEntity->AnimController->Skeleton->BoneCount, GL_FALSE,
                             (float*)CurrentEntity->AnimController->HierarchicalModelSpaceMatrices);
        }
        else
        {
          mat4 Mat4Zeros = {};
          glUniformMatrix4fv(glGetUniformLocation(EntityIDShaderID, "g_boneMatrices"), 1, GL_FALSE,
                             Mat4Zeros.e);
        }
        if(GameState->SelectionMode == SELECT_Mesh)
        {
          Render::mesh* SelectedMesh = {};
          if(GetSelectedMesh(GameState, &SelectedMesh))
          {
            glBindVertexArray(SelectedMesh->VAO);

            glDrawElements(GL_TRIANGLES, SelectedMesh->IndiceCount, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          }
        }
        Render::model* CurrentModel = GameState->Resources.GetModel(GameState->Entities[e].ModelID);
        for(int m = 0; m < CurrentModel->MeshCount; m++)
        {
          glBindVertexArray(CurrentModel->Meshes[m]->VAO);
          assert(e < USHRT_MAX);
          assert(m < USHRT_MAX);
          uint16_t EntityID = (uint16_t)e;
          uint16_t MeshID   = (uint16_t)m;
          uint32_t R        = (EntityID & 0x00FF) >> 0;
          uint32_t G        = (EntityID & 0xFF00) >> 8;
          uint32_t B        = (MeshID & 0x00FF) >> 0;
          uint32_t A        = (MeshID & 0xFF00) >> 8;

          vec4 EntityColorID = { (float)R / 255.0f, (float)G / 255.0f, (float)B / 255.0f,
                                 (float)A / 255.0f };
          glUniform4fv(glGetUniformLocation(EntityIDShaderID, "g_id"), 1, (float*)&EntityColorID);
          glDrawElements(GL_TRIANGLES, CurrentModel->Meshes[m]->IndiceCount, GL_UNSIGNED_INT, 0);
        }
      }
      glFlush();
      glFinish();
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      uint16_t IDColor[2] = {};
      glReadPixels(Input->MouseX, Input->MouseY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, IDColor);
      GameState->SelectedEntityIndex = (uint32_t)IDColor[0];
      GameState->SelectedMeshIndex   = (uint32_t)IDColor[1];
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Entity creation
    if(GameState->IsEntityCreationMode && Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      GameState->IsEntityCreationMode = false;
      vec3 RayDir =
        GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                             GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
      raycast_result RaycastResult =
        RayIntersectPlane(GameState->Camera.Position, RayDir, {}, { 0, 1, 0 });
      if(RaycastResult.Success && GameState->CurrentModelID.Value != 0)
      {
        Anim::transform NewTransform = {};
        NewTransform.Translation     = RaycastResult.IntersectP;
        NewTransform.Scale           = { 1, 1, 1 };

        Render::model* Model = GameState->Resources.GetModel(GameState->CurrentModelID);
        rid* MaterialIDs     = PushArray(GameState->PersistentMemStack, Model->MeshCount, rid);
        if(GameState->CurrentMaterialID.Value > 0)
        {
          for(int m = 0; m < Model->MeshCount; m++)
          {
            MaterialIDs[m] = GameState->CurrentMaterialID;
          }
        }
        AddEntity(GameState, GameState->CurrentModelID, MaterialIDs, NewTransform);
      }
    }
  }

  //----------------------UPDATE------------------------
  UpdateCamera(&GameState->Camera, Input);
  UpdateSun(&GameState->R.SunVPMatrix, &GameState->R.SunDirection, GameState->R.SunPosition,
            GameState->R.SunNearClipPlane, GameState->R.SunFarClipPlane);

  // Dynamics
  g_Force                 = GameState->Force;
  g_ForceStart            = GameState->ForceStart;
  g_ApplyingForce         = GameState->ApplyingForce;
  g_ApplyingTorque        = GameState->ApplyingTorque;
  g_UseGravity            = GameState->UseGravity;
  g_Bias                  = GameState->Beta;
  g_Mu                    = GameState->Mu;
  g_VisualizeFc           = GameState->VisualizeFc;
  g_VisualizeFcComponents = GameState->VisualizeFcComponents;
  g_VisualizeFriction     = GameState->VisualizeFriction;

  // Colision
  g_VisualizeContactPoints   = GameState->VisualizeContactPoints;
  g_VisualizeContactManifold = GameState->VisualizeContactManifold;

  for(int i = 0; i < GameState->EntityCount; i++)
  {
    GameState->Entities[i].RigidBody.q =
      Math::EulerToQuat(GameState->Entities[i].Transform.Rotation);
    GameState->Entities[i].RigidBody.X = GameState->Entities[i].Transform.Translation;

    const rigid_body& RB = GameState->Entities[i].RigidBody;
    if(GameState->VisualizeOmega)
    {
      Debug::PushLine(RB.X, RB.X + RB.w, { 0, 1, 0, 1 });
      Debug::PushWireframeSphere(RB.X + RB.w, 0.05f, { 0, 1, 0, 1 });
    }
    if(GameState->VisualizeV)
    {
      Debug::PushLine(RB.X, RB.X + RB.v, { 1, 1, 0, 1 });
      Debug::PushWireframeSphere(RB.X + RB.v, 0.05f, { 1, 1, 0, 1 });
    }
  }
  SimulateDynamics(GameState);

  if(GameState->PlayerEntityIndex != -1)
  {
    entity* PlayerEntity = {};
    if(GetEntityAtIndex(GameState, &PlayerEntity, GameState->PlayerEntityIndex))
    {
      Gameplay::UpdatePlayer(PlayerEntity, Input);
    }
  }

  if(GameState->R.ShowLightPosition)
  {
    mat4 Mat4LightPosition = Math::Mat4Translate(GameState->R.LightPosition);
    Debug::PushGizmo(&GameState->Camera, &Mat4LightPosition);
  }

  if(GameState->R.ShowSun)
  {
    mat4 Mat4SunPosition = Math::Mat4Translate(GameState->R.SunPosition);
    Debug::PushGizmo(&GameState->Camera, &Mat4SunPosition);
    Debug::PushLine(GameState->R.SunPosition, GameState->R.SunPosition + GameState->R.SunDirection);
    Debug::PushWireframeSphere(GameState->R.SunPosition + GameState->R.SunDirection, 0.05f);
  }

  // -----------ENTITY ANIMATION UPDATE-------------
  for(int e = 0; e < GameState->EntityCount; e++)
  {
    Anim::animation_controller* Controller = GameState->Entities[e].AnimController;
    if(Controller)
    {
      for(int i = 0; i < Controller->AnimStateCount; i++)
      {
        assert(Controller->AnimationIDs[i].Value > 0);
        Controller->Animations[i] = GameState->Resources.GetAnimation(Controller->AnimationIDs[i]);
      }
      Anim::UpdateController(Controller, Input->dt, Controller->BlendFunc);
    }
  }

  //----------ANIMATION EDITOR INTERACTION-----------
  if(Input->IsMouseInEditorMode && GameState->SelectionMode == SELECT_Bone &&
     GameState->AnimEditor.Skeleton)
  {
    if(Input->Space.EndedDown && Input->Space.Changed)
    {
      GameState->IsAnimationPlaying = !GameState->IsAnimationPlaying;
    }
    if(GameState->IsAnimationPlaying)
    {
      EditAnimation::PlayAnimation(&GameState->AnimEditor, Input->dt);
    }
    if(Input->i.EndedDown && Input->i.Changed)
    {
      InsertBlendedKeyframeAtTime(&GameState->AnimEditor, GameState->AnimEditor.PlayHeadTime);
    }
    if(Input->LeftShift.EndedDown)
    {
      if(Input->n.EndedDown && Input->n.Changed)
      {
        EditAnimation::EditPreviousBone(&GameState->AnimEditor);
      }
      if(Input->ArrowLeft.EndedDown && Input->ArrowLeft.Changed)
      {
        EditAnimation::JumpToPreviousKeyframe(&GameState->AnimEditor);
      }
      if(Input->ArrowRight.EndedDown && Input->ArrowRight.Changed)
      {
        EditAnimation::JumpToNextKeyframe(&GameState->AnimEditor);
      }
    }
    else
    {
      if(Input->n.EndedDown && Input->n.Changed)
      {
        EditAnimation::EditNextBone(&GameState->AnimEditor);
      }
      if(Input->ArrowLeft.EndedDown)
      {
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, -1 * Input->dt);
      }
      if(Input->ArrowRight.EndedDown)
      {
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, 1 * Input->dt);
      }
    }
    if(Input->LeftCtrl.EndedDown)
    {
      if(Input->c.EndedDown && Input->c.Changed)
      {
        EditAnimation::CopyKeyframeToClipboard(&GameState->AnimEditor,
                                               GameState->AnimEditor.CurrentKeyframe);
      }
      else if(Input->x.EndedDown && Input->x.Changed)
      {
        EditAnimation::CopyKeyframeToClipboard(&GameState->AnimEditor,
                                               GameState->AnimEditor.CurrentKeyframe);
        DeleteCurrentKeyframe(&GameState->AnimEditor);
      }
      else if(Input->v.EndedDown && Input->v.Changed && GameState->AnimEditor.Skeleton)
      {
        EditAnimation::InsertKeyframeFromClipboardAtTime(&GameState->AnimEditor,
                                                         GameState->AnimEditor.PlayHeadTime);
      }
    }
    if(Input->Delete.EndedDown && Input->Delete.Changed)
    {
      EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
    }
    if(GameState->AnimEditor.KeyframeCount > 0)
    {
      EditAnimation::CalculateHierarchicalmatricesAtTime(&GameState->AnimEditor);
    }

    float CurrentlySelectedDistance = INFINITY;
    // Bone Selection
    for(int i = 0; i < GameState->AnimEditor.Skeleton->BoneCount; i++)
    {
      mat4 Mat4Bone =
        Math::MulMat4(TransformToMat4(GameState->AnimEditor.Transform),
                      Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices[i],
                                    GameState->AnimEditor.Skeleton->Bones[i].BindPose));

      const float BoneSphereRadius = 0.1f;

      vec3 Position = Math::GetMat4Translation(Mat4Bone);
      vec3 RayDir =
        GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                             GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
      raycast_result RaycastResult =
        RayIntersectSphere(GameState->Camera.Position, RayDir, Position, BoneSphereRadius);
      if(RaycastResult.Success)
      {
        Debug::PushWireframeSphere(Position, BoneSphereRadius, { 1, 1, 0, 1 });
        float DistanceToIntersection =
          Math::Length(RaycastResult.IntersectP - GameState->Camera.Position);

        if(Input->MouseRight.EndedDown && Input->MouseRight.Changed &&
           DistanceToIntersection < CurrentlySelectedDistance)
        {
          EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, i);
          CurrentlySelectedDistance = DistanceToIntersection;
        }
      }
      else
      {
        Debug::PushWireframeSphere(Position, BoneSphereRadius);
      }
    }
    if(GameState->AnimEditor.Skeleton)
    {
      mat4 Mat4Bone =
        Math::MulMat4(TransformToMat4(GameState->AnimEditor.Transform),
                      Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices
                                      [GameState->AnimEditor.CurrentBone],
                                    GameState->AnimEditor.Skeleton
                                      ->Bones[GameState->AnimEditor.CurrentBone]
                                      .BindPose));
      Debug::PushGizmo(&GameState->Camera, &Mat4Bone);
    }
    // Copy editor poses to entity anim controller
    assert(0 <= GameState->AnimEditor.EntityIndex &&
           GameState->AnimEditor.EntityIndex < GameState->EntityCount);
    {
      memcpy(GameState->Entities[GameState->AnimEditor.EntityIndex]
               .AnimController->HierarchicalModelSpaceMatrices,
             GameState->AnimEditor.HierarchicalModelSpaceMatrices,
             sizeof(mat4) * GameState->AnimEditor.Skeleton->BoneCount);
    }
  }
  //---------------------RENDERING----------------------------

  // RENDER QUEUE SUBMISSION
  GameState->R.MeshInstanceCount = 0;
  for(int e = 0; e < GameState->EntityCount; e++)
  {
    Render::model* CurrentModel = GameState->Resources.GetModel(GameState->Entities[e].ModelID);
    for(int m = 0; m < CurrentModel->MeshCount; m++)
    {
      mesh_instance MeshInstance = {};
      MeshInstance.Mesh          = CurrentModel->Meshes[m];
      MeshInstance.Material =
        GameState->Resources.GetMaterial(GameState->Entities[e].MaterialIDs[m]);
      MeshInstance.EntityIndex = e;
      AddMeshInstance(&GameState->R, MeshInstance);
    }
  }

  // GEOMETRY/DEPTH PRE-PASS
  {
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.GBufferFBO);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO(Lukas) SORT(MeshInstances,  ByMesh);
    {
      Render::mesh* PreviousMesh = nullptr;
      uint32_t      GeomPrePassShaderID =
        GameState->Resources.GetShader(GameState->R.ShaderGeomPreePass);
      glUseProgram(GeomPrePassShaderID);
      for(int i = 0; i < GameState->R.MeshInstanceCount; i++)
      {
        Render::mesh* CurrentMesh        = GameState->R.MeshInstances[i].Mesh;
        int           CurrentEntityIndex = GameState->R.MeshInstances[i].EntityIndex;

        if(CurrentMesh != PreviousMesh)
        {
          glBindVertexArray(CurrentMesh->VAO);
          PreviousMesh = CurrentMesh;
        }

        // TODO(Lukas) Add logic for bone matrix submission

        glUniformMatrix4fv(glGetUniformLocation(GeomPrePassShaderID, "mat_mvp"), 1, GL_FALSE,
                           GetEntityMVPMatrix(GameState, CurrentEntityIndex).e);
        glUniformMatrix4fv(glGetUniformLocation(GeomPrePassShaderID, "mat_prev_mvp"), 1, GL_FALSE,
                           GameState->PrevFrameMVPMatrices[CurrentEntityIndex].e);
        glUniformMatrix4fv(glGetUniformLocation(GeomPrePassShaderID, "mat_model"), 1, GL_FALSE,
                           GetEntityModelMatrix(GameState, CurrentEntityIndex).e);
        glUniformMatrix4fv(glGetUniformLocation(GeomPrePassShaderID, "mat_view"), 1, GL_FALSE,
                           GameState->Camera.ViewMatrix.e);
        glDrawElements(GL_TRIANGLES, CurrentMesh->IndiceCount, GL_UNSIGNED_INT, 0);
      }
      glBindVertexArray(0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // Saving previous frame entity MVP matrix (USED ONLY FOR MOTION BLUR)
  {
    for(int e = 0; e < GameState->EntityCount; e++)
    {
      GameState->PrevFrameMVPMatrices[e] = GetEntityMVPMatrix(GameState, e);
    }
  }

  // DEPTH MAP PASS
  if(GameState->R.RealTimeDirectionalShadows || GameState->R.RecomputeDirectionalShadows)
  {
    uint32_t SimpleDepthShaderID = GameState->Resources.GetShader(GameState->R.ShaderSimpleDepth);
    glUseProgram(SimpleDepthShaderID);
    glUniformMatrix4fv(glGetUniformLocation(SimpleDepthShaderID, "mat_sun_vp"), 1, GL_FALSE,
                       GameState->R.SunVPMatrix.e);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.DepthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
#if FIGHT_PETER_PAN
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
#endif

    // TODO(Lukas) SORT(MeshInstances,  ByMesh);
    {
      Render::mesh* PreviousMesh = nullptr;
      for(int i = 0; i < GameState->R.MeshInstanceCount; i++)
      {
        Render::mesh* CurrentMesh        = GameState->R.MeshInstances[i].Mesh;
        int           CurrentEntityIndex = GameState->R.MeshInstances[i].EntityIndex;

        if(CurrentMesh != PreviousMesh)
        {
          glBindVertexArray(CurrentMesh->VAO);
          PreviousMesh = CurrentMesh;
        }

        // TODO(Lukas) Add logic for bone matrix submission

        glUniformMatrix4fv(glGetUniformLocation(SimpleDepthShaderID, "mat_model"), 1, GL_FALSE,
                           GetEntityModelMatrix(GameState, CurrentEntityIndex).e);
        glDrawElements(GL_TRIANGLES, CurrentMesh->IndiceCount, GL_UNSIGNED_INT, 0);
      }
      glBindVertexArray(0);
    }

#if FIGHT_PETER_PAN
    glCullFace(GL_BACK);
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    GameState->R.RecomputeDirectionalShadows = false;
  }

  if(GameState->R.ClearDirectionalShadows)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.DepthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GameState->R.ClearDirectionalShadows = false;
  }

  {
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.ScreenFBO[GameState->R.CurrentFramebuffer]);
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw Cubemap
    // TODO (rytis): Finish cubemap loading
    if(GameState->DrawCubemap)
    {
      if(GameState->R.Cubemap.CubemapTexture == -1)
      {
        GameState->R.Cubemap.CubemapTexture =
          LoadCubemap(&GameState->Resources, GameState->R.Cubemap.FaceIDs);
      }
      glDepthFunc(GL_LEQUAL);
      GLuint CubemapShaderID = GameState->Resources.GetShader(GameState->R.ShaderCubemap);
      glUseProgram(CubemapShaderID);
      glUniformMatrix4fv(glGetUniformLocation(CubemapShaderID, "mat_projection"), 1, GL_FALSE,
                         GameState->Camera.ProjectionMatrix.e);
      Render::mesh* CubemapMesh =
        GameState->Resources.GetModel(GameState->CubemapModelID)->Meshes[0];
      glUniformMatrix4fv(glGetUniformLocation(CubemapShaderID, "mat_view"), 1, GL_FALSE,
                         Math::Mat3ToMat4(Math::Mat4ToMat3(GameState->Camera.ViewMatrix)).e);
      glBindVertexArray(CubemapMesh->VAO);
      glBindTexture(GL_TEXTURE_CUBE_MAP, GameState->R.Cubemap.CubemapTexture);
      glDrawElements(GL_TRIANGLES, CubemapMesh->IndiceCount, GL_UNSIGNED_INT, 0);

      glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
      glBindVertexArray(0);
    }

    // TODO(Lukas) SORT(MeshInstances, ByBlend, ByMaterial, ByMesh);
    // Draw scene to backbuffer
    {
      material*     PreviousMaterial = nullptr;
      Render::mesh* PreviousMesh     = nullptr;
      uint32_t      CurrentShaderID  = 0;
      for(int i = 0; i < GameState->R.MeshInstanceCount; i++)
      {
        material*     CurrentMaterial    = GameState->R.MeshInstances[i].Material;
        Render::mesh* CurrentMesh        = GameState->R.MeshInstances[i].Mesh;
        int           CurrentEntityIndex = GameState->R.MeshInstances[i].EntityIndex;
        if(CurrentMaterial != PreviousMaterial)
        {
          if(PreviousMaterial)
          {
            glBindTexture(GL_TEXTURE_2D, 0);
          }
          CurrentShaderID = SetMaterial(GameState, &GameState->Camera, CurrentMaterial);

          PreviousMaterial = CurrentMaterial;
        }
        if(CurrentMesh != PreviousMesh)
        {
          glBindVertexArray(CurrentMesh->VAO);
          PreviousMesh = CurrentMesh;
        }
        if(CurrentMaterial->Common.IsSkeletal &&
           GameState->Entities[CurrentEntityIndex].AnimController)
        {
          glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "g_boneMatrices"),
                             GameState->Entities[CurrentEntityIndex]
                               .AnimController->Skeleton->BoneCount,
                             GL_FALSE,
                             (float*)GameState->Entities[CurrentEntityIndex]
                               .AnimController->HierarchicalModelSpaceMatrices);
        }
        else
        {
          mat4 Mat4Zeros = {};
          glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "g_boneMatrices"), 1, GL_FALSE,
                             Mat4Zeros.e);
        }
        glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_mvp"), 1, GL_FALSE,
                           GetEntityMVPMatrix(GameState, CurrentEntityIndex).e);
        glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_model"), 1, GL_FALSE,
                           GetEntityModelMatrix(GameState, CurrentEntityIndex).e);
        glDrawElements(GL_TRIANGLES, CurrentMesh->IndiceCount, GL_UNSIGNED_INT, 0);
      }
      glBindVertexArray(0);
    }

    // SELECTION HIGLIGHTING
    entity* SelectedEntity;
    if(Input->IsMouseInEditorMode && GetSelectedEntity(GameState, &SelectedEntity))
    {
      // MESH SELECTION HIGHLIGHTING
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDepthFunc(GL_LEQUAL);
      vec4   ColorRed      = vec4{ 1, 1, 0, 1 };
      GLuint ColorShaderID = GameState->Resources.GetShader(GameState->R.ShaderColor);
      glUseProgram(ColorShaderID);
      glUniform4fv(glGetUniformLocation(ColorShaderID, "g_color"), 1, (float*)&ColorRed);
      glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "mat_mvp"), 1, GL_FALSE,
                         GetEntityMVPMatrix(GameState, GameState->SelectedEntityIndex).e);
      if(SelectedEntity->AnimController)
      {
        glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "g_boneMatrices"),
                           SelectedEntity->AnimController->Skeleton->BoneCount, GL_FALSE,
                           (float*)SelectedEntity->AnimController->HierarchicalModelSpaceMatrices);
      }
      else
      {
        mat4 Mat4Zeros = {};
        glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "g_boneMatrices"), 1, GL_FALSE,
                           Mat4Zeros.e);
      }
      if(GameState->SelectionMode == SELECT_Mesh)
      {
        Render::mesh* SelectedMesh = {};
        if(GetSelectedMesh(GameState, &SelectedMesh))
        {
          glBindVertexArray(SelectedMesh->VAO);

          glDrawElements(GL_TRIANGLES, SelectedMesh->IndiceCount, GL_UNSIGNED_INT, 0);
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
      }
      // MODEL SELECTION HIGHLIGHTING
      else if(GameState->SelectionMode == SELECT_Entity)
      {
        entity* SelectedEntity = {};
        if(GetSelectedEntity(GameState, &SelectedEntity))
        {
          mat4 Mat4EntityTransform = TransformToMat4(&SelectedEntity->Transform);
          Debug::PushGizmo(&GameState->Camera, &Mat4EntityTransform,
                           SelectedEntity->Transform.Scale);
          Render::model* Model = GameState->Resources.GetModel(SelectedEntity->ModelID);
          for(int m = 0; m < Model->MeshCount; m++)
          {
            glBindVertexArray(Model->Meshes[m]->VAO);
            glDrawElements(GL_TRIANGLES, Model->Meshes[m]->IndiceCount, GL_UNSIGNED_INT, 0);
          }
        }
      }
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // RENDERING MATERIAL PREVIEW TO TEXTURE
  if(GameState->CurrentMaterialID.Value > 0)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);

    material* PreviewMaterial = GameState->Resources.GetMaterial(GameState->CurrentMaterialID);
    glClearColor(0.7f, 0.7f, 0.7f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    uint32_t MaterialPreviewShaderID =
      SetMaterial(GameState, &GameState->PreviewCamera, PreviewMaterial);

    if(PreviewMaterial->Common.IsSkeletal)
    {
      mat4 Mat4Zeros = {};
      glUniformMatrix4fv(glGetUniformLocation(MaterialPreviewShaderID, "g_boneMatrices"), 1,
                         GL_FALSE, Mat4Zeros.e);
    }
    glEnable(GL_BLEND);
    mat4           PreviewSphereMatrix = Math::Mat4Ident();
    Render::model* UVSphereModel       = GameState->Resources.GetModel(GameState->UVSphereModelID);
    glBindVertexArray(UVSphereModel->Meshes[0]->VAO);
    glUniformMatrix4fv(glGetUniformLocation(MaterialPreviewShaderID, "mat_mvp"), 1, GL_FALSE,
                       Math::MulMat4(GameState->PreviewCamera.VPMatrix, Math::Mat4Ident()).e);
    glUniformMatrix4fv(glGetUniformLocation(MaterialPreviewShaderID, "mat_model"), 1, GL_FALSE,
                       PreviewSphereMatrix.e);
    glUniform3fv(glGetUniformLocation(MaterialPreviewShaderID, "lightPosition"), 1,
                 (float*)&GameState->R.PreviewLightPosition);

    glDrawElements(GL_TRIANGLES, UVSphereModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // ------------POST-PROCESSING------------
  // NOTE(rytis): Post-processing was done pretty sloppily, so it will most likely require some
  // kind of rewrite/reintegration in the future.
  //
  // For now, though, it should be good enough?? Might still want to improve some parts (like
  // the hard-coded screen quad vertices).
  //
  // Currently post-processing shaders only affect scene elements (entities, cubemap). GUI and
  // debug drawings *should* be untouched.

  {
    uint32_t CurrentFramebuffer = GameState->R.CurrentFramebuffer;
    uint32_t CurrentTexture     = GameState->R.CurrentTexture;

    glDisable(GL_DEPTH_TEST);

    if(GameState->R.PPEffects)
    {
      if(GameState->R.PPEffects & (POST_Blur | POST_DepthOfField))
      {
        if(GameState->R.PostBlurStdDev != GameState->R.PostBlurLastStdDev)
        {
          GameState->R.PostBlurLastStdDev = GameState->R.PostBlurStdDev;
          GenerateGaussianBlurKernel(GameState->R.PostBlurKernel, BLUR_KERNEL_SIZE,
                                     GameState->R.PostBlurLastStdDev);
        }

        GLuint PostBlurHShaderID = GameState->Resources.GetShader(GameState->R.PostBlurH);
        glUseProgram(PostBlurHShaderID);

        BindNextFramebuffer(GameState->R.ScreenFBO, &GameState->R.CurrentFramebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniform1f(glGetUniformLocation(PostBlurHShaderID, "Offset"), 1.0f / SCREEN_WIDTH);
        glUniform1fv(glGetUniformLocation(PostBlurHShaderID, "Kernel"), BLUR_KERNEL_SIZE,
                     GameState->R.PostBlurKernel);

        BindTextureAndSetNext(GameState->R.ScreenTexture, &GameState->R.CurrentTexture);
        DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);

        GLuint PostBlurVShaderID = GameState->Resources.GetShader(GameState->R.PostBlurV);
        glUseProgram(PostBlurVShaderID);

        glUniform1f(glGetUniformLocation(PostBlurVShaderID, "Offset"), 1.0f / SCREEN_HEIGHT);
        glUniform1fv(glGetUniformLocation(PostBlurVShaderID, "Kernel"), BLUR_KERNEL_SIZE,
                     GameState->R.PostBlurKernel);

        if(GameState->R.PPEffects & POST_DepthOfField)
        {
          BindNextFramebuffer(GameState->R.ScreenFBO, &GameState->R.CurrentFramebuffer);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          GLuint ShaderDepthOfFieldID =
            GameState->Resources.GetShader(GameState->R.PostDepthOfField);

          glUseProgram(ShaderDepthOfFieldID);
          {
            int tex_index = 1;
            glActiveTexture(GL_TEXTURE0 + tex_index);
            glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferPositionTexID);
            glUniform1i(glGetUniformLocation(ShaderDepthOfFieldID, "u_PositionMap"), tex_index);
          }
          {
            int tex_index = 2;
            glActiveTexture(GL_TEXTURE0 + tex_index);
            glBindTexture(GL_TEXTURE_2D, GameState->R.ScreenTexture[CurrentTexture]);
            glUniform1i(glGetUniformLocation(ShaderDepthOfFieldID, "u_InputMap"), tex_index);
          }
          {
            int tex_index = 3;
            glActiveTexture(GL_TEXTURE0 + tex_index);
            BindTextureAndSetNext(GameState->R.ScreenTexture, &GameState->R.CurrentTexture);
            glUniform1i(glGetUniformLocation(ShaderDepthOfFieldID, "u_BlurredMap"), tex_index);
          }

          DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
          glActiveTexture(GL_TEXTURE0);
        }
      }

      if(GameState->R.PPEffects & POST_MotionBlur)
      {
        BindNextFramebuffer(GameState->R.ScreenFBO, &GameState->R.CurrentFramebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLuint ShaderMotionBlurID = GameState->Resources.GetShader(GameState->R.PostMotionBlur);

        glUseProgram(ShaderMotionBlurID);
        {
          int tex_index = 1;
          glActiveTexture(GL_TEXTURE0 + tex_index);
          glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferVelocityTexID);
          glUniform1i(glGetUniformLocation(ShaderMotionBlurID, "u_VelocityMap"), tex_index);
        }
        {
          int tex_index = 2;
          glActiveTexture(GL_TEXTURE0 + tex_index);
          BindTextureAndSetNext(GameState->R.ScreenTexture, &GameState->R.CurrentTexture);
          glUniform1i(glGetUniformLocation(ShaderMotionBlurID, "u_InputMap"), tex_index);
        }

        DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
        glActiveTexture(GL_TEXTURE0);
      }

      if(GameState->R.PPEffects & POST_Grayscale)
      {
        GLuint PostGrayscaleShaderID = GameState->Resources.GetShader(GameState->R.PostGrayscale);
        glUseProgram(PostGrayscaleShaderID);

        BindNextFramebuffer(GameState->R.ScreenFBO, &GameState->R.CurrentFramebuffer);
        BindTextureAndSetNext(GameState->R.ScreenTexture, &GameState->R.CurrentTexture);
        DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
      }

      if(GameState->R.PPEffects & POST_NightVision)
      {
        GLuint PostNightVisionShaderID =
          GameState->Resources.GetShader(GameState->R.PostNightVision);
        glUseProgram(PostNightVisionShaderID);

        BindNextFramebuffer(GameState->R.ScreenFBO, &GameState->R.CurrentFramebuffer);
        BindTextureAndSetNext(GameState->R.ScreenTexture, &GameState->R.CurrentTexture);
        DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
      }
    }
    else
    {
      GLuint PostDefaultShaderID = GameState->Resources.GetShader(GameState->R.PostDefaultShader);
      glUseProgram(PostDefaultShaderID);
    }

    if(GameState->R.DrawDepthMap)
    {
      GLuint RenderDepthMapShaderID = GameState->Resources.GetShader(GameState->R.RenderDepthMap);
      glUseProgram(RenderDepthMapShaderID);
      BindNextFramebuffer(GameState->R.ScreenFBO, &GameState->R.CurrentFramebuffer);

      int TexIndex = 1;

      glActiveTexture(GL_TEXTURE0 + TexIndex);
      glBindTexture(GL_TEXTURE_2D, GameState->R.DepthMapTexture);
      glUniform1i(glGetUniformLocation(RenderDepthMapShaderID, "DepthMap"), TexIndex);
      DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
      glActiveTexture(GL_TEXTURE0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, GameState->R.ScreenTexture[GameState->R.CurrentTexture]);
    DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);

    glEnable(GL_DEPTH_TEST);

    GameState->R.CurrentFramebuffer = CurrentFramebuffer;
    GameState->R.CurrentTexture     = CurrentTexture;
  }

  // ------------------DEBUG------------------
  Debug::DrawWireframeSpheres(GameState);

  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  if(GameState->DrawGizmos)
  {
    Debug::DrawGizmos(GameState);
  }
  Debug::DrawLines(GameState);
  Debug::DrawQuads(GameState);
  Debug::ClearDrawArrays();
  Text::ClearTextRequestCounts();
}
