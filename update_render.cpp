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

extern bool g_VisualizeContactPoints;
extern bool g_VisualizeContactManifold;

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  BEGIN_FRAME();

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

    if(!GameState->UseHotReloading)
    {
      TIMED_BLOCK(FilesystemUpdate);
      GameState->Resources.UpdateHardDriveAssetPathLists();
      GameState->Resources.DeleteUnused();
      GameState->Resources.ReloadModified();
    }
  }

  BEGIN_TIMED_BLOCK(Update)
  if(GameState->UseHotReloading)
  {
    TIMED_BLOCK(FilesystemUpdate);
    GameState->Resources.UpdateHardDriveAssetPathLists();
    GameState->Resources.DeleteUnused();
    GameState->Resources.ReloadModified();
  }

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

        GameState->Physics.RigidBodies[i].q = GameState->Entities[i].Transform.Rotation;
        GameState->Physics.RigidBodies[i].X = GameState->Entities[i].Transform.Translation;

        GameState->Physics.RigidBodies[i].R =
          Math::Mat4ToMat3(Math::Mat4Rotate(GameState->Entities[i].Transform.Rotation));

        GameState->Physics.RigidBodies[i].Mat4Scale =
          Math::Mat4Scale(GameState->Entities[i].Transform.Scale);

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
      GameState->Entities[i].RigidBody             = GameState->Physics.RigidBodies[i];
      GameState->Entities[i].Transform.Rotation    = GameState->Physics.RigidBodies[i].q;
      GameState->Entities[i].Transform.Translation = GameState->Physics.RigidBodies[i].X;
    }
  }

  if(GameState->PlayerEntityIndex != -1)
  {
    entity* PlayerEntity = {};
    if(GetEntityAtIndex(GameState, &PlayerEntity, GameState->PlayerEntityIndex))
    {
      Gameplay::UpdatePlayer(PlayerEntity, Input, &GameState->Camera, &GameState->MMSet);
    }
  }

  if(GameState->R.ShowLightPosition)
  {
    mat4 Mat4LightPosition = Math::Mat4Translate(GameState->R.LightPosition);
    Debug::PushGizmo(&GameState->Camera, &Mat4LightPosition);
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
        assert(0 < Controller->AnimationIDs[i].Value);
        Controller->Animations[i] = GameState->Resources.GetAnimation(Controller->AnimationIDs[i]);
      }

      Anim::UpdateController(Controller, Input->dt, Controller->BlendFunc);

      // Todo(Lukas): remove most parts of this code as it is repeated multiple times in different
      // locations
      for(int i = 0; i < Controller->AnimStateCount; i++)
      {
        const Anim::animation*       CurrentAnimation = Controller->Animations[i];
        const Anim::animation_state* CurrentState     = &Controller->States[i];

        // Transform current pose into the space of the root bone
        mat4 Mat4InvRoot = Math::Mat4Ident();

				if(GameState->MMTransformToRootSpace)
        {
          const int HipBoneIndex = 0;
          mat4      Mat4Hip      = Controller->HierarchicalModelSpaceMatrices[HipBoneIndex];

          vec3 Up      = { 0, 1, 0 };
          vec3 Right   = Math::Normalized(Math::Cross(Up, Mat4Hip.Z));
          vec3 Forward = Math::Cross(Right, Up);

          mat4 Mat4Root = Math::Mat4Ident();
          Mat4Root.T    = { Mat4Hip.T.X, 0, Mat4Hip.T.Z };
          Mat4Root.Y    = Up;
          Mat4Root.X    = Right;
          Mat4Root.Z    = Forward;

          Debug::PushGizmo(&GameState->Camera, &Mat4Root);
          Mat4InvRoot = Math::InvMat4(Mat4Root);

          for(int b = 0; b < Controller->Skeleton->BoneCount; b++)
          {
            Controller->HierarchicalModelSpaceMatrices[b] =
              Math::MulMat4(Math::InvMat4(Mat4Root), Controller->HierarchicalModelSpaceMatrices[b]);
          }
        }

        const float AnimDuration =
          (CurrentAnimation->SampleTimes[CurrentAnimation->KeyframeCount - 1] -
           CurrentAnimation->SampleTimes[0]);

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

        int FutureTrajectoryPointCount = 0;
        {
          FutureTrajectoryPointCount = (int)(GameState->TrajectoryLengthInTime /
                                             (AnimDuration / CurrentAnimation->KeyframeCount));
        }

        int EndKeyframeIndex = MinInt32(PrevKeyframeIndex + FutureTrajectoryPointCount,
                                        CurrentAnimation->KeyframeCount - 1);
        int SamplePeriod =
          (int)floorf(FutureTrajectoryPointCount / (float)GameState->TrajectorySampleCount);
        for(int i = PrevKeyframeIndex; i < EndKeyframeIndex - SamplePeriod; i += SamplePeriod)
        {
          vec3 LocalHipPositionA =
            CurrentAnimation->Transforms[0 + i * CurrentAnimation->ChannelCount].Translation;
          vec3 LocalHipPositionB =
            CurrentAnimation->Transforms[0 + (i + SamplePeriod) * CurrentAnimation->ChannelCount]
              .Translation;

          LocalHipPositionA =
            Math::MulMat4Vec4(Mat4InvRoot,
                              { LocalHipPositionA.X, LocalHipPositionA.Y, LocalHipPositionA.Z, 1 })
              .XYZ;
          LocalHipPositionB =
            Math::MulMat4Vec4(Mat4InvRoot,
                              { LocalHipPositionB.X, LocalHipPositionB.Y, LocalHipPositionB.Z, 1 })
              .XYZ;

          vec3 HipPositionA =
            Math::MulMat4Vec4(CurrentEntityModelMatrix,
                              { LocalHipPositionA.X, LocalHipPositionA.Y, LocalHipPositionA.Z, 1 })
              .XYZ;
          vec3 HipPositionB =
            Math::MulMat4Vec4(CurrentEntityModelMatrix,
                              { LocalHipPositionB.X, LocalHipPositionB.Y, LocalHipPositionB.Z, 1 })
              .XYZ;
          vec3 RootPositionA = Math::MulMat4Vec4(CurrentEntityModelMatrix,
                                                 { LocalHipPositionA.X, 0, LocalHipPositionA.Z, 1 })
                                 .XYZ;
          vec3 RootPositionB = Math::MulMat4Vec4(CurrentEntityModelMatrix,
                                                 { LocalHipPositionB.X, 0, LocalHipPositionB.Z, 1 })
                                 .XYZ;

          //Debug::PushLine(HipPositionA, HipPositionB, { 0, 0, 1, 1 });
          Debug::PushLine(RootPositionA, RootPositionB, { 0, 1, 1, 1 });
        }
      }

      for(int b = 0; b < Controller->Skeleton->BoneCount; b++)
      {
        mat4 Mat4Bone = Math::MulMat4(Anim::TransformToMat4(&GameState->Entities[e].Transform),
                                      Math::MulMat4(Controller->HierarchicalModelSpaceMatrices[b],
                                                    Controller->Skeleton->Bones[b].BindPose));
        vec3 Position = Math::GetMat4Translation(Mat4Bone);

        if(0 < b)
        {
          int  ParentIndex = Controller->Skeleton->Bones[b].ParentIndex;
          mat4 Mat4Parent =
            Math::MulMat4(Anim::TransformToMat4(&GameState->Entities[e].Transform),
                          Math::MulMat4(Controller->HierarchicalModelSpaceMatrices[ParentIndex],
                                        Controller->Skeleton->Bones[ParentIndex].BindPose));
          mat4 Mat4Root = Math::MulMat4(Anim::TransformToMat4(&GameState->Entities[e].Transform),
                                        Math::MulMat4(Controller->HierarchicalModelSpaceMatrices[0],
                                                      Controller->Skeleton->Bones[0].BindPose));
          vec3 ParentPosition = Math::GetMat4Translation(Mat4Parent);

#define USE_DIAMOND_VISUALIZATION 1
#if USE_DIAMOND_VISUALIZATION
          float BoneLength    = Math::Length(Position - ParentPosition);
          vec3  ParentToChild = Math::Normalized(Position - ParentPosition);

          vec3 Forward = Math::Normalized(
            Math::Cross(ParentToChild, { Mat4Root._11, Mat4Root._12, Mat4Root._13 }));
          vec3 Right = Math::Normalized(Math::Cross(ParentToChild, Forward));

          mat4 DiamondMatrix = Mat4Parent;

          DiamondMatrix.X = Right;
          DiamondMatrix.Y = ParentToChild;
          DiamondMatrix.Z = Forward;

          Debug::PushShadedBone(DiamondMatrix, BoneLength);
#else
          Debug::PushLine(Position, ParentPosition);
#endif
        }
        Debug::PushWireframeSphere(Position, GameState->BoneSphereRadius);
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
  Text::ClearTextRequestCounts();
  END_TIMED_BLOCK(DebugDrawingSubmission);

  END_TIMED_BLOCK(Render);
  READ_GPU_QUERY_TIMERS();
  END_FRAME();
}
