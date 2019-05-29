#include "entity_animation_control.h"
#include "debug_drawing.h"

void DrawFrameInfo(mm_frame_info AnimGoal, mat4 CoordinateFrame,
                   mm_info_debug_settings DebugSettings, vec3 BoneColor, vec3 VelocityColor,
                   vec3 TrajectoryColor, vec3 DirectionColor);

void DrawTrajectory(mat4 CoordinateFrame, const trajectory* Trajectory, vec3 PastColor,
                    vec3 PresentColor, vec3 FutureColor);

void
SetDefaultMMControllerFileds(mm_aos_entity_data* MMEntityData)
{
  *MMEntityData->MMControllerRID = {};
  *MMEntityData->BlendStack      = {};
  *MMEntityData->EntityIndex     = -1;
  *MMEntityData->FollowSpline    = false;
  *MMEntityData->SplineState     = { };
  {
    MMEntityData->SplineState->SplineIndex       = -1;
    MMEntityData->SplineState->NextWaypointIndex = 0;
    MMEntityData->SplineState->Loop              = true;
    MMEntityData->SplineState->MovingInPositive  = true;
  }

  InitTrajectory(MMEntityData->Trajectory);
  *MMEntityData->InputController = {};
  {
    MMEntityData->InputController->MaxSpeed      = 1.0f;
    MMEntityData->InputController->PositionBias  = 0.08f;
    MMEntityData->InputController->DirectionBias = 2;
    MMEntityData->InputController->UseStrafing   = false;
    MMEntityData->InputController->UseSmoothGoal = true;
  }

  *MMEntityData->Skeleton             = NULL;
  *MMEntityData->MMController         = NULL;
  *MMEntityData->LastMatchedGoal      = {};
  *MMEntityData->MirroredAnimGoal     = {};
  *MMEntityData->AnimGoal             = {};
  *MMEntityData->LastMatchedTransform = IdentityTransform();
  *MMEntityData->AnimPlayerTime       = 0;
}

void
CopyMMEntityData(int32_t DestIndex, int32_t SourceIndex, mm_entity_data* MMEntityData)
{
  mm_aos_entity_data Dest   = GetAOSMMDataAtIndex(DestIndex, MMEntityData);
  mm_aos_entity_data Source = GetAOSMMDataAtIndex(SourceIndex, MMEntityData);
  CopyAOSMMEntityData(&Dest, &Source);
}

void
RemoveMMControllerDataAtIndex(entity* Entities, int32_t MMControllerIndex,
                              Resource::resource_manager* Resources, mm_entity_data* MMEntityData)
{
  assert(0 <= MMControllerIndex && MMControllerIndex < MMEntityData->Count);

  mm_aos_entity_data RemovedController = GetAOSMMDataAtIndex(MMControllerIndex, MMEntityData);
  if(RemovedController.MMControllerRID->Value > 0)
  {
    Resources->MMControllers.RemoveReference(*RemovedController.MMControllerRID);
  }
  {
    Anim::animation_player* AnimPlayer =
      Entities[*RemovedController.EntityIndex].AnimPlayer;
    assert(AnimPlayer);
    AnimPlayer->BlendFunc = NULL;
    for(int i = 0; i < ANIM_PLAYER_MAX_ANIM_COUNT; i++)
    {
      assert(AnimPlayer->AnimationIDs[i].Value == 0);
      AnimPlayer->Animations[i] = NULL;
      AnimPlayer->States[i]     = {};
    }
    AnimPlayer->AnimStateCount = 0;
  }

  mm_aos_entity_data LastController = GetAOSMMDataAtIndex(MMEntityData->Count - 1, MMEntityData);
  CopyAOSMMEntityData(&RemovedController, &LastController);
  MMEntityData->Count--;
}

int32_t
GetEntityMMDataIndex(int32_t EntityIndex, const mm_entity_data* MMEntityData)
{
  int32_t MMDataIndex = -1;
  for(int i = 0; i < MMEntityData->Count; i++)
  {
    if(MMEntityData->EntityIndices[i] == EntityIndex)
    {
      MMDataIndex = i;
      break;
    }
  }
  return MMDataIndex;
}

