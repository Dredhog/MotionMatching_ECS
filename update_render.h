#pragma once

#include "load_shader.h"

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "game.h"
#include "mesh.h"
#include "model.h"
#include "file_io.h"
#include "asset.h"
#include "load_texture.h"
#include "misc.h"
#include "intersection_testing.h"
#include "load_asset.h"
#include "render_data.h"
#include "material_upload.h"
#include "text.h"

#include "debug_drawing.h"
#include "camera.h"

#include "editor_ui.h"
#include <limits.h>

static const vec3 g_BoneColors[] = {
  { 0.41f, 0.93f, 0.23f }, { 0.14f, 0.11f, 0.80f }, { 0.35f, 0.40f, 0.77f },
  { 0.96f, 0.24f, 0.15f }, { 0.20f, 0.34f, 0.44f }, { 0.37f, 0.34f, 0.14f },
  { 0.22f, 0.99f, 0.77f }, { 0.80f, 0.70f, 0.11f }, { 0.81f, 0.92f, 0.18f },
  { 0.51f, 0.86f, 0.13f }, { 0.80f, 0.94f, 0.10f }, { 0.70f, 0.42f, 0.52f },
  { 0.26f, 0.50f, 0.61f }, { 0.10f, 0.21f, 0.81f }, { 0.96f, 0.22f, 0.63f },
  { 0.77f, 0.22f, 0.79f }, { 0.30f, 0.00f, 0.07f }, { 0.98f, 0.28f, 0.02f },
  { 0.92f, 0.42f, 0.14f }, { 0.47f, 0.31f, 0.72f },
};

mat4
GetEntityModelMatrix(game_state* GameState, int32_t EntityIndex)
{
  mat4 ModelMatrix = TransformToMat4(&GameState->Entities[EntityIndex].Transform);
  return ModelMatrix;
}

mat4
GetEntityMVPMatrix(game_state* GameState, int32_t EntityIndex)
{
  mat4 ModelMatrix = GetEntityModelMatrix(GameState, EntityIndex);
  mat4 MVPMatrix   = Math::MulMat4(GameState->Camera.VPMatrix, ModelMatrix);
  return MVPMatrix;
}

