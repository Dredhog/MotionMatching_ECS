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
    /*printf("sizeof(Anim::skeleton)            : %ld\n", sizeof(Anim::skeleton));
    printf("sizeof(mm_frame_info)             : %ld\n", sizeof(mm_frame_info));
    printf("sizeof(mm_controller_data)        : %ld\n", sizeof(mm_controller_data));
    printf("sizeof(mm_entity_data)            : %ld\n", sizeof(mm_entity_data));
    printf("sizeof(mm_aos_entity_data)        : %ld\n", sizeof(mm_aos_entity_data));
    printf("sizeof(blend_stack)               : %ld\n", sizeof(blend_stack));
    printf("sizeof(Anim::animation_player): %ld\n", sizeof(Anim::animation_player));
    printf("sizeof(transform)                 : %ld\n", sizeof(transform));
    printf("sizeof(pose_transform)            : %ld\n", sizeof(trajectory_transform));
    printf("alignof(pose_transform)           : %ld\n\n", alignof(trajectory_transform));*/

    INIT_GPU_TIMERS();
    TIMED_BLOCK(FirstInit);
    PartitionMemoryInitAllocators(&GameMemory, GameState);
    RegisterLoadInitialResources(GameState);
    SetGameStatePODFields(GameState);
    InitializeECS(GameState->PersistentMemStack, &GameState->ECSRuntime, &GameState->ECSWorld,
                  Mibibytes(1));

		//TODO(Lukas) MOVE THIS WHRE IT'S MORE APPROPIATE
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(3);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

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

	//Entity keyboard selection
  {
    bool NoEntitySelected = (GameState->SelectedEntityIndex < 0 ||
                             GameState->SelectedEntityIndex >= GameState->EntityCount);
    if(Input->j.EndedDown && Input->j.Changed)
    {
      if(NoEntitySelected && GameState->EntityCount > 0)
      {
        GameState->SelectedEntityIndex = GameState->EntityCount - 1;
        GameState->SelectedMeshIndex=0;
      }
      else if(GameState->SelectedEntityIndex > 0)
      {
        GameState->SelectedEntityIndex--;
        GameState->SelectedMeshIndex=0;
      }
    }
    else if(Input->k.EndedDown && Input->k.Changed)
    {
      if(NoEntitySelected && GameState->EntityCount > 0)
      {
        GameState->SelectedEntityIndex = 0;
        GameState->SelectedMeshIndex=0;
      }
      else if(GameState->SelectedEntityIndex < GameState->EntityCount - 1)
      {
        GameState->SelectedEntityIndex++;
        GameState->SelectedMeshIndex=0;
      }
    }
    else if(Input->v.EndedDown && Input->v.Changed)
    {
			GameState->SelectedEntityIndex = -1;
			GameState->SelectedEntityIndex = -1;
		}
  }

  {
    if(Input->f.EndedDown && Input->f.Changed)
    {
      GameState->Camera.OrbitSelected = !GameState->Camera.OrbitSelected;
    }
    entity* SelectedEntity = {};
    if(GameState->Camera.OrbitSelected && GetSelectedEntity(GameState, &SelectedEntity))
    {
      UpdateCamera(&GameState->Camera, SelectedEntity->Transform.T + vec3{ 0, 1, 0 }, Input);
			//Keeping the first person camera rotations up to date
      {
        const float DegToRad         = float(M_PI) / 180.0f;
        GameState->Camera.Rotation.X = asinf(GameState->Camera.Forward.Y) / DegToRad;
        GameState->Camera.Rotation.Y =
          (float(M_PI) + atan2f(GameState->Camera.Forward.X, GameState->Camera.Forward.Z)) /
          DegToRad;
      }
    }
    else
    {
      UpdateCamera(&GameState->Camera, Input);
    }
  }

  // Editor
  if(Input->IsMouseInEditorMode)
  {
    EditWorldAndInteractWithGUI(GameState, Input);
  }

  //--------------------WORLD UPDATE------------------------

  // Runtime motion matching start to finish
  {
    mm_entity_data&             MMEntityData        = GameState->MMEntityData;
    mm_debug_settings&          MMDebug             = GameState->MMDebug;
    spline_system&              SplineSystem        = GameState->SplineSystem;
    entity*                     Entities            = GameState->Entities;
    int32_t                     DebugEntityCount    = GameState->EntityCount;
    Resource::resource_manager& Resources           = GameState->Resources;
    vec3                        CameraForward       = GameState->Camera.Forward;
    Memory::stack_allocator*    TempStack           = GameState->TemporaryMemStack;
    mm_timeline_state*          MMTimelineState     = &GameState->MMTimelineState;
    int32_t                     SelectedEntityIndex = GameState->SelectedEntityIndex;
    testing_system*             TestingSystem       = &GameState->TestingSystem;
    bool                        AllowWASDControls   = GameState->Camera.OrbitSelected;

    int ActiveInputControlledCount;
    int FirstSplineControlledIndex;
    int ActiveSplineControlledCount;
    SortMMEntityDataByUsage(&ActiveInputControlledCount, &FirstSplineControlledIndex,
                            &ActiveSplineControlledCount, &MMEntityData, &SplineSystem);
    int ActiveControllerCount = ActiveInputControlledCount + ActiveSplineControlledCount;

    FetchMMControllerDataPointers(&Resources, MMEntityData.MMControllers,
                                  MMEntityData.MMControllerRIDs, ActiveControllerCount);
    FetchSkeletonPointers(MMEntityData.Skeletons, MMEntityData.EntityIndices, Entities,
                          ActiveControllerCount);
    FetchAnimationPointers(&Resources, MMEntityData.MMControllers, MMEntityData.BlendStacks,
                           ActiveControllerCount);
    PlayAnimsIfBlendStacksAreEmpty(MMEntityData.BlendStacks, MMEntityData.AnimPlayerTimes,
                                   MMEntityData.MMControllers, ActiveControllerCount);

    entity_goal_input InputOverrides[MAX_SIMULTANEOUS_TEST_COUNT];
    int               InputOverrideCount = 0;
#if 1
    for(int i = 0; i < TestingSystem->ActiveTests.Count; i++)
    {
      active_test& Test = TestingSystem->ActiveTests[i];
      if(Test.Type == TEST_FacingChange)
      {
        entity_goal_input Temp = {};
        {
          Temp.EntityIndex = Test.EntityIndex;
          Temp.WorldDir    = Test.FacingTest.TargetWorldFacing;
        }
        InputOverrides[InputOverrideCount++] = Temp;
      }
    }
#endif

    GenerateGoalsFromInput(&MMEntityData.AnimGoals[0], &MMEntityData.MirroredAnimGoals[0],
                           &MMEntityData.Trajectories[0], TempStack, &MMEntityData.BlendStacks[0],
                           &MMEntityData.AnimPlayerTimes[0], &MMEntityData.Skeletons[0],
                           &MMEntityData.MMControllers[0], &MMEntityData.InputControllers[0],
                           &MMEntityData.EntityIndices[0], ActiveInputControlledCount, Entities,
                           Input, InputOverrides, InputOverrideCount, CameraForward,
                           AllowWASDControls);
    AssertSplineIndicesAndClampWaypointIndices(&MMEntityData
                                                  .SplineStates[FirstSplineControlledIndex],
                                               ActiveSplineControlledCount,
                                               SplineSystem.Splines.Elements,
                                               SplineSystem.Splines.Count);
    GenerateGoalsFromSplines(TempStack, &MMEntityData.AnimGoals[FirstSplineControlledIndex],
                             &MMEntityData.MirroredAnimGoals[FirstSplineControlledIndex],
                             &MMEntityData.Trajectories[FirstSplineControlledIndex],
                             &MMEntityData.SplineStates[FirstSplineControlledIndex],
                             &MMEntityData.InputControllers[FirstSplineControlledIndex],
                             &MMEntityData.MMControllers[FirstSplineControlledIndex],
                             &MMEntityData.BlendStacks[FirstSplineControlledIndex],
                             &MMEntityData.AnimPlayerTimes[FirstSplineControlledIndex],
                             &MMEntityData.Skeletons[FirstSplineControlledIndex],
                             &MMEntityData.EntityIndices[FirstSplineControlledIndex],
                             ActiveSplineControlledCount, SplineSystem.Splines.Elements,
                             SplineSystem.Splines.Count, Entities);
    MotionMatchGoals(MMEntityData.BlendStacks, MMEntityData.LastMatchedGoals,
                     MMEntityData.LastMatchedTransforms, MMEntityData.AnimGoals,
                     MMEntityData.MirroredAnimGoals, MMEntityData.MMControllers,
                     MMEntityData.AnimPlayerTimes, MMEntityData.EntityIndices,
                     ActiveControllerCount, Entities);
    DrawGoalFrameInfos(MMEntityData.AnimGoals, MMEntityData.EntityIndices, ActiveControllerCount,
                       Entities, &MMDebug.CurrentGoal);
    DrawGoalFrameInfos(MMEntityData.LastMatchedGoals, MMEntityData.BlendStacks,
                       MMEntityData.LastMatchedTransforms, ActiveControllerCount,
                       &MMDebug.MatchedGoal, { 1, 1, 0 }, { 0, 1, 0 }, { 1, 0, 0 });
    ComputeLocalRootMotion(MMEntityData.OutDeltaRootMotions, MMEntityData.Skeletons,
                           MMEntityData.BlendStacks, MMEntityData.AnimPlayerTimes,
                           ActiveControllerCount, Input->dt);
    if(MMDebug.ApplyRootMotion)
      ApplyRootMotion(Entities, MMEntityData.Trajectories, MMEntityData.OutDeltaRootMotions,
                      MMEntityData.EntityIndices, ActiveControllerCount);
    if(MMDebug.ShowSmoothGoals)
      DrawControlTrajectories(MMEntityData.Trajectories, MMEntityData.InputControllers,
                              MMEntityData.EntityIndices, ActiveControllerCount, Entities);
    AdvanceAnimPlayerTimes(MMEntityData.AnimPlayerTimes, ActiveControllerCount, Input->dt);
    RemoveBlendedOutAnimsFromBlendStacks(MMEntityData.BlendStacks, MMEntityData.AnimPlayerTimes,
                                         ActiveControllerCount);
    OverwriteSelectedMMEntity(MMEntityData.BlendStacks, MMEntityData.AnimPlayerTimes,
                              MMTimelineState, Entities, &MMEntityData, SelectedEntityIndex);
    CopyMMAnimDataToAnimationPlayers(Entities, MMEntityData.BlendStacks,
                                     MMEntityData.AnimPlayerTimes, MMEntityData.EntityIndices,
                                     ActiveControllerCount);

    int FirstInactiveControllerIndex = ActiveControllerCount;
    int InactiveControllerCount      = MMEntityData.Count - ActiveControllerCount;
    ClearAnimationData(&MMEntityData.BlendStacks[FirstInactiveControllerIndex],
                       &MMEntityData.EntityIndices[FirstInactiveControllerIndex],
                       InactiveControllerCount, Entities, DebugEntityCount);
  }
  // TODO(Lukas) this late camera update invalidates the debug gizmos drawn between here and the
  // first camera update. Make the gizmos use the VP matrix, when submitting the drawing primitives
  // at the end of the frame.
  {
    entity* SelectedEntity = {};
    if(GameState->Camera.OrbitSelected && GetSelectedEntity(GameState, &SelectedEntity))
    {
      UpdateCamera(&GameState->Camera, SelectedEntity->Transform.T + vec3{ 0, 1, 0 }, Input);
      // Keeping the first person camera rotations up to date
      {
        const float DegToRad         = float(M_PI) / 180.0f;
        GameState->Camera.Rotation.X = asinf(GameState->Camera.Forward.Y) / DegToRad;
        GameState->Camera.Rotation.Y =
          (float(M_PI) + atan2f(GameState->Camera.Forward.X, GameState->Camera.Forward.Z)) /
          DegToRad;
      }
    }
  }

  if(GameState->UpdatePhysics)
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

  // Waypoint debug visualizaiton
	const vec3 VerticalSplineOffset = {0,0.02f,0};
  for(int i = 0; i < GameState->SplineSystem.Splines.Count; i++)
  {
    waypoint PreviousWaypoint = {};
    for(int j = 0; j < GameState->SplineSystem.Splines[i].Waypoints.Count; j++)
    {
      waypoint CurrentWaypoint = GameState->SplineSystem.Splines[i].Waypoints[j];
      vec4 WaypointColor = { 0.2f, 0.2f, 1, 1 };
      if(j > 0)
      {
        if(GameState->DrawTrajectoryLines)
        {
          Debug::PushLine(PreviousWaypoint.Position + VerticalSplineOffset,
                          CurrentWaypoint.Position + VerticalSplineOffset, WaypointColor,
                          GameState->OverlaySplines);
        }
      }
      if(GameState->SplineSystem.SelectedSplineIndex == i &&
         GameState->SplineSystem.SelectedWaypointIndex == j)
      {
        WaypointColor = { 1.0f, 1.0f, 0, 1 };
      }
      if(GameState->DrawTrajectoryWaypoints)
      {
        Debug::PushWireframeSphere(CurrentWaypoint.Position, 0.1f, WaypointColor);
      }
      PreviousWaypoint = CurrentWaypoint;
    }
  }

	// Spline visualization
  for(int i = 0; i < GameState->SplineSystem.Splines.Count && GameState->DrawTrajectorySplines; i++)
  {
    if(GameState->SplineSystem.Splines[i].Waypoints.Count == 0)
    {
      continue;
    }

    movement_spline& Spline = GameState->SplineSystem.Splines[i];

    vec3       PreviousPoint = Spline.CatmullRomPoint(0, 0);
    const bool Loop          = GameState->DrawSplinesLooped;
    for(int j = 1; j < Spline.Waypoints.Count + (Loop ? 1 : 0); j++)
    {
      const int SubdivisionCount = 10;
      for(int k = 0; k < SubdivisionCount; k++)
      {
        float t            = float(k + 1) / SubdivisionCount;
        vec3  CurrentPoint = Spline.CatmullRomPoint(j % Spline.Waypoints.Count, t, Loop);
        Debug::PushLine(PreviousPoint + VerticalSplineOffset, CurrentPoint + VerticalSplineOffset,
                        { 1, 0.2f, 0.2f, 1 }, GameState->OverlaySplines);
        PreviousPoint = CurrentPoint;
      }
    }
  }

  //------------Performing Measurements------------
  {
    // Measure Ground Truth Foot Skate
    testing_system& Tests = GameState->TestingSystem;
    for(int i = 0; i < Tests.ActiveTests.Count; i++)
    {
      active_test& Test = Tests.ActiveTests[i];
      if(Test.Type == TEST_AnimationFootSkate)
      {
        entity*          Entity = &GameState->Entities[Test.EntityIndex];
        Anim::animation* Anim = GameState->Resources.GetAnimation(Test.FootSkateTest.AnimationRID);

        foot_skate_data_row FootSkateTableRow =
          MeasureFootSkate(GameState->TemporaryMemStack, &Test.FootSkateTest,
                           Entity->AnimPlayer->Skeleton, Anim, Test.FootSkateTest.ElapsedTime,
                           1 / 60.0f);
        AddRow(&Test.DataTable, &FootSkateTableRow, sizeof(FootSkateTableRow));
        Test.FootSkateTest.ElapsedTime += Input->dt;
        if(Test.FootSkateTest.ElapsedTime >= Anim::GetAnimDuration(Anim))
        {
          int32_t TestIndex = Tests.GetEntityTestIndex(Test.EntityIndex, Test.Type);
          Tests.WriteTestToCSV(TestIndex);
          Tests.DestroyTest(&GameState->Resources, TestIndex);
          i--;
        }
      }
    }

    // Measure Controller Foot Skate
    for(int i = 0; i < Tests.ActiveTests.Count; i++)
    {
      active_test& Test = Tests.ActiveTests[i];
      if(Test.Type == TEST_ControllerFootSkate)
      {
        entity* Entity        = &GameState->Entities[Test.EntityIndex];
        int32_t MMEntityIndex = GetEntityMMDataIndex(Test.EntityIndex, &GameState->MMEntityData);
        mm_aos_entity_data MMEntity = GetAOSMMDataAtIndex(MMEntityIndex, &GameState->MMEntityData);
        blend_stack*       BlendStack = MMEntity.BlendStack;
        const Anim::skeleton_mirror_info* MirrorInfo =
          &(**MMEntity.MMController).Params.DynamicParams.MirrorInfo;

        foot_skate_data_row FootSkateTableRow =
          MeasureFootSkate(&Test.FootSkateTest, Entity->AnimPlayer, *MMEntity.MMController,
                           MirrorInfo, BlendStack, TransformToMat4(Entity->Transform),
                           *MMEntity.OutDeltaRootMotion, Test.FootSkateTest.ElapsedTime, Input->dt);
        AddRow(&Test.DataTable, &FootSkateTableRow, sizeof(FootSkateTableRow));
        Test.FootSkateTest.ElapsedTime += Input->dt;
      }
    }

    // Measure Direction Goal Reach Time
    for(int i = 0; i < Tests.ActiveTests.Count; i++)
    {
      active_test& Test = Tests.ActiveTests[i];
      if(Test.Type == TEST_FacingChange)
      {
        entity* Entity          = &GameState->Entities[Test.EntityIndex];
        mat3    EntityRotMatrix = Math::QuatToMat3(Entity->Transform.R);
        mat3    InvEntityRotMatrix;
        quat    InvR = Entity->Transform.R;
        InvR.V *= -1;
        InvEntityRotMatrix = Math::QuatToMat3(InvR);

        vec3 CurrentFacing = Math::Normalized(EntityRotMatrix.Z);

        const float DegToRad = float(M_PI) / 180.0f;

        Debug::PushLine(Entity->Transform.T, Entity->Transform.T + CurrentFacing, { 0, 0, 0, 1 });

        facing_test& FacingTest = Test.FacingTest;
        {
          FacingTest.ElapsedTime += Input->dt;
          if(FacingTest.HasActiveCase)
          {
            Debug::PushLine(Entity->Transform.T, Entity->Transform.T + FacingTest.TargetWorldFacing,
                            { 1, 1, 0, 1 });
            if(FacingTest.ElapsedTime > FacingTest.MaxWaitTime) // Failed Test
            {
              facing_turn_time_data_row ResultRow = {};
              {
                ResultRow.TimeTaken = FacingTest.ElapsedTime,
                ResultRow.Passed    = 0;
                ResultRow.LocalTargetAngle = FacingTest.TestStartLocalTargetAngle;
                ResultRow.AngleThreshold = FacingTest.TargetAngleThreshold;
              }
              AddRow(&Test.DataTable, &ResultRow, sizeof(ResultRow));
              FacingTest.HasActiveCase = false;
            }
            else
            {
              // Compute target local facing
              vec3 TargetLocalFacing =
                Math::Normalized(Math::MulMat3Vec3(FacingTest.InvTargetBasis, CurrentFacing));

              // Compute target local angle
              float TargetLocalAngle = atan2f(TargetLocalFacing.X, TargetLocalFacing.Z);
              if(AbsFloat(TargetLocalAngle) <=
                 DegToRad * FacingTest.TargetAngleThreshold) // Passed test
              {
                facing_turn_time_data_row ResultRow = {};
                {
                    ResultRow.TimeTaken = FacingTest.ElapsedTime;
                    ResultRow.Passed    = 1;
                    ResultRow.LocalTargetAngle = FacingTest.TestStartLocalTargetAngle;
                    ResultRow.AngleThreshold = FacingTest.TargetAngleThreshold;
                }
                AddRow(&Test.DataTable, &ResultRow, sizeof(ResultRow));
                FacingTest.HasActiveCase = false;
              }
            }
          }
          else if(FacingTest.RemainingCaseCount > 0) // Start New Case
          {
            FacingTest.ElapsedTime = 0.0f;
            // Get current world angle
            float CurrentWorldAngle = atan2f(CurrentFacing.X, CurrentFacing.Z);

            // Generate goal local angle
            FacingTest.TestStartLocalTargetAngle =
              FacingTest.MaxTestAngle * ((float(rand()) / RAND_MAX) * 2 - 1.0f);

            // Genrate goal world angle
            float TargetWorldAngle =
              CurrentWorldAngle + DegToRad * FacingTest.TestStartLocalTargetAngle;

            // Generate goal world facing
            FacingTest.TargetWorldFacing = { sinf(TargetWorldAngle), 0, cosf(TargetWorldAngle) };

            // Generate inv target basis
            FacingTest.InvTargetBasis = Math::Mat3RotateY(-TargetWorldAngle / DegToRad);
            FacingTest.HasActiveCase  = true;

            FacingTest.RemainingCaseCount--;
          }
          else
          {
            int32_t TestIndex = Tests.GetEntityTestIndex(Test.EntityIndex, Test.Type);
            Tests.WriteTestToCSV(TestIndex);
            Tests.DestroyTest(&GameState->Resources, TestIndex);
            i--;
          }
        }
      }
    }

    // Measure Deviation From Trajectory
    for(int i = 0; i < Tests.ActiveTests.Count; i++)
    {
      active_test& Test = Tests.ActiveTests[i];
			if(Test.Type == TEST_TrajectoryFollowing)
      {
        entity* Entity        = &GameState->Entities[Test.EntityIndex];
        int32_t MMEntityIndex = GetEntityMMDataIndex(Test.EntityIndex, &GameState->MMEntityData);
        mm_aos_entity_data MMEntity = GetAOSMMDataAtIndex(MMEntityIndex, &GameState->MMEntityData);

        trajectory_follow_data_row TrajectoryFollowTableRow =
          MeasureTrajectoryFollowing(Entity->Transform, MMEntity.SplineState,
                                     &GameState->SplineSystem
                                        .Splines[MMEntity.SplineState->SplineIndex],
                                     Test.FollowTest.ElapsedTime, Input->dt);

        AddRow(&Test.DataTable, &TrajectoryFollowTableRow, sizeof(TrajectoryFollowTableRow));
        Test.FollowTest.ElapsedTime += Input->dt;
      }
    }
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
    Anim::animation_player* Controller               = GameState->Entities[e].AnimPlayer;
    mat4                    CurrentEntityModelMatrix = GetEntityModelMatrix(GameState, e);
    if(Controller)
    {
      for(int i = 0; i < Controller->AnimStateCount; i++)
      {
        if(Controller->AnimationIDs[i].Value > 0)
        {
          Controller->Animations[i] =
            GameState->Resources.GetAnimation(Controller->AnimationIDs[i]);
        }
      }

      int MMEntityIndex = -1;
      if((MMEntityIndex = GetEntityMMDataIndex(e, &GameState->MMEntityData)) != -1)
      {
        mm_aos_entity_data MMEntity = GetAOSMMDataAtIndex(MMEntityIndex, &GameState->MMEntityData);
        playback_info      PlaybackInfo = {};
        PlaybackInfo.BlendStack         = MMEntity.BlendStack;
        if(MMEntity.MMControllerRID->Value > 0 && MMEntity.MMController)
        {
          PlaybackInfo.MirrorInfo = &(*MMEntity.MMController)->Params.DynamicParams.MirrorInfo;
          Anim::UpdatePlayer(Controller, Input->dt, Controller->BlendFunc, &PlaybackInfo);
        }
        else
        {
          assert(Controller->BlendFunc == NULL);
          Anim::UpdatePlayer(Controller, Input->dt, Controller->BlendFunc);
        }
      }
      else
      {
        //assert(Controller->BlendFunc == NULL);
				float Proxydt = Input->dt;
        Anim::UpdatePlayer(Controller, Input->dt, Controller->BlendFunc, &Proxydt);
      }

      // TODO(Lukas): remove most parts of this code as it is repeated multiple times in different
      // locations
      for(int a = 0; a < Controller->AnimStateCount; a++)
      {
        const Anim::animation*       CurrentAnimation = Controller->Animations[a];
        const Anim::animation_state* CurrentState     = &Controller->States[a];

        assert(CurrentAnimation);

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
          float XMirrorScale      = CurrentState->Mirror ? -1 : 1;
          mat4  MirrorScaleMatrix = Math::Mat4Scale(XMirrorScale, 1, 1);

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
            Mat4Hips = Math::MulMat4(MirrorScaleMatrix, Mat4Hips);

            Anim::GetRootAndInvRootMatrices(&Mat4Root, &Mat4InvRoot, Mat4Hips);
          }


#if 1
          if(!GameState->PreviewAnimationsInRootSpace)
          {
            int32_t HipBoneIndex = 0;
            vec3    RootP =
              CurrentAnimation
                ->Transforms[PrevKeyframeIndex * CurrentAnimation->ChannelCount + HipBoneIndex]
                .T;
            RootP.Y = 0;
            RootP.X *= XMirrorScale;
            Debug::PushWireframeSphere(RootP, 0.02f, { 1, 0, 0, 1 });
          }
#endif


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
              Math::MulMat4Vec4(Mat4InvRoot, { XMirrorScale * LocalHipPositionA.X,
                                               LocalHipPositionA.Y, LocalHipPositionA.Z, 1 })
                .XYZ;
            LocalHipPositionB =
              Math::MulMat4Vec4(Mat4InvRoot, { XMirrorScale * LocalHipPositionB.X,
                                               LocalHipPositionB.Y, LocalHipPositionB.Z, 1 })
                .XYZ;

            const bool OverlayTrajectories = false;
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
              Debug::PushLine(HipPositionA, HipPositionB, { 0, 0, 1, 1 }, OverlayTrajectories);
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
              Debug::PushLine(RootPositionA, RootPositionB, { 1, 0, 0, 1 }, OverlayTrajectories);
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
    if(!GameState->DrawActorMeshes && GameState->Entities[e].AnimPlayer)
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
      MeshInstance.AnimPlayer = GameState->Entities[e].AnimPlayer;
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

		glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if(GameState->DrawDebugSpheres)
		{
			Debug::DrawWireframeSpheres(GameState);
		}
		if(GameState->DrawDebugLines)
		{
			Debug::DrawDepthTestedLines(GameState);
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
  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  if(GameState->DrawGizmos)
  {
    Debug::DrawGizmos(GameState);
  }
  if(GameState->DrawDebugLines)
  {
    Debug::DrawOverlayLines(GameState);
  }
  Debug::DrawQuads(GameState);
  Debug::ClearDrawArrays();
  END_TIMED_BLOCK(DebugDrawingSubmission);
  Text::ClearTextRequestCounts();

  END_TIMED_BLOCK(Render);
  READ_GPU_QUERY_TIMERS();
  END_TIMED_FRAME();
}