void
ClearAnimationData(blend_stack* BlendStacks, int32_t* EntityIndices, int32_t Count,
                   entity* Entities, int32_t DebugEntityCount)
{
  for(int i = 0; i < Count; i++)
  {
    int EntityIndex = EntityIndices[i];
    assert(0 <= EntityIndex && EntityIndex < DebugEntityCount);
    BlendStacks[i].Clear();

    // Clear out anim plaer
    Anim::animation_player* AnimPlayer = Entities[EntityIndex].AnimPlayer;
    AnimPlayer->BlendFunc                  = NULL;
    for(int a = 0; a < AnimPlayer->AnimStateCount; a++)
    {
      assert(AnimPlayer->AnimationIDs[a].Value == 0);
      AnimPlayer->Animations[a] = NULL;
      AnimPlayer->States[a]     = {};
    }
    AnimPlayer->AnimStateCount = 0;
  }
}


bool
IsSplineAtIndexValid(int32_t Index, const spline_system* SplineSystem)
{
  if(0 <= Index && Index < SplineSystem->Splines.Count)
  {
    if(SplineSystem->Splines[Index].Waypoints.Count > 0)
    {
      return true;
    }
  }
  return false;
}

bool
ShouldSwap(int LeftIndex, int RightIndex, const mm_entity_data* MMData,
           const spline_system* SplineSystem)
{
  if(MMData->MMControllerRIDs[LeftIndex].Value > 0) // Left has controller
  {
    if(MMData->MMControllerRIDs[RightIndex].Value > 0) // Right has controller
    {
      if(!MMData->FollowSpline[LeftIndex]) // left is input controlled
      {
        return false;
      }
      else // Left is spline controlled
      {
        if(!MMData->FollowSpline[RightIndex]) // Right is input controlled
        {
          return true;
        }
        else // Both are spline controlled
        {
          if(IsSplineAtIndexValid(MMData->SplineStates[LeftIndex].SplineIndex,
                                  SplineSystem)) // Left has valid spline
          {
            return false;
          }
          else // Left has invalid spline
          {
            if(IsSplineAtIndexValid(MMData->SplineStates[RightIndex].SplineIndex,
                                    SplineSystem)) // Right has valid spline
            {
              return true;
            }
            else
            {
              return false;
            }
          }
        }
      }
    }
    return false;
  }
  return true;
}

void
SortMMEntityDataByUsage(int32_t* OutInputControlledCount, int32_t* OutTrajectoryControlledStart,
                        int32_t* OutTrajectoryControlledCount, mm_entity_data* MMEntityData,
                        const spline_system* Splines)
{
  for(int i = 0; i < MMEntityData->Count - 1; i++)
  {
    int SmallestIndex = i;
    for(int j = i + 1; j < MMEntityData->Count; j++)
    {
      if(ShouldSwap(SmallestIndex, j, MMEntityData, Splines))
      {
        SmallestIndex = j;
      }
    }
    if(SmallestIndex != i)
    {
      mm_aos_entity_data A = GetAOSMMDataAtIndex(i, MMEntityData);
      mm_aos_entity_data B = GetAOSMMDataAtIndex(SmallestIndex, MMEntityData);
      SwapMMEntityData(&A, &B);
    }
  }

  *OutInputControlledCount      = 0;
  *OutTrajectoryControlledCount = 0;
  for(int i = 0; i < MMEntityData->Count; i++)
  {
    if(MMEntityData->MMControllerRIDs[i].Value > 0)
    {
      if(MMEntityData->FollowSpline[i])
      {
        if(IsSplineAtIndexValid(MMEntityData->SplineStates[i].SplineIndex, Splines))
        {
          (*OutTrajectoryControlledCount)++;
        }
      }
      else
      {
        (*OutInputControlledCount)++;
      }
    }
  }
  *OutTrajectoryControlledStart = *OutInputControlledCount;
}

void
FetchMMControllerDataPointers(Resource::resource_manager* Resources,
                              mm_controller_data** OutMMControllers, rid* MMControllerRIDs,
                              int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    OutMMControllers[i] = Resources->GetMMController(MMControllerRIDs[i]);
    assert(OutMMControllers[i]);
  }
}