void
AddEntity(game_state* GameState, Render::model* Model, int32_t* MaterialIndices,
          Anim::transform Transform)
{
  assert(0 <= GameState->EntityCount && GameState->EntityCount < ENTITY_MAX_COUNT);

  entity NewEntity          = {};
  NewEntity.Model           = Model;
  NewEntity.MaterialIndices = MaterialIndices;
  NewEntity.Transform       = Transform;

  GameState->Entities[GameState->EntityCount++] = NewEntity;
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  game_state* GameState = (game_state*)GameMemory.PersistentMemory;
  assert(GameMemory.HasBeenInitialized);
  //---------------------BEGIN INIT -------------------------
  if(GameState->MagicChecksum != 123456)
  {
    GameState->WAVLoaded     = false;
    GameState->MagicChecksum = 123456;
    GameState->PersistentMemStack =
      Memory::CreateStackAllocatorInPlace((uint8_t*)GameMemory.PersistentMemory +
                                            sizeof(game_state),
                                          GameMemory.PersistentMemorySize - sizeof(game_state));
    GameState->TemporaryMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory,
                                          GameMemory.TemporaryMemorySize);
    Memory::stack_allocator* TemporaryMemStack  = GameState->TemporaryMemStack;
    Memory::stack_allocator* PersistentMemStack = GameState->PersistentMemStack;

    // --------LOAD MODELS/ACTORS--------
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/gizmo1.model",
                             &GameState->GizmoModel, NULL);
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/debug_meshes.model",
                             &GameState->QuadModel, NULL);
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/multimesh_soldier.actor",
                             &GameState->CharacterModel, &GameState->AnimEditor.Skeleton);
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/inverse_cube.model",
                             &GameState->CubemapModel, NULL);
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/sphere.model",
                             &GameState->SphereModel, NULL);
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/uv_sphere.model",
                             &GameState->UVSphereModel, NULL);
    AddModel(&GameState->R, GameState->CharacterModel);
    Render::model* TempSponzaPtr;

    /*
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/ak47.model", &TempSponzaPtr, NULL);
    AddModel(&GameState->R, TempSponzaPtr);

    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/sponza.model", &TempSponzaPtr, NULL);
    AddModel(&GameState->R, TempSponzaPtr);

    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/conference.model", &TempSponzaPtr,
                             NULL);
    AddModel(&GameState->R, TempSponzaPtr);
    */

    // -----------LOAD SHADERS------------
    GameState->R.ShaderPhong = CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/phong");
    GameState->R.ShaderLightingMapPhong =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/lighting_map_phong");
    GameState->R.ShaderMaterialPhong =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/material_blinn_phong");
    GameState->R.ShaderCubemap =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/cubemap");
    GameState->R.ShaderGizmo = CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/gizmo");
    GameState->R.ShaderQuad =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/debug_quad");
    GameState->R.ShaderColor = CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/color");
    GameState->R.ShaderSkeletalBoneColor =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/skeletal_bone_color");
    GameState->R.ShaderSkeletalPhong =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/skeletal_phong");
    GameState->R.ShaderTexturedQuad =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/debug_textured_quad");
    GameState->R.ShaderID = CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/id");
    //------------LOAD TEXTURES-----------
    // Diffuse Maps
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/diffuse/body_dif.png"),
               "body_diff");
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/diffuse/arm_dif.png"),
               "arm_diff");
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/diffuse/hand_dif.png"),
               "hand_diff");
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/diffuse/leg_dif.png"),
               "leg_diff");
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/diffuse/helmet_diff.png"),
               "helmet_diff");
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/diffuse/glass_dif.png"),
               "glass_diff");
    // Specular Maps
    AddTexture(&GameState->R,
               Texture::LoadTexture("./data/textures/specular/hand_showroom_spec.png"), "had_spec");
    AddTexture(&GameState->R,
               Texture::LoadTexture("./data/textures/specular/helmet_showroom_spec.png"),
               "helmet_spec");
    AddTexture(&GameState->R,
               Texture::LoadTexture("./data/textures/specular/body_showroom_spec.png"),
               "body_sped");
    AddTexture(&GameState->R,
               Texture::LoadTexture("./data/textures/specular/leg_showroom_spec.png"), "leg_spec");
    AddTexture(&GameState->R,
               Texture::LoadTexture("./data/textures/specular/arm_showroom_spec.png"), "arm_spec");
    // Normal Maps
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/normal/hand_showroom_ddn.png"),
               "hand_norm");
    AddTexture(&GameState->R,
               Texture::LoadTexture("./data/textures/normal/helmet_showroom_ddn.png"),
               "helmet_norm");
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/normal/glass_ddn.png"),
               "glass_norm");
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/normal/body_showroom_ddn.png"),
               "body_norm");
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/normal/leg_showroom_ddn.png"),
               "leg_norm");
    AddTexture(&GameState->R, Texture::LoadTexture("./data/textures/normal/arm_showroom_ddn.png"),
               "arm_norm");
    GameState->CollapsedTextureID = Texture::LoadTexture("./data/textures/collapsed.bmp");
    GameState->ExpandedTextureID  = Texture::LoadTexture("./data/textures/expanded.bmp");
    assert(GameState->CollapsedTextureID);
    assert(GameState->ExpandedTextureID);

    GameState->Font = Text::LoadFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14, 8, 1);

    GameState->CubemapTexture =
      Texture::LoadCubemap(TemporaryMemStack, "./data/textures/iceflats", "tga");
    // -------END ASSET LOADING

    // ======Set GL state
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    // Create index framebuffer for mesh picking
    glGenFramebuffers(1, &GameState->IndexFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
    glGenTextures(1, &GameState->IDTexture);
    glBindTexture(GL_TEXTURE_2D, GameState->IDTexture);
#if 0
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED_INTEGER, GL_INT,
                 NULL);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA,
                 GL_UNSIGNED_INT, NULL);
