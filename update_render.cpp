#include "load_shader.h"

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "game.h"
#include "mesh.h"
#include "model.h"
#include "asset.h"
#include "load_texture.h"
#include "misc.h"
#include "intersection_testing.h"
#include "load_asset.h"
#include "render_data.h"
#include "material_upload.h"
#include "material_io.h"
#include "collision_testing.h"

#include "text.h"

#include "debug_drawing.h"
#include "camera.h"
#include "player_controller.h"

#include "gui_testing.h"
#include <limits.h>
#include "dynamics.h"

void AddEntity(game_state* GameState, rid ModelID, rid* MaterialIDs, Anim::transform Transform);
mat4 GetEntityModelMatrix(game_state* GameState, int32_t EntityIndex);
mat4 GetEntityMVPMatrix(game_state* GameState, int32_t EntityIndex);

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  game_state* GameState = (game_state*)GameMemory.PersistentMemory;
  assert(GameMemory.HasBeenInitialized);

  //---------------------BEGIN INIT -------------------------
  if(GameState->MagicChecksum != 123456)
  {
    GameState->MagicChecksum = 123456;

    GameState->TemporaryMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory,
                                          GameMemory.TemporaryMemorySize);
    // SEGMENT MEMORY
    assert(GameMemory.PersistentMemorySize > sizeof(game_state));

    uint32_t AvailableSubsystemMemory = GameMemory.PersistentMemorySize - sizeof(game_state);
    uint32_t PersistentStackSize      = (uint32_t)((float)AvailableSubsystemMemory * 0.3);
    uint8_t* PersistentStackStart     = (uint8_t*)GameMemory.PersistentMemory + sizeof(game_state);

    uint32_t ResourceMemorySize = AvailableSubsystemMemory - PersistentStackSize;
    uint8_t* ResouceMemoryStart = PersistentStackStart + PersistentStackSize;

    GameState->PersistentMemStack =
      Memory::CreateStackAllocatorInPlace(PersistentStackStart, PersistentStackSize);
    GameState->Resources.Create(ResouceMemoryStart, ResourceMemorySize);
    // END SEGMENTATION

    // --------LOAD MODELS/ACTORS--------
    RegisterDebugModels(GameState);

    // -----------LOAD SHADERS------------
    GameState->R.ShaderPhong =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "shaders/phong");
    GameState->R.ShaderCubemap =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "shaders/cubemap");
    GameState->R.ShaderGizmo =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "shaders/gizmo");
    GameState->R.ShaderQuad =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "shaders/debug_quad");
    GameState->R.ShaderColor =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "shaders/color");
    GameState->R.ShaderTexturedQuad =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "shaders/debug_textured_quad");
    GameState->R.ShaderID =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "shaders/id");

    //------------LOAD TEXTURES-----------
    GameState->CollapsedTextureID = Texture::LoadTexture("./data/textures/collapsed.bmp");
    GameState->ExpandedTextureID  = Texture::LoadTexture("./data/textures/expanded.bmp");
    assert(GameState->CollapsedTextureID);
    assert(GameState->ExpandedTextureID);

    //--------------LOAD FONT--------------
    GameState->Font = Text::LoadFont("data/UbuntuMono.ttf", 18, 1, 2);

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
      assert(0 && "error: incomplete frambuffer!\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // -------Init GameState
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

    GameState->PreviewCamera          = GameState->Camera;
    GameState->PreviewCamera.Position = { 0, 0, 2 };
    GameState->PreviewCamera.Rotation = {};
    UpdateCamera(&GameState->PreviewCamera, Input);

    GameState->R.LightPosition        = { 0.7f, 1, 1 };
    GameState->R.PreviewLightPosition = { 0.7f, 0, 2 };

    GameState->R.LightSpecularColor = { 1, 1, 1 };
    GameState->R.LightDiffuseColor  = { 1, 1, 1 };
    GameState->R.LightAmbientColor  = { 0.3f, 0.3f, 0.3f };
    GameState->R.ShowLightPosition  = false;

    GameState->SimulateDynamics        = true;
    GameState->DrawCubemap             = true;
    GameState->DrawDebugSpheres        = true;
    GameState->DrawTimeline            = true;
    GameState->DrawGizmos              = true;
    GameState->IsAnimationPlaying      = false;
    GameState->EditorBoneRotationSpeed = 45.0f;
    GameState->CurrentMaterialID       = { 0 };
    GameState->PlayerEntityIndex       = -1;
    GameState->AssignedA               = false;
    GameState->AssignedB               = false;
    GameState->IterationCount          = 0;
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
    UI::TestGui(GameState, Input);
    // GUI
    /*IMGUIControlPanel(GameState, Input);

    // ANIMATION TIMELINE
    if(GameState->SelectionMode == SELECT_Bone && GameState->DrawTimeline &&
       GameState->AnimEditor.Skeleton)
    {
      VisualizeTimeline(GameState);
    }
    */

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
        entity* CurrentEntity;
        assert(GetEntityAtIndex(GameState, &CurrentEntity, e));
        glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderID, "mat_mvp"), 1, GL_FALSE,
                           GetEntityMVPMatrix(GameState, e).e);
        if(CurrentEntity->AnimController)
        {
          glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderID, "g_boneMatrices"),
                             CurrentEntity->AnimController->Skeleton->BoneCount, GL_FALSE,
                             (float*)CurrentEntity->AnimController->HierarchicalModelSpaceMatrices);
        }
        else
        {
          mat4 Mat4Zeros = {};
          glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderID, "g_boneMatrices"), 1,
                             GL_FALSE, Mat4Zeros.e);
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
          glUniform4fv(glGetUniformLocation(GameState->R.ShaderID, "g_id"), 1,
                       (float*)&EntityColorID);
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

  if(Input->i.EndedDown && Input->i.Changed)
  {
    ++GameState->IterationCount;
  }
  else if(Input->o.EndedDown && Input->o.Changed)
  {
    --GameState->IterationCount;
  }

  g_Force          = GameState->Force;
  g_ForceStart     = GameState->ForceStart;
  g_ApplyingForce  = GameState->ApplyingForce;
  g_ApplyingTorque = GameState->ApplyingTorque;

  if(GameState->SimulateDynamics)
  {
    SimulateDynamics(GameState);
  }

  // Collision testing
  if(GameState->AssignedA && GameState->AssignedB)
  {
    entity* EntityA;
    GetEntityAtIndex(GameState, &EntityA, GameState->EntityA);

    entity* EntityB;
    GetEntityAtIndex(GameState, &EntityB, GameState->EntityB);

    Render::mesh* MeshA = GameState->Resources.GetModel(EntityA->ModelID)->Meshes[0];
    Render::mesh* MeshB = GameState->Resources.GetModel(EntityB->ModelID)->Meshes[0];

    mat4 ModelAMatrix = GetEntityModelMatrix(GameState, GameState->EntityA);
    mat4 ModelBMatrix = GetEntityModelMatrix(GameState, GameState->EntityB);

    material* EntityAMaterial = GameState->Resources.GetMaterial(EntityA->MaterialIDs[0]);
    material* EntityBMaterial = GameState->Resources.GetMaterial(EntityB->MaterialIDs[0]);

    Debug::PushWireframeSphere({}, 0.05f, { 1, 1, 0, 1 });

    if(AreColliding(GameState, Input, MeshA, MeshB, ModelAMatrix, ModelBMatrix,
                    GameState->IterationCount))
    {
      EntityAMaterial->Phong.Flags        = EntityAMaterial->Phong.Flags & !(PHONG_UseDiffuseMap);
      EntityBMaterial->Phong.Flags        = EntityBMaterial->Phong.Flags & !(PHONG_UseDiffuseMap);
      EntityAMaterial->Common.UseBlending = true;
      EntityBMaterial->Common.UseBlending = true;

      vec4 Color                          = { 1.0f, 0.0f, 0.0f, 0.8f };
      EntityAMaterial->Phong.DiffuseColor = Color;
      EntityBMaterial->Phong.DiffuseColor = Color;
      GameState->ABCollide                = true;
    }
    else
    {
      EntityAMaterial->Phong.Flags        = EntityAMaterial->Phong.Flags & !(PHONG_UseDiffuseMap);
      EntityBMaterial->Phong.Flags        = EntityBMaterial->Phong.Flags & !(PHONG_UseDiffuseMap);
      EntityAMaterial->Common.UseBlending = true;
      EntityBMaterial->Common.UseBlending = true;

      vec4 Color                          = { 0.5f, 0.5f, 0.5f, 0.8f };
      EntityAMaterial->Phong.DiffuseColor = Color;
      EntityBMaterial->Phong.DiffuseColor = Color;
      GameState->ABCollide                = false;
    }
  }

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

  GameState->R.MeshInstanceCount = 0;
  // Put entiry data to draw drawing queue every frame to avoid erroneous indirection due to
  // sorting
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

  {
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Draw Cubemap
    // TODO (rytis): Finish cubemap loading
    if(GameState->DrawCubemap)
    {
      if(GameState->Cubemap.CubemapTexture == -1)
      {
        GameState->Cubemap.CubemapTexture =
          LoadCubemap(&GameState->Resources, GameState->Cubemap.FaceIDs);
      }
      glDepthFunc(GL_LEQUAL);
      glUseProgram(GameState->R.ShaderCubemap);
      glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderCubemap, "mat_projection"), 1,
                         GL_FALSE, GameState->Camera.ProjectionMatrix.e);
      glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderCubemap, "mat_view"), 1, GL_FALSE,
                         Math::Mat3ToMat4(Math::Mat4ToMat3(GameState->Camera.ViewMatrix)).e);
      glBindVertexArray(GameState->Resources.GetModel(GameState->CubemapModelID)->Meshes[0]->VAO);
      glBindTexture(GL_TEXTURE_CUBE_MAP, GameState->Cubemap.CubemapTexture);
      glDrawElements(GL_TRIANGLES,
                     GameState->Resources.GetModel(GameState->CubemapModelID)
                       ->Meshes[0]
                       ->IndiceCount,
                     GL_UNSIGNED_INT, 0);

      glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
      glBindVertexArray(0);
    }

    // Draw scene to backbuffer
    // SORT(MeshInstances, ByMaterial, MyMesh);

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
          glBindVertexArray(0);
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

  // Higlight entity
  entity* SelectedEntity;
  if(Input->IsMouseInEditorMode && GetSelectedEntity(GameState, &SelectedEntity))
  {
    // Higlight mesh
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDepthFunc(GL_LEQUAL);
    vec4 ColorRed = vec4{ 1, 1, 0, 1 };
    glUseProgram(GameState->R.ShaderColor);
    glUniform4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_color"), 1, (float*)&ColorRed);
    glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "mat_mvp"), 1, GL_FALSE,
                       GetEntityMVPMatrix(GameState, GameState->SelectedEntityIndex).e);
    if(SelectedEntity->AnimController)
    {
      glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_boneMatrices"),
                         SelectedEntity->AnimController->Skeleton->BoneCount, GL_FALSE,
                         (float*)SelectedEntity->AnimController->HierarchicalModelSpaceMatrices);
    }
    else
    {
      mat4 Mat4Zeros = {};
      glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_boneMatrices"), 1,
                         GL_FALSE, Mat4Zeros.e);
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
    // Highlight Entity
    else if(GameState->SelectionMode == SELECT_Entity)
    {
      entity* SelectedEntity = {};
      if(GetSelectedEntity(GameState, &SelectedEntity))
      {
        mat4 Mat4EntityTransform = TransformToMat4(&SelectedEntity->Transform);
        Debug::PushGizmo(&GameState->Camera, &Mat4EntityTransform);
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

  // Draw material preview to texture
  if(GameState->CurrentMaterialID.Value > 0)
  {
    material* PreviewMaterial = GameState->Resources.GetMaterial(GameState->CurrentMaterialID);
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
    glClearColor(0.7f, 0.7f, 0.7f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    uint32_t ShaderID = SetMaterial(GameState, &GameState->PreviewCamera, PreviewMaterial);

    if(PreviewMaterial->Common.IsSkeletal)
    {
      mat4 Mat4Zeros = {};
      glUniformMatrix4fv(glGetUniformLocation(ShaderID, "g_boneMatrices"), 1, GL_FALSE,
                         Mat4Zeros.e);
    }
    glEnable(GL_BLEND);
    mat4           PreviewSphereMatrix = Math::Mat4Ident();
    Render::model* UVSphereModel       = GameState->Resources.GetModel(GameState->UVSphereModelID);
    glBindVertexArray(UVSphereModel->Meshes[0]->VAO);
    glUniformMatrix4fv(glGetUniformLocation(ShaderID, "mat_mvp"), 1, GL_FALSE,
                       Math::MulMat4(GameState->PreviewCamera.VPMatrix, Math::Mat4Ident()).e);
    glUniformMatrix4fv(glGetUniformLocation(ShaderID, "mat_model"), 1, GL_FALSE,
                       PreviewSphereMatrix.e);
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderPhong, "lightPosition"), 1,
                 (float*)&GameState->R.PreviewLightPosition);

    glDrawElements(GL_TRIANGLES, UVSphereModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

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

void
AddEntity(game_state* GameState, rid ModelID, rid* MaterialIDs, Anim::transform Transform)
{
  assert(0 <= GameState->EntityCount && GameState->EntityCount < ENTITY_MAX_COUNT);

  entity NewEntity      = {};
  NewEntity.ModelID     = ModelID;
  NewEntity.MaterialIDs = MaterialIDs;
  NewEntity.Transform   = Transform;
  GameState->Resources.Models.AddReference(ModelID);

  GameState->Entities[GameState->EntityCount++] = NewEntity;
}

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