void
FetchSkeletonPointers(Anim::skeleton** OutSkeletons, const int32_t* EntityIndices,
                      const entity* Entities, int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    OutSkeletons[i] = Entities[EntityIndices[i]].AnimPlayer->Skeleton;
    assert(OutSkeletons[i]);
  }
}

void
FetchAnimationPointers(Resource::resource_manager* Resources, mm_controller_data** MMControllers,
                       int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    MMControllers[i]->Animations.HardClear();
    for(int j = 0; j < MMControllers[i]->Params.AnimRIDs.Count; j++)
    {
      Anim::animation* Anim = Resources->GetAnimation(MMControllers[i]->Params.AnimRIDs[j]);
      assert(Anim);
      MMControllers[i]->Animations.Push(Anim);
    }
  }
}

void
PlayAnimsIfBlendStacksAreEmpty(blend_stack* BSs, float* GlobalTimes,
                               const mm_controller_data* const* MMControllers, int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    if(BSs[i].Empty())
    {
      const int   IndexInSet     = 0;
      const float LocalStartTime = 0.0f;
      const float BlendInTime    = 0.0f;
      const bool  Mirror         = false;
      GlobalTimes[i]             = 0.0f;
      PlayAnimation(&BSs[i], MMControllers[i]->Animations[IndexInSet], LocalStartTime,
                    GlobalTimes[i], BlendInTime, Mirror);
    }
  }
}

void
DrawGoalFrameInfos(const mm_frame_info* GoalInfos, const blend_stack* BlendStacks,
                   const transform* LastMatchTransforms, int32_t Count,
                   const mm_info_debug_settings* MMInfoDebug, vec3 BoneColor, vec3 TrajectoryColor,
                   vec3 DirectionColor)
{
  for(int i = 0; i < Count; i++)
  {
    DrawFrameInfo(GoalInfos[i], TransformToMat4(LastMatchTransforms[i]), *MMInfoDebug, BoneColor,
                  BoneColor, TrajectoryColor, DirectionColor);
  }
}

void
DrawGoalFrameInfos(const mm_frame_info* GoalInfos, const int32_t* EntityIndices, int32_t Count,
                   const entity* Entities, const mm_info_debug_settings* MMInfoDebug,
                   vec3 BoneColor, vec3 TrajectoryColor, vec3 DirectionColor)
{
  for(int i = 0; i < Count; i++)
  {
    DrawFrameInfo(GoalInfos[i], TransformToMat4(Entities[EntityIndices[i]].Transform), *MMInfoDebug,
                  BoneColor, BoneColor, TrajectoryColor, DirectionColor);
  }
}

void
DrawControlTrajectories(const trajectory* Trajectories, const mm_input_controller* InputControllers,
                        const int32_t* EntityIndices, int32_t Count, const entity* Entities)
{
  for(int i = 0; i < Count; i++)
  {
    if(InputControllers[i].UseSmoothGoal)
    {
      DrawTrajectory(TransformToMat4(Entities[EntityIndices[i]].Transform), &Trajectories[i],
                     { 0, 1, 0 }, { 0, 0, 1 }, { 1, 1, 0 });
    }
  }
}

