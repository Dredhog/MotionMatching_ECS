#include "load_shader.h"

#include "linear_math/matrix.h"
#include "linear_math/vector.h"
#include "linear_math/distribution.h"

// TODO(Lukas) make sure that animations references are properly managed

#include "game.h"
#include "mesh.h"
#include "model.h"
#include "asset.h"
#include "load_texture.h"
#include "misc.h"
#include "text.h"
#include "material_io.h"
#include "camera.h"
#include "shader_def.h"
#include "render_data.h"

#include "profile.h"

#include "intersection_testing.h"
#include "debug_drawing.h"
#include "player_controller.h"
#include "material_upload.h"

#include "dynamics.h"
#include "gui_testing.h"

#include "initialization.h"
#include "edit_mode_interaction.h"
#include "rendering.h"
#include "post_processing.h"

// TODO remove these globals
extern bool g_VisualizeContactPoints;
extern bool g_VisualizeContactManifold;

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  BEGIN_TIMED_FRAME();

  game_state* GameState = (game_state*)GameMemory.PersistentMemory;
  assert(GameMemory.HasBeenInitialized);

  // GAME STATE INITIALIZATION (ONLY RUN ON FIRST FRAME)
  if(GameState->MagicChecksum != 123456)
  {
    INIT_GPU_TIMERS();
    TIMED_BLOCK(FirstInit);
    PartitionMemoryInitAllocators(&GameMemory, GameState);
    RegisterLoadInitialResources(GameState);
    SetGameStatePODFields(GameState);
    InitializeECS(GameState->PersistentMemStack, &GameState->ECSRuntime, &GameState->ECSWorld,
                  Mibibytes(1));

    if(!GameState->UpdatePathList)
    {
      TIMED_BLOCK(FilesystemUpdate);
      GameState->Resources.UpdateHardDriveAssetPathLists();
    }
  }

  BEGIN_TIMED_BLOCK(Update)
  {
    TIMED_BLOCK(FilesystemUpdate);
    if(GameState->UpdatePathList)
    {
      TIMED_BLOCK(UpdateHardDrivePathList);
      GameState->Resources.UpdateHardDriveAssetPathLists();
    }
    if(GameState->UseHotReloading)
    {
      TIMED_BLOCK(HotReloadAssets);
      GameState->Resources.DeleteUnused();
      GameState->Resources.ReloadModified();
    }
  }

  if(Input->IsMouseInEditorMode)
  {
    // TODO(LUkas) Move this material check to somewhere more appropriate
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

    EditWorldAndInteractWithGUI(GameState, Input);
  }

  //--------------------WORLD UPDATE------------------------

  UpdateCamera(&GameState->Camera, Input);

  {
    TIMED_BLOCK(Physics);

    assert(GameState->EntityCount <= RIGID_BODY_MAX_COUNT);
    GameState->Physics.RBCount = GameState->EntityCount;

    {
      g_VisualizeContactPoints   = GameState->Physics.Switches.VisualizeContactPoints;
      g_VisualizeContactManifold = GameState->Physics.Switches.VisualizeContactManifold;
      // Copy entity transform state into the physics world
      // Note: valid entiteis are always stored without gaps in their array
      for(int i = 0; i < GameState->EntityCount; i++)
      {
        // Copy rigid body from entity (Mainly needed when loading scenes)
        GameState->Physics.RigidBodies[i] = GameState->Entities[i].RigidBody;

        if(FloatsEqualByThreshold(Math::Length(GameState->Entities[i].Transform.R), 0.0f, 0.0001f))
        {
          GameState->Entities[i].Transform.R = Math::QuatIdent();
        }
        else
        {
          Math::Normalize(&GameState->Entities[i].Transform.R);
        }

        GameState->Physics.RigidBodies[i].q = GameState->Entities[i].Transform.R;
        GameState->Physics.RigidBodies[i].X = GameState->Entities[i].Transform.T;

        GameState->Physics.RigidBodies[i].R =
          Math::Mat4ToMat3(Math::Mat4Rotate(GameState->Entities[i].Transform.R));

        GameState->Physics.RigidBodies[i].Mat4Scale =
          Math::Mat4Scale(GameState->Entities[i].Transform.S);

        GameState->Physics.RigidBodies[i].Collider =
          GameState->Resources.GetModel(GameState->Entities[i].ModelID)->Meshes[0];

        const rigid_body& RB = GameState->Physics.RigidBodies[i];
        if(GameState->Physics.Switches.VisualizeOmega)
        {
          Debug::PushLine(RB.X, RB.X + RB.w, { 0, 1, 0, 1 });
          Debug::PushWireframeSphere(RB.X + RB.w, 0.05f, { 0, 1, 0, 1 });
        }
        if(GameState->Physics.Switches.VisualizeV)
        {
          Debug::PushLine(RB.X, RB.X + RB.v, { 1, 1, 0, 1 });
          Debug::PushWireframeSphere(RB.X + RB.v, 0.05f, { 1, 1, 0, 1 });
        }
      }
    }

    // Actual physics work
    SimulateDynamics(&GameState->Physics);

    for(int i = 0; i < GameState->EntityCount; i++)
    {
      GameState->Entities[i].RigidBody   = GameState->Physics.RigidBodies[i];
      GameState->Entities[i].Transform.R = GameState->Physics.RigidBodies[i].q;
      GameState->Entities[i].Transform.T = GameState->Physics.RigidBodies[i].X;
    }
  }

  {
    // GameState->MMData.Params.DynamicParams = GameState->MMParams.DynamicParams;

    /* if(GetEntityAtIndex(GameState, &PlayerEntity, GameState->PlayerEntityIndex) &&
        GameState->MMData.FrameInfos.IsValid())
     {
       GameState->MMData.Animations.HardClear();
       for(int i = 0; i< GameState->MMData.Params.AnimRIDs.Count; i++)
       {
         GameState->MMData.Animations.Push(
           GameState->Resources.GetAnimation(GameState->MMData.Params.AnimRIDs[i]));
       }
       Gameplay::UpdatePlayer(PlayerEntity, &GameState->PlayerBlendStack,
                              GameState->TemporaryMemStack, Input, &GameState->Camera,
                              &GameState->MMData, &GameState->MMDebug, GameState->PlayerSpeed);
     }
     */
  }

  if(GameState->R.ShowLightPosition)
  {
    mat4 Mat4LightPosition = Math::Mat4Translate(GameState->R.LightPosition);
    Debug::PushGizmo(&GameState->Camera, Mat4LightPosition);
  }

  GameState->R.CumulativeTime += Input->dt;

  BEGIN_TIMED_BLOCK(AnimationSystem);
  // -----------ENTITY ANIMATION UPDATE-------------
  for(int e = 0; e < GameState->EntityCount; e++)
  {
    Anim::animation_controller* Controller               = GameState->Entities[e].AnimController;
    mat4                        CurrentEntityModelMatrix = GetEntityModelMatrix(GameState, e);
    if(Controller)
    {
      for(int i = 0; i < Controller->AnimStateCount; i++)
      {
        Controller->Animations[i] =
          (0 < Controller->AnimationIDs[i].Value)
            ? GameState->Resources.GetAnimation(Controller->AnimationIDs[i])
            : NULL;
      }

      Anim::UpdateController(Controller, Input->dt, Controller->BlendFunc,
                             &GameState->PlayerBlendStack);

      // TODO(Lukas): remove most parts of this code as it is repeated multiple times in different
      // locations
      for(int a = 0; a < Controller->AnimStateCount; a++)
      {
        const Anim::animation*       CurrentAnimation = Controller->Animations[a];
        const Anim::animation_state* CurrentState     = &Controller->States[a];

        if(!CurrentAnimation)
        {
          continue;
        }

        if(GameState->MMDebug.ShowRootTrajectories || GameState->MMDebug.ShowHipTrajectories)
        {
          const float AnimDuration = Anim::GetAnimDuration(CurrentAnimation);

          // Compute the index of the keyframe left of the playhead
          int PrevKeyframeIndex = 0;
          {
            float SampleTime = CurrentState->PlaybackRateSec *
                               (Controller->GlobalTimeSec - CurrentState->StartTimeSec);
            if(CurrentState->Loop && AnimDuration < SampleTime)
            {
              SampleTime = SampleTime - AnimDuration * (float)((int)(SampleTime / AnimDuration));
            }
            else if(AnimDuration < SampleTime)
            {
              SampleTime = AnimDuration;
            }

            for(int k = 0; k < CurrentAnimation->KeyframeCount - 1; k++)
            {
              if(SampleTime <= CurrentAnimation->SampleTimes[k + 1])
              {
                PrevKeyframeIndex = k;
                break;
              }
            }
          }

          mat4 Mat4InvRoot = Math::Mat4Ident();

          // Transform current pose into the space of the root bone
          if(GameState->PreviewAnimationsInRootSpace)
          {
            mat4    Mat4Root;
            int32_t HipBoneIndex = 0;
            // mat4    HipBindPose  = Controller->Skeleton->Bones[HipBoneIndex].BindPose;

            mat4 Mat4Hips =
              // Math::MulMat4(HipBindPose,
              TransformToMat4(
                CurrentAnimation->Transforms[PrevKeyframeIndex * CurrentAnimation->ChannelCount +
                                             HipBoneIndex]) /*)*/;

            Anim::GetRootAndInvRootMatrices(&Mat4Root, &Mat4InvRoot, Mat4Hips);
          }

          int FutureTrajectoryPointCount = (int)(GameState->MMDebug.TrajectoryDuration /
                                                 (AnimDuration / CurrentAnimation->KeyframeCount));

          int EndKeyframeIndex = MinInt32(PrevKeyframeIndex + FutureTrajectoryPointCount,
                                          CurrentAnimation->KeyframeCount - 1);
          int SamplePeriod =
            MaxInt32(1, (int)floorf(FutureTrajectoryPointCount /
                                    (float)GameState->MMDebug.TrajectorySampleCount));
          for(int i = PrevKeyframeIndex; i < EndKeyframeIndex - SamplePeriod; i += SamplePeriod)
          {
            int32_t HipBoneIndex = 0;
            vec3    LocalHipPositionA =
              CurrentAnimation->Transforms[HipBoneIndex + i * CurrentAnimation->ChannelCount].T;
            vec3 LocalHipPositionB =
              CurrentAnimation
                ->Transforms[HipBoneIndex + (i + SamplePeriod) * CurrentAnimation->ChannelCount]
                .T;

            LocalHipPositionA =
              Math::MulMat4Vec4(Mat4InvRoot, { LocalHipPositionA.X, LocalHipPositionA.Y,
                                               LocalHipPositionA.Z, 1 })
                .XYZ;
            LocalHipPositionB =
              Math::MulMat4Vec4(Mat4InvRoot, { LocalHipPositionB.X, LocalHipPositionB.Y,
                                               LocalHipPositionB.Z, 1 })
                .XYZ;

            if(GameState->MMDebug.ShowHipTrajectories)
            {
              vec3 HipPositionA = Math::MulMat4Vec4(CurrentEntityModelMatrix,
                                                    { LocalHipPositionA.X, LocalHipPositionA.Y,
                                                      LocalHipPositionA.Z, 1 })
                                    .XYZ;
              vec3 HipPositionB = Math::MulMat4Vec4(CurrentEntityModelMatrix,
                                                    { LocalHipPositionB.X, LocalHipPositionB.Y,
                                                      LocalHipPositionB.Z, 1 })
                                    .XYZ;
              Debug::PushLine(HipPositionA, HipPositionB, { 0, 0, 1, 1 });
            }

            if(GameState->MMDebug.ShowRootTrajectories)
            {
              vec3 RootPositionA =
                Math::MulMat4Vec4(CurrentEntityModelMatrix,
                                  { LocalHipPositionA.X, 0, LocalHipPositionA.Z, 1 })
                  .XYZ;
              vec3 RootPositionB =
                Math::MulMat4Vec4(CurrentEntityModelMatrix,
                                  { LocalHipPositionB.X, 0, LocalHipPositionB.Z, 1 })
                  .XYZ;
              Debug::PushLine(RootPositionA, RootPositionB, { 0, 1, 1, 1 });
            }
          }
        }
      }

      if(GetEntityMMDataIndex(e, &GameState->MMEntityData) != -1 ||
         GameState->PreviewAnimationsInRootSpace)
      {
        mat4    Mat4Root;
        mat4    Mat4InvRoot;
        int32_t HipBoneIndex    = 0;
        mat4    Mat4HipBindPose = Controller->Skeleton->Bones[HipBoneIndex].BindPose;
        mat4    Mat4Hips =
          Math::MulMat4(Controller->HierarchicalModelSpaceMatrices[HipBoneIndex], Mat4HipBindPose);

        // Debug::PushGizmo(&GameState->Camera, Math::MulMat4(CurrentEntityModelMatrix, Mat4Hips));

        Anim::GetRootAndInvRootMatrices(&Mat4Root, &Mat4InvRoot, Mat4Hips);
        for(int b = 0; b < Controller->Skeleton->BoneCount; b++)
        {
          Controller->HierarchicalModelSpaceMatrices[b] =
            Math::MulMat4(Mat4InvRoot, Controller->HierarchicalModelSpaceMatrices[b]);
        }
      }
      if(GameState->DrawActorSkeletons)
      {
        DrawSkeleton(Controller->Skeleton, Controller->HierarchicalModelSpaceMatrices,
                     GetEntityModelMatrix(GameState, e), GameState->BoneSphereRadius);
      }
    }
  }

  //----------ANIMATION EDITOR INTERACTION-----------
  if(Input->IsMouseInEditorMode && GameState->SelectionMode == SELECT_Bone &&
     GameState->AnimEditor.Skeleton)
  {
    AnimationEditorInteraction(GameState, Input);
  }
  END_TIMED_BLOCK(AnimationSystem);
  END_TIMED_BLOCK(Update);

  //---------------------RENDERING----------------------------
  BEGIN_TIMED_BLOCK(Render);

  // RENDER QUEUE SUBMISSION
  GameState->R.MeshInstanceCount = 0;
  for(int e = 0; e < GameState->EntityCount; e++)
  {
    Render::model* CurrentModel = GameState->Resources.GetModel(GameState->Entities[e].ModelID);
    if(!GameState->DrawActorMeshes && GameState->Entities[e].AnimController)
    {
      continue;
    }
    for(int m = 0; m < CurrentModel->MeshCount; m++)
    {
      mesh_instance MeshInstance = {};
      MeshInstance.Mesh          = CurrentModel->Meshes[m];
      MeshInstance.Material =
        GameState->Resources.GetMaterial(GameState->Entities[e].MaterialIDs[m]);
      MeshInstance.MVP            = GetEntityMVPMatrix(GameState, e);
      MeshInstance.PrevMVP        = GameState->PrevFrameMVPMatrices[e];
      MeshInstance.AnimController = GameState->Entities[e].AnimController;
      AddMeshInstance(&GameState->R, MeshInstance);
    }
  }

  // SHADED GIZMO SUBMISSION
  Debug::SubmitShadedBoneMeshInstances(GameState, NewPhongMaterial());

  BEGIN_GPU_TIMED_BLOCK(GeomPrePass);
  RenderGBufferDataToTextures(GameState);
  END_GPU_TIMED_BLOCK(GeomPrePass);

  // Saving previous frame entity MVP matrix (USED ONLY FOR MOTION BLUR)
  {
    for(int e = 0; e < GameState->EntityCount; e++)
    {
      GameState->PrevFrameMVPMatrices[e] = GetEntityMVPMatrix(GameState, e);
    }
  }

  BEGIN_GPU_TIMED_BLOCK(Shadowmapping);
  RenderShadowmapCascadesToTextures(GameState);
  END_GPU_TIMED_BLOCK(Shadowmapping);

  BEGIN_GPU_TIMED_BLOCK(SSAO);
  {
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.SSAOFBO);
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    if(GameState->R.RenderSSAO)
    {
      RenderSSAOToTexture(GameState);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  END_GPU_TIMED_BLOCK(SSAO);

  if(GameState->R.RenderVolumetricScattering)
  {
    BEGIN_GPU_TIMED_BLOCK(VolumetricLighting);
    RenderVolumeLightingToTexture(GameState);
    END_GPU_TIMED_BLOCK(VolumetricLighting);
  }

  {
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.HdrFBOs[0]);
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderMainSceneObjects(GameState);

    if(GameState->DrawCubemap)
    {
      RenderCubemap(GameState);
    }

    entity* SelectedEntity;
    if(Input->IsMouseInEditorMode && GetSelectedEntity(GameState, &SelectedEntity))
    {
      RenderObjectSelectionHighlighting(GameState, SelectedEntity);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // RENDERING MATERIAL PREVIEW TO TEXTURE
  // TODO(Lukas) only render preview if material uses time or parameters were changed
  if(GameState->CurrentMaterialID.Value > 0)
  {
    RenderMaterialPreviewToTexture(GameState);
  }

  //--------------Post Processing-----------------
  BEGIN_GPU_TIMED_BLOCK(PostProcessing);
  PerformPostProcessing(GameState);
  END_GPU_TIMED_BLOCK(PostProcessing);

  //---------------DEBUG DRAWING------------------
  BEGIN_TIMED_BLOCK(DebugDrawingSubmission);
  if(GameState->DrawDebugSpheres)
  {
    Debug::DrawWireframeSpheres(GameState);
  }
  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  if(GameState->DrawGizmos)
  {
    Debug::DrawGizmos(GameState);
  }
  if(GameState->DrawDebugLines)
  {
    Debug::DrawLines(GameState);
  }
  Debug::DrawQuads(GameState);
  Debug::ClearDrawArrays();
  END_TIMED_BLOCK(DebugDrawingSubmission);
  Text::ClearTextRequestCounts();

  END_TIMED_BLOCK(Render);
  READ_GPU_QUERY_TIMERS();
  END_TIMED_FRAME();
}