#endif
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
      assert(0 && "error: incomplete frambuffer!\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // -------InitGameState
    GameState->Camera.Position      = { 0, 1.6f, 2 };
    GameState->Camera.Up            = { 0, 1, 0 };
    GameState->Camera.Forward       = { 0, 0, -1 };
    GameState->Camera.Right         = { 1, 0, 0 };
    GameState->Camera.Rotation      = { -20 };
    GameState->Camera.NearClipPlane = 0.001f;
    GameState->Camera.FarClipPlane  = 1000.0f;
    GameState->Camera.FieldOfView   = 70.0f;
    GameState->Camera.MaxTiltAngle  = 90.0f;
    GameState->Camera.Speed         = 2.0f;

    GameState->PreviewCamera          = GameState->Camera;
    GameState->PreviewCamera.Position = { 0, 0, 3 };
    GameState->PreviewCamera.Rotation = {};
    UpdateCamera(&GameState->PreviewCamera, Input);

    GameState->R.LightPosition = { 1, 1, 2 };
    //-----Testing purposes-----
    // NOTE: Not sure if separating light into these 3 components should be done.
    GameState->R.LightSpecularColor = { 1.0f, 1.0f, 1.0f };
    GameState->R.LightDiffuseColor  = GameState->R.LightSpecularColor * 0.5f;
    GameState->R.LightAmbientColor  = GameState->R.LightSpecularColor * 0.2f;
    //--------------------------
    GameState->R.LightColor = GameState->R.LightSpecularColor;

    GameState->DrawWireframe           = false;
    GameState->DrawCubemap             = false;
    GameState->DrawBoneWeights         = false;
    GameState->DrawTimeline            = true;
    GameState->DrawGizmos              = true;
    GameState->IsModelSpinning         = false;
    GameState->IsAnimationPlaying      = false;
    GameState->EditorBoneRotationSpeed = 45.0f;

    {
      material LightMapPhong0                       = NewLightMapPhongMaterial();
      LightMapPhong0.LightMapPhong.DiffuseMapIndex  = 3;
      LightMapPhong0.LightMapPhong.SpecularMapIndex = 8;
      LightMapPhong0.LightMapPhong.Shininess        = 0.5f;
      AddMaterial(&GameState->R, LightMapPhong0);

      material LightMapPhong1                       = NewLightMapPhongMaterial();
      LightMapPhong1.LightMapPhong.DiffuseMapIndex  = 0;
      LightMapPhong1.LightMapPhong.SpecularMapIndex = 7;
      LightMapPhong1.LightMapPhong.Shininess        = 0.5f;
      AddMaterial(&GameState->R, LightMapPhong1);

      material LightMapPhong2                       = NewLightMapPhongMaterial();
      LightMapPhong2.LightMapPhong.DiffuseMapIndex  = 0;
      LightMapPhong2.LightMapPhong.SpecularMapIndex = 1;
      LightMapPhong2.LightMapPhong.Shininess        = 0.5f;
      AddMaterial(&GameState->R, LightMapPhong2);

      material LightMapPhong3                       = NewLightMapPhongMaterial();
      LightMapPhong3.LightMapPhong.DiffuseMapIndex  = 5;
      LightMapPhong3.LightMapPhong.SpecularMapIndex = 10;
      LightMapPhong3.LightMapPhong.Shininess        = 0.5f;
      AddMaterial(&GameState->R, LightMapPhong3);

      material Phong0              = NewPhongMaterial();
      Phong0.Phong.DiffuseMapIndex = 0;
      AddMaterial(&GameState->R, Phong0);

      material Phong1              = NewPhongMaterial();
      Phong1.Phong.DiffuseMapIndex = 1;
      AddMaterial(&GameState->R, Phong1);

      material Phong2              = NewPhongMaterial();
      Phong2.Phong.DiffuseMapIndex = 2;
      AddMaterial(&GameState->R, Phong2);

      material Phong3              = NewPhongMaterial();
      Phong3.Phong.DiffuseMapIndex = 3;
      AddMaterial(&GameState->R, Phong3);

      material Phong4              = NewPhongMaterial();
      Phong4.Phong.DiffuseMapIndex = 4;
      AddMaterial(&GameState->R, Phong4);

      material Phong5              = NewPhongMaterial();
      Phong5.Phong.DiffuseMapIndex = 5;
      AddMaterial(&GameState->R, Phong5);

      material Phong6              = NewPhongMaterial();
      Phong6.Phong.DiffuseMapIndex = 6;
      AddMaterial(&GameState->R, Phong6);

      material Color0 = NewColorMaterial();
      AddMaterial(&GameState->R, Color0);
    }
  }
  //---------------------END INIT -------------------------

  //----------------------UPDATE------------------------
  UpdateCamera(&GameState->Camera, Input);

  if(GameState->IsModelSpinning)
  {
    entity* Entity = {};
    if(GetSelectedEntity(GameState, &Entity))
    {
      Entity->Transform.Rotation.Y += 45.0f * Input->dt;
    }
  }
  if(Input->b.EndedDown && Input->b.Changed)
  {
    GameState->DrawBoneWeights = !GameState->DrawBoneWeights;
  }
  if(Input->g.EndedDown && Input->g.Changed)
  {
    GameState->DrawGizmos = !GameState->DrawGizmos;
  }
  if(Input->f.EndedDown && Input->f.Changed)
  {
    GameState->DrawWireframe = !GameState->DrawWireframe;
  }
  if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed && GameState->IsEntityCreationMode)
  {
    GameState->IsEntityCreationMode = false;
    vec3 RayDir =
      GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                           GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
    raycast_result RaycastResult =
      RayIntersectPlane(GameState->Camera.Position, RayDir, {}, { 0, 1, 0 });
    if(RaycastResult.Success && GameState->R.ModelCount > 0)
    {
      Anim::transform NewTransform = {};
      NewTransform.Translation     = RaycastResult.IntersectP;
      NewTransform.Scale           = { 1, 1, 1 };

      if(GameState->R.ModelCount > 0)
      {
        Render::model* Model = GameState->R.Models[GameState->CurrentModel];
        int32_t*       MaterialIndices =
          PushArray(GameState->PersistentMemStack, Model->MeshCount, int32_t);
        if(0 <= GameState->CurrentMaterial && GameState->CurrentMaterial < MATERIAL_MAX_COUNT)
        {
          for(int m = 0; m < Model->MeshCount; m++)
          {
            MaterialIndices[m] = GameState->CurrentMaterial;
          }
        }
        AddEntity(GameState, Model, MaterialIndices, NewTransform);
      }
    }
  }

  //---------------ANIMATION EDITOR UPDATE-----------------
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
      else if(Input->v.EndedDown && Input->v.Changed)
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
  }

  //---------------------RENDERING----------------------------

  GameState->R.MeshInstanceCount = 0;
  // Put enriry data to darw drawing queue every frame to avoid erroneous indirection due to sorting
  for(int e = 0; e < GameState->EntityCount; e++)
  {
    for(int m = 0; m < GameState->Entities[e].Model->MeshCount; m++)
    {
      mesh_instance MeshInstance = {};
      MeshInstance.Mesh          = GameState->Entities[e].Model->Meshes[m];
      MeshInstance.Material    = &GameState->R.Materials[GameState->Entities[e].MaterialIndices[m]];
      MeshInstance.EntityIndex = e;
      AddMeshInstance(&GameState->R, MeshInstance);
    }
  }

  // Draw scene to backbuffer
  // SORT(MeshInstances, ByMaterial, MyMesh);
  glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  material*     PreviousMaterial    = nullptr;
  Render::mesh* PreviousMesh        = nullptr;
  int32_t       PreviousEntityIndex = -1;
  uint32_t      CurrentShaderID     = 0;
  for(int i = 0; i < GameState->R.MeshInstanceCount; i++)
  {
    material*     CurrentMaterial    = GameState->R.MeshInstances[i].Material;
    Render::mesh* CurrentMesh        = GameState->R.MeshInstances[i].Mesh;
    int           CurrentEntityIndex = GameState->R.MeshInstances[i].EntityIndex;
    if(CurrentMaterial != PreviousMaterial)
    {
      if(PreviousMaterial)
      {
        // UnsetMaterial(&GameState->R, PreviousMaterial);
        {
          glBindTexture(GL_TEXTURE_2D, 0);
          glBindVertexArray(0);
        }
      }
      material Material = *CurrentMaterial;
      CurrentShaderID   = SetMaterial(&GameState->R, &GameState->Camera, CurrentMaterial);

      PreviousMaterial = CurrentMaterial;
    }
    if(CurrentMesh != PreviousMesh)
    {
      glBindVertexArray(CurrentMesh->VAO);
      PreviousMesh = CurrentMesh;
    }
    glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_mvp"), 1, GL_FALSE,
                       GetEntityMVPMatrix(GameState, CurrentEntityIndex).e);
    glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_model"), 1, GL_FALSE,
                       GetEntityModelMatrix(GameState, CurrentEntityIndex).e);
    glDrawElements(GL_TRIANGLES, CurrentMesh->IndiceCount, GL_UNSIGNED_INT, 0);
  }
  glBindVertexArray(0);

  // Higlight entity
  entity* SelectedEntity;
  if(GetSelectedEntity(GameState, &SelectedEntity))
  {
    // Higlight mesh
    glDepthFunc(GL_LEQUAL);
    Render::mesh* SelectedMesh = {};
    if(GetSelectedMesh(GameState, &SelectedMesh))
    {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glUseProgram(GameState->R.ShaderColor);
      vec4 ColorRed = vec4{ 1, 1, 0, 1 };
      glUniform4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_color"), 1, (float*)&ColorRed);
      glBindVertexArray(SelectedMesh->VAO);

      glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "mat_mvp"), 1, GL_FALSE,
                         GetEntityMVPMatrix(GameState, GameState->SelectedEntityIndex).e);
      glDrawElements(GL_TRIANGLES, SelectedMesh->IndiceCount, GL_UNSIGNED_INT, 0);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  }

  // Draw material preview to texture
  {
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
    glClearColor(0.7f, 0.7f, 0.7f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    uint32_t ShaderID = SetMaterial(&GameState->R, &GameState->PreviewCamera,
                                    &GameState->R.Materials[GameState->CurrentMaterial]);
    glEnable(GL_BLEND);
    mat4 PreviewSphereMatrix = Math::Mat4Ident();
    glBindVertexArray(GameState->UVSphereModel->Meshes[0]->VAO);
    glUniformMatrix4fv(glGetUniformLocation(ShaderID, "mat_mvp"), 1, GL_FALSE,
                       Math::MulMat4(GameState->PreviewCamera.VPMatrix, Math::Mat4Ident()).e);
    glUniformMatrix4fv(glGetUniformLocation(ShaderID, "mat_model"), 1, GL_FALSE,
                       PreviewSphereMatrix.e);

    glDrawElements(GL_TRIANGLES, GameState->UVSphereModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT,
                   0);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  if(GameState->DrawGizmos && GameState->AnimEditor.Skeleton)
  {
    mat4        BoneGizmos[SKELETON_MAX_BONE_COUNT];
    const float BoneSphereRadius = 0.1f;
    for(int i = 0; i < GameState->AnimEditor.Skeleton->BoneCount; i++)
    {
      /*BoneGizmos[i] =
        Math::MulMat4(ModelMatrix,
                      Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices[i],
                                    GameState->AnimEditor.Skeleton->Bones[i].BindPose));*/
      BoneGizmos[i] = Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices[i],
                                    GameState->AnimEditor.Skeleton->Bones[i].BindPose);
      if(Input->MouseRight.EndedDown && Input->MouseRight.Changed)
      {
        vec3 Position = Math::GetMat4Translation(BoneGizmos[i]);
        vec3 RayDir =
          GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                               GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
        raycast_result RaycastResult =
          RayIntersectSphere(GameState->Camera.Position, RayDir, Position, BoneSphereRadius);
        if(RaycastResult.Success)
        {
          EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, i);
        }
      }
    }

    for(int i = 0; i < GameState->AnimEditor.Skeleton->BoneCount; i++)
    {
      vec3 Position = Math::GetMat4Translation(BoneGizmos[i]);
      if(GameState->DrawWireframe)
      {
        DEBUGPushWireframeSphere(&GameState->Camera, Position, BoneSphereRadius);
      }
      if(GameState->DrawGizmos)
      {
        DEBUGPushGizmo(&GameState->Camera, &BoneGizmos[GameState->AnimEditor.CurrentBone]);
      }
    }
  }
  if(Input->IsMouseInEditorMode)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    // ANIMATION TIMELINE
    if(GameState->DrawTimeline && GameState->AnimEditor.Skeleton)
    {
      VisualizeTimeline(GameState);
    }
    // GUI
    DrawAndInteractWithEditorUI(GameState, Input);

    // Selection
    if(Input->MouseRight.EndedDown && Input->MouseRight.Changed)
    {
      // Draw entities to ID buffer
      // SORT_MESH_INSTANCES(ByEntity);
      glDisable(GL_BLEND);
      glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
      glClearColor(0.9f, 0.9f, 0.9f, 1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glUseProgram(GameState->R.ShaderID);
      for(int e = 0; e < GameState->EntityCount; e++)
      {
        glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderID, "mat_mvp"), 1, GL_FALSE,
                           GetEntityMVPMatrix(GameState, e).e);
        for(int m = 0; m < GameState->Entities[e].Model->MeshCount; m++)
        {
          glBindVertexArray(GameState->Entities[e].Model->Meshes[m]->VAO);
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
          glUniform4fv(glGetUniformLocation(GameState->R.ShaderID, "g_id"), 1,
                       (float*)&EntityColorID);
          glDrawElements(GL_TRIANGLES, GameState->Entities[e].Model->Meshes[m]->IndiceCount,
                         GL_UNSIGNED_INT, 0);
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
  }
  DEBUGDrawWireframeSpheres(GameState);

  glClear(GL_DEPTH_BUFFER_BIT);
  DEBUGDrawGizmos(GameState);
}