void
GenerateGoalsFromInput(mm_frame_info* OutGoals, mm_frame_info* OutMirroredGoals,
                       trajectory* Trajectories, Memory::stack_allocator* TempAlloc,
                       const blend_stack* BlendStacks, const float* GlobalTimes,
                       const Anim::skeleton* const*     Skeletons,
                       const mm_controller_data* const* MMControllers,
                       const mm_input_controller* InputControllers, const int32_t* EntityIndices,
                       int32_t Count, const entity* Entities, const game_input* Input,
                       const entity_goal_input* InputOverrides, int32_t InputOverrideCount,
                       vec3 CameraForward)
{
  // TODO(Lukas) Add joystick option here
  vec3 Dir         = {};
  vec3 ViewForward = Math::Normalized(vec3{ CameraForward.X, 0, CameraForward.Z });
  {
    vec3 YAxis     = { 0, 1, 0 };
    vec3 ViewRight = Math::Cross(ViewForward, YAxis);

    if(Input->ArrowUp.EndedDown)
    {
      Dir += ViewForward;
    }
    if(Input->ArrowDown.EndedDown)
    {
      Dir -= ViewForward;
    }
    if(Input->ArrowRight.EndedDown)
    {
      Dir += ViewRight;
    }
    if(Input->ArrowLeft.EndedDown)
    {
      Dir -= ViewRight;
    }
    if(Math::Length(Dir) > 0.5f)
    {
      Dir = Math::Normalized(Dir);
    }
  }

  for(int e = 0; e < Count; e++)
  {
    vec3 InputDir = Dir;
    for(int i = 0; i < InputOverrideCount; i++)
    {
      if(InputOverrides[i].EntityIndex == EntityIndices[e])
      {
        InputDir = InputOverrides[i].WorldDir;
      }
    }

    quat R = Entities[EntityIndices[e]].Transform.R;
    R.V *= -1;
    vec3 GoalVelocity =
      Math::MulMat3Vec3(Math::QuatToMat3(R), InputControllers[e].MaxSpeed * InputDir);
    vec3 GoalFacing =
      InputControllers[e].UseStrafing
        ? Math::MulMat3Vec3(Math::QuatToMat3(R), ViewForward)
        : (Math::Length(InputDir) != 0 ? Math::MulMat3Vec3(Math::QuatToMat3(R), InputDir)
                                       : vec3{ 0, 0, 1 });

    blend_in_info DominantBlend = BlendStacks[e].Peek();
    float         LocalAnimTime = GetLocalSampleTime(DominantBlend.Animation, GlobalTimes[e],
                                             DominantBlend.GlobalAnimStartTime);

    mat4 InvEntityMatrix = Math::InvMat4(TransformToMat4(Entities[EntityIndices[e]].Transform));
    trajectory_update_args TrajectoryArgs = {};
    {
        TrajectoryArgs.PositionBias    = InputControllers[e].PositionBias;
        TrajectoryArgs.DirectionBias   = InputControllers[e].DirectionBias;
        TrajectoryArgs.InvEntityMatrix = InvEntityMatrix;
    }
    GetMMGoal(&OutGoals[e], &OutMirroredGoals[e], &Trajectories[e], TempAlloc, Skeletons[e],
              DominantBlend.Animation, DominantBlend.Mirror, LocalAnimTime, GoalVelocity,
              GoalFacing, MMControllers[e]->Params.DynamicParams.TrajectoryTimeHorizon,
              MMControllers[e]->Params.FixedParams,
              InputControllers[e].UseSmoothGoal ? &TrajectoryArgs : NULL);
  }
}

void
AssertSplineIndicesAndClampWaypointIndices(spline_follow_state* SplineStates, int32_t Count,
                                           const movement_spline* Splines, int32_t DebugSplineCount)
{
  for(int i = 0; i < Count; i++)
  {
    int SplineIndex = SplineStates[i].SplineIndex;

    assert(0 <= SplineIndex && SplineIndex < DebugSplineCount);
    assert(Splines[SplineIndex].Waypoints.Count > 0);

    SplineStates[i].NextWaypointIndex = ClampInt32InIn(0, SplineStates[i].NextWaypointIndex,
                                                       Splines[SplineIndex].Waypoints.Count - 1);
  }
}

void
GenerateGoalsFromSplines(Memory::stack_allocator* TempAlloc, mm_frame_info* OutGoals,
                         mm_frame_info* OutMirroredGoals, trajectory* Trajectories,
                         spline_follow_state*             SplineStates,
                         const mm_input_controller*       InputControllers,
                         const mm_controller_data* const* MMControllers,
                         const blend_stack* BlendStacks, const float* AnimPlayerTimes,
                         const Anim::skeleton* const* Skeletons, const int32_t* EntityIndices,
                         int32_t Count, const movement_spline* Splines, int32_t DebugSplineCount,
                         const entity* Entities)
{
  for(int e = 0; e < Count; e++)
  {

    const float WaypointRadius  = 0.8f;
    const float Inputdt         = 1 / 60.0f;
    transform   EntityTransform = Entities[EntityIndices[e]].Transform;

    quat InvR = EntityTransform.R;
    InvR.V *= -1;

    spline_follow_state& EntitySplineState = SplineStates[e];
    assert(0 <= EntitySplineState.SplineIndex && EntitySplineState.SplineIndex < DebugSplineCount);

    const movement_spline* Spline = &Splines[EntitySplineState.SplineIndex];

    vec3 WorldDiff =
      Spline->Waypoints[EntitySplineState.NextWaypointIndex].Position - EntityTransform.T;
    vec3 LocalEntityToWaypoint = Math::MulMat3Vec3(Math::QuatToMat3(InvR), WorldDiff);
    vec3 LocalDir              = Math::Normalized(LocalEntityToWaypoint);

    vec3 GoalVelocity = LocalDir*InputControllers[e].MaxSpeed;
    vec3 GoalFacing   = LocalDir;

    blend_in_info DominantBlend = BlendStacks[e].Peek();
    float         LocalAnimTime = GetLocalSampleTime(DominantBlend.Animation, AnimPlayerTimes[e],
                                             DominantBlend.GlobalAnimStartTime);

    trajectory_update_args TrajectoryArgs =  {};
    {
        TrajectoryArgs.PositionBias  = InputControllers[e].PositionBias;
        TrajectoryArgs.DirectionBias = InputControllers[e].DirectionBias;
        TrajectoryArgs.InvEntityMatrix = Math::InvMat4(TransformToMat4(EntityTransform));
    }

    GetMMGoal(&OutGoals[e], &OutMirroredGoals[e], &Trajectories[e], TempAlloc, Skeletons[e],
              DominantBlend.Animation, DominantBlend.Mirror, LocalAnimTime, GoalVelocity,
              GoalFacing, MMControllers[e]->Params.DynamicParams.TrajectoryTimeHorizon,
              MMControllers[e]->Params.FixedParams,
              InputControllers[e].UseSmoothGoal ? &TrajectoryArgs : NULL);

    if(Math::Length(LocalEntityToWaypoint) < WaypointRadius)
    {
      /*if(EntitySplineState.Loop)
      {*/
      EntitySplineState.NextWaypointIndex =
        (EntitySplineState.NextWaypointIndex + 1) % Spline->Waypoints.Count;
      /*}
      else 
      {
        EntitySplineState.MovingInPositive = false;
        int Forward           = (EntitySplineState.MovingInPositive) ? 1 : -1;
        int NextWaypointIndex = EntitySplineState.NextWaypointIndex + Forward;
        if(NextWaypointIndex < 0 ||
           NextWaypointIndex > Spline->Waypoints.Count - 1) // Travel backwards
        {
          NextWaypointIndex -= 2 * Forward;
        }
        EntitySplineState.NextWaypointIndex =
          ClampInt32InIn(0, NextWaypointIndex, Spline->Waypoints.Count - 1);
      }*/
    }
  }
}

// Only used to visualize the mirrored match, not adequate way to flip for searching mirrors
mm_frame_info
VisualFlipGoalX(const mm_frame_info& Goal)
{
  mm_frame_info FlippedGoal = Goal;
  for(int i = 0; i < MM_COMPARISON_BONE_COUNT; i++)
  {
    FlippedGoal.BonePs[i].X *= -1;
    FlippedGoal.BoneVs[i].X *= -1;
  }
  for(int i = 0; i < MM_POINT_COUNT; i++)
  {
    FlippedGoal.TrajectoryPs[i].X *= -1;
    FlippedGoal.TrajectoryAngles[i] *= -1;
  }
  return FlippedGoal;
}

// Figure out if matched potition is sufficiently far away from all animations which are already
// playing
bool
IsNewAnimSufficientlyFarAway(const Anim::animation* NewAnim, float NewAnimLocalStartTime,
                             const blend_stack& BlendStack, float AnimPlayerTime,
                             float TimeOffetThreshold)
{
  for(int a = 0; a < BlendStack.Count; a++)
  {
    blend_in_info ActiveAnimBlend = BlendStack[a];

    float ActiveAnimLocalAnimTime = GetLocalSampleTime(ActiveAnimBlend.Animation, AnimPlayerTime,
                                                       ActiveAnimBlend.GlobalAnimStartTime);
    if(NewAnim == ActiveAnimBlend.Animation &&
       AbsFloat(ActiveAnimLocalAnimTime - NewAnimLocalStartTime) < TimeOffetThreshold)
    {
      return false;
    }
  }
  return true;
}

void
MotionMatchGoals(blend_stack* OutBlendStacks, mm_frame_info* LastMatchedGoals,
                 transform* OutLastMatchedTransforms, const mm_frame_info* AnimGoals,
                 const mm_frame_info*             MirroredAnimGoals,
                 const mm_controller_data* const* MMControllers, const float* GlobalTimes,
                 const int32_t* EntityIndices, int32_t Count, entity* Entities)
{
  for(int i = 0; i < Count; i++)
  {
    int32_t NewAnimIndex;
    float   NewAnimLocalStartTime;
    bool    NewMatchIsMirrored = false;

    mm_frame_info BestMatch = {};
    if(!MMControllers[i]->Params.DynamicParams.MatchMirroredAnimations)
    {
      MotionMatch(&NewAnimIndex, &NewAnimLocalStartTime, &BestMatch, MMControllers[i],
                  AnimGoals[i]);
    }
    else
    {
      MotionMatchWithMirrors(&NewAnimIndex, &NewAnimLocalStartTime, &BestMatch, &NewMatchIsMirrored,
                             MMControllers[i], AnimGoals[i], MirroredAnimGoals[i]);
    }

    const Anim::animation* MatchedAnim = MMControllers[i]->Animations[NewAnimIndex];

    if(IsNewAnimSufficientlyFarAway(MatchedAnim, NewAnimLocalStartTime, OutBlendStacks[i],
                                    GlobalTimes[i],
                                    MMControllers[i]->Params.DynamicParams.MinTimeOffsetThreshold))
    {
      LastMatchedGoals[i] = (NewMatchIsMirrored) ? VisualFlipGoalX(BestMatch) : BestMatch;

      PlayAnimation(&OutBlendStacks[i], MMControllers[i]->Animations[NewAnimIndex],
                    NewAnimLocalStartTime, GlobalTimes[i],
                    MMControllers[i]->Params.DynamicParams.BlendInTime, NewMatchIsMirrored);

      // Store the transform of where the last match occured
      OutLastMatchedTransforms[i] = Entities[EntityIndices[i]].Transform;
    }
  }
}

transform
GetLocalAnimRootMotionDelta(Anim::animation* RootMotionAnim, const Anim::skeleton* Skeleton,
                            bool MirrorRootMotionInX, float LocalSampleTime, float dt)
{
  assert(RootMotionAnim);

  const int HipBoneIndex = 0;

  transform RootDelta = IdentityTransform();

  float NextSampleTime = LocalSampleTime + dt;

  transform CurrentHipTransform =
    Anim::LinearAnimationBoneSample(RootMotionAnim, HipBoneIndex, LocalSampleTime);
  transform NextHipTransform =
    Anim::LinearAnimationBoneSample(RootMotionAnim, HipBoneIndex, NextSampleTime);

  {
    // mat4 Mat4CurrentHip = TransformToMat4(CurrentHipTransform);
    mat4 Mat4CurrentHip =
      Math::MulMat4(Skeleton->Bones[HipBoneIndex].BindPose, TransformToMat4(CurrentHipTransform));
    mat4 Mat4CurrentRoot;
    mat4 Mat4InvCurrentRoot;
    Anim::GetRootAndInvRootMatrices(&Mat4CurrentRoot, &Mat4InvCurrentRoot, Mat4CurrentHip);
    {
      // mat4 Mat4NextHip = TransformToMat4(NextHipTransform);
      mat4 Mat4NextHip =
        Math::MulMat4(Skeleton->Bones[HipBoneIndex].BindPose, TransformToMat4(NextHipTransform));
      mat4 Mat4NextRoot;
      Anim::GetRootAndInvRootMatrices(&Mat4NextRoot, NULL, Mat4NextHip);
      {
        mat4 Mat4LocalRootDelta = Math::MulMat4(Mat4InvCurrentRoot, Mat4NextRoot);
        quat dR                 = Math::QuatFromTo(Mat4CurrentRoot.Z, Mat4NextRoot.Z);
        if(MirrorRootMotionInX)
        {
          Mat4LocalRootDelta = Math::MulMat4(Math::Mat4Scale(-1, 1, 1), Mat4LocalRootDelta);
          dR                 = Math::QuatFromTo(Mat4NextRoot.Z, Mat4CurrentRoot.Z);
        }

        vec3 dT     = Mat4LocalRootDelta.T;
        RootDelta.T = dT;
        RootDelta.R = dR;
        return RootDelta;
      }
    }
  }
}

void
ComputeLocalRootMotion(transform* OutLocalDeltaRootMotions, const Anim::skeleton* const* Skeletons,
                       const blend_stack* BlendStacks, const float* GlobalTimes, int32_t Count,
                       float dt)
{
  for(int i = 0; i < Count; i++)
  {
    blend_in_info    AnimBlend      = BlendStacks[i].Peek();
    Anim::animation* RootMotionAnim = AnimBlend.Animation;
    float            LocalSampleTime =
      Anim::GetLocalSampleTime(RootMotionAnim, GlobalTimes[i], AnimBlend.GlobalAnimStartTime);
    OutLocalDeltaRootMotions[i] =
      GetLocalAnimRootMotionDelta(RootMotionAnim, Skeletons[i], AnimBlend.Mirror, LocalSampleTime,
                                  dt);
  }
}

void
ApplyRootMotion(entity* InOutEntities, trajectory* Trajectories,
                const transform* LocalDeltaRootMotions, int32_t* EntityIndices, int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    transform* TargetTransform = &InOutEntities[EntityIndices[i]].Transform;

    vec3 dT =
      Math::MulMat4Vec4(Math::Mat4Rotate(TargetTransform->R), { LocalDeltaRootMotions[i].T, 0 })
        .XYZ;

    TargetTransform->R = TargetTransform->R * LocalDeltaRootMotions[i].R;
    TargetTransform->T += dT;

    vec2 DeltaToNewRoot = vec2{ TargetTransform->T.X, TargetTransform->T.Z } -
                          Trajectories[i].Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT].T;
    for(int p = 0; p < HALF_TRAJECTORY_TRANSFORM_COUNT; p++)
    {
      Trajectories[i].Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT + p].T += DeltaToNewRoot;
    }
    Trajectories[i].Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT].R = TargetTransform->R;
  }
}

void
AdvanceAnimPlayerTimes(float* InOutAnimPlayerTimes, int32_t Count, float dt)
{
  for(int i = 0; i < Count; i++)
  {
    InOutAnimPlayerTimes[i] += dt;
  }
}

void
RemoveBlendedOutAnimsFromBlendStacks(blend_stack* InOutBlendStacks, const float* GlobalPlayTimes,
                                     int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    for(int j = InOutBlendStacks[i].Count - 1; j >= 1; j--)
    {
      blend_in_info TempBlendInfo = InOutBlendStacks[i][j];

      if((GlobalPlayTimes[i] - TempBlendInfo.GlobalBlendStartTime) >= TempBlendInfo.BlendDuration)
      {
        for(int k = 0; k < j; k++)
        {
          InOutBlendStacks[i].PopBack();
        }
        break;
      }
    }
  }
}

void
CopyMMAnimDataToAnimationPlayers(entity* OutEntities, const blend_stack* BlendStacks,
                                 const float* GlobalPlayTimes, const int32_t* EntityIndices,
                                 int32_t Count)
{
  for(int e = 0; e < Count; e++)
  {
    Anim::animation_player* C = OutEntities[EntityIndices[e]].AnimPlayer;
    C->BlendFunc              = BlendStackBlendFunc;
    C->GlobalTimeSec          = GlobalPlayTimes[e];
    for(int i = 0; i < ANIM_PLAYER_MAX_ANIM_COUNT; i++)
    {
      C->States[i] = {};
    }
    C->AnimStateCount = BlendStacks[e].Count;

    for(int a = 0; a < C->AnimStateCount; a++)
    {
      const blend_in_info BlendInfo = BlendStacks[e][a];
      C->Animations[a]              = BlendInfo.Animation;
      C->States[a].StartTimeSec     = BlendInfo.GlobalAnimStartTime;
      C->States[a].Mirror           = BlendInfo.Mirror;
      C->States[a].Loop             = BlendInfo.Loop;
      C->States[a].PlaybackRateSec  = 1.0f;
    }
  }
}

void
DrawFrameInfo(mm_frame_info AnimGoal, mat4 CoordinateFrame, mm_info_debug_settings DebugSettings,
              vec3 BoneColor, vec3 VelocityColor, vec3 TrajectoryColor, vec3 DirectionColor)
{
	const vec3 VerticalOffset{0, 0.01f, 0};
  for(int i = 0; i < MM_COMPARISON_BONE_COUNT; i++)
  {
    vec4 HomogLocalBoneP = { AnimGoal.BonePs[i], 1 };
    vec3 WorldBoneP      = Math::MulMat4Vec4(CoordinateFrame, HomogLocalBoneP).XYZ;
    vec4 HomogLocalBoneV = { AnimGoal.BoneVs[i], 0 };
    vec3 WorldBoneV      = Math::MulMat4Vec4(CoordinateFrame, HomogLocalBoneV).XYZ;
    vec3 WorldVEnd       = WorldBoneP + WorldBoneV;

    if(DebugSettings.ShowBonePositions)
    {
      Debug::PushWireframeSphere(WorldBoneP, 0.02f, { BoneColor, 1 });
    }
    if(DebugSettings.ShowBoneVelocities)
    {
      Debug::PushLine(WorldBoneP+VerticalOffset, WorldVEnd+VerticalOffset, { VelocityColor, 1 }, DebugSettings.Overlay);
      Debug::PushWireframeSphere(WorldVEnd, 0.01f, { VelocityColor, 1 });
    }
  }
  vec3 PrevWorldTrajectoryPointP = CoordinateFrame.T;
  if(DebugSettings.ShowTrajectory)
  {
    for(int i = 0; i < MM_POINT_COUNT; i++)
    {
      vec4 HomogTrajectoryPointP = { AnimGoal.TrajectoryPs[i], 1 };
      vec3 WorldTrajectoryPointP = Math::MulMat4Vec4(CoordinateFrame, HomogTrajectoryPointP).XYZ;
      {
        Debug::PushLine(PrevWorldTrajectoryPointP+VerticalOffset, WorldTrajectoryPointP+VerticalOffset, { TrajectoryColor, 1 },
                        DebugSettings.Overlay);
        Debug::PushWireframeSphere(WorldTrajectoryPointP, 0.02f, { TrajectoryColor, 1 });
        PrevWorldTrajectoryPointP = WorldTrajectoryPointP;
      }
      const float GoalDirectionLength = 0.2f;
      if(DebugSettings.ShowTrajectoryAngles)
      {
        vec4 LocalSpaceFacingDirection = { sinf(AnimGoal.TrajectoryAngles[i]), 0,
                                           cosf(AnimGoal.TrajectoryAngles[i]), 0 };
        vec3 WorldSpaceFacingDirection =
          Math::MulMat4Vec4(CoordinateFrame, LocalSpaceFacingDirection).XYZ;
        Debug::PushLine(WorldTrajectoryPointP + VerticalOffset,
                        WorldTrajectoryPointP + GoalDirectionLength * WorldSpaceFacingDirection +
                          VerticalOffset,
                        { DirectionColor, 1 }, DebugSettings.Overlay);
      }
    }
  }
}

void
DrawTrajectory(mat4 CoordinateFrame, const trajectory* Trajectory, vec3 PastColor,
               vec3 PresentColor, vec3 FutureColor)
{
  for(int i = 0; i < TRAJECTORY_TRANSFORM_COUNT; i++)
  {
    vec3 CurrentWorldPos = vec3{ Trajectory->Transforms[i].T.X, 0, Trajectory->Transforms[i].T.Y };
    vec3 Forward = { 0, 0, 1 };
    vec3 FacingDir = Math::MulMat3Vec3(Math::QuatToMat3(Trajectory->Transforms[i].R), Forward);

    vec3 PointColor = (i < HALF_TRAJECTORY_TRANSFORM_COUNT)
                        ? PastColor
                        : ((i == HALF_TRAJECTORY_TRANSFORM_COUNT) ? PresentColor : FutureColor);
    //Debug::PushWireframeSphere(CurrentWorldPos, 0.005f, { PointColor, 0.5 });
    Debug::PushLine(CurrentWorldPos, CurrentWorldPos + FacingDir * 0.1f, { 0, 1, 1, 1 });
  }
}
