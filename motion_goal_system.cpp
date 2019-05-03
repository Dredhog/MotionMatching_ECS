#include "motion_goal_system.h"
#include "debug_drawing.h"

void
FetchAnimControllerPointers(Anim::animation_controller** OutAnimControllers,
                            const int32_t* EntityIndices, const entity* Entities, int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    OutAnimControllers[i] = Entities[EntityIndices[i]].AnimController;
    assert(OutAnimControllers[i]);
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
PlayAnimsIfBlendStacksAreEmpty(blend_stack* BSs, Anim::animation_controller** ACs,
                               const mm_controller_data* const* MMControllers, int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    if(BSs[i].Empty())
    {
      ACs[i]->BlendFunc = ThirdPersonAnimationBlendFunction;

      const int   IndexInSet     = 0;
      const float LocalStartTime = 0.0f;
      const float BlendInTime    = 0.0f;
      const bool  Mirror         = false;
      PlayAnimation(ACs[i], &BSs[i], MMControllers[i]->Params.AnimRIDs[IndexInSet],
                    MMControllers[i]->Animations[IndexInSet], LocalStartTime, BlendInTime, Mirror);
      ACs[i]->Animations[BSs[i].Peek().AnimStateIndex] = MMControllers[i]->Animations[IndexInSet];
    }
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
GenerateGoalsFromInput(mm_frame_info* OutGoals, mm_frame_info* OutMirroredGoals,
                       Memory::stack_allocator* TempAlloc, const blend_stack* BlendStacks,
                       const Anim::animation_controller* const* AnimControllers,
                       const mm_controller_data* const*         MMControllers,
                       const mm_input_controller* InputControllers, const int32_t* EntityIndices,
                       int32_t Count, const entity* Entities, const game_input* Input,
                       vec3 CameraForward)
{
  // TODO(Lukas) Add joystick option here
  vec3 Dir = {};
  {
    vec3 YAxis       = { 0, 1, 0 };
    vec3 ViewForward = Math::Normalized(vec3{ CameraForward.X, 0, CameraForward.Z });
    vec3 ViewRight   = Math::Cross(ViewForward, YAxis);

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

  for(int i = 0; i < Count; i++)
  {
    quat R = Entities[EntityIndices[i]].Transform.R;
    R.V *= -1;
    vec3 GoalVelocity =
      Math::MulMat4Vec4(Math::Mat4Rotate(R), { InputControllers[i].MaxSpeed * Dir, 0 }).XYZ;

    blend_in_info DominantBlend = BlendStacks[i].Peek();

    GetMMGoal(&OutGoals[i], &OutMirroredGoals[i], TempAlloc, DominantBlend.AnimStateIndex,
              DominantBlend.Mirror, AnimControllers[i], GoalVelocity,
              MMControllers[i]->Params.FixedParams);
  }
}

void
GenerateGoalsFromSplines(mm_frame_info* OutGoals, const spline_follow_state* SplineStates,
                         const Anim::animation_controller* const* ACs,
                         const blend_stack* BlendStacks, int32_t Count,
                         const movement_spline* Splines)
{
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

void
MotionMatchGoals(blend_stack* OutBlendStacks, Anim::animation_controller* const* AnimControllers,
                 mm_frame_info* LastMatchedGoals, const mm_frame_info* AnimGoals,
                 const mm_frame_info*             MirroredAnimGoals,
                 const mm_controller_data* const* MMControllers, int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    int32_t NewAnimIndex;
    float   NewAnimStartTime;
    bool    NewMatchIsMirrored = false;

    mm_frame_info BestMatch = {};
    if(!MMControllers[i]->Params.DynamicParams.MatchMirroredAnimations)
    {
      MotionMatch(&NewAnimIndex, &NewAnimStartTime, &BestMatch, MMControllers[i], AnimGoals[i]);
    }
    else
    {
      MotionMatchWithMirrors(&NewAnimIndex, &NewAnimStartTime, &BestMatch, &NewMatchIsMirrored,
                             MMControllers[i], AnimGoals[i], MirroredAnimGoals[i]);
    }

    const Anim::animation* MatchedAnim = MMControllers[i]->Animations[NewAnimIndex];
    // animation pointer should always be present in the AnimController
    int   ActiveStateIndex    = OutBlendStacks[i].Peek().AnimStateIndex;
    float ActiveAnimLocalTime = Anim::GetLocalSampleTime(AnimControllers[i], ActiveStateIndex,
                                                         AnimControllers[i]->GlobalTimeSec);

    // Figure out if matched frame is sufficiently far away from the current to start a new
    // animation
    if(AnimControllers[i]->AnimationIDs[ActiveStateIndex].Value !=
         MMControllers[i]->Params.AnimRIDs[NewAnimIndex].Value ||
       (AbsFloat(ActiveAnimLocalTime - NewAnimStartTime) >=
          MMControllers[i]->Params.DynamicParams.MinTimeOffsetThreshold &&
        NewMatchIsMirrored == OutBlendStacks[i].Peek().Mirror) ||
       NewMatchIsMirrored != OutBlendStacks[i].Peek().Mirror)
    {
      LastMatchedGoals[i] = (NewMatchIsMirrored) ? VisualFlipGoalX(BestMatch) : BestMatch;
      PlayAnimation(AnimControllers[i], &OutBlendStacks[i],
                    MMControllers[i]->Params.AnimRIDs[NewAnimIndex],
                    MMControllers[i]->Animations[NewAnimIndex], NewAnimStartTime,
                    MMControllers[i]->Params.DynamicParams.BelndInTime, NewMatchIsMirrored);
    }
  }
}

void
ComputeLocalRootMotion(transform*                               OutLocalDeltaRootMotions,
                       const Anim::animation_controller* const* AnimControllers,
                       const blend_stack* BlendStacks, int32_t Count, float dt)
{
  for(int i = 0; i < Count; i++)
  {
    blend_in_info    AnimBlend      = BlendStacks[i].Peek();
    Anim::animation* RootMotionAnim = AnimControllers[i]->Animations[AnimBlend.AnimStateIndex];
    float            LocalSampleTime =
      Anim::GetLocalSampleTime(RootMotionAnim,
                               &AnimControllers[i]->States[AnimBlend.AnimStateIndex],
                               AnimControllers[i]->GlobalTimeSec);
    OutLocalDeltaRootMotions[i] = GetAnimRootMotionDelta(RootMotionAnim, AnimControllers[i],
                                                         AnimBlend.Mirror, LocalSampleTime, dt);
  }
}

void
ApplyRootMotion(entity* InOutEntities, const transform* LocalDeltaRootMotions,
                int32_t* EntityIndices, int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    transform* TargetTransform = &InOutEntities[EntityIndices[i]].Transform;

    vec3 dT =
      Math::MulMat4Vec4(Math::Mat4Rotate(TargetTransform->R), { LocalDeltaRootMotions[i].T, 0 })
        .XYZ;

    TargetTransform->R = TargetTransform->R * LocalDeltaRootMotions[i].R;
    TargetTransform->T += dT;
  }
}


void
DrawFrameInfo(mm_frame_info AnimGoal, mat4 CoordinateFrame, mm_info_debug_settings DebugSettings,
              vec3 BoneColor, vec3 VelocityColor, vec3 TrajectoryColor, vec3 DirectionColor)
{
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
      Debug::PushLine(WorldBoneP, WorldVEnd, { VelocityColor, 1 });
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
        Debug::PushLine(PrevWorldTrajectoryPointP, WorldTrajectoryPointP, { TrajectoryColor, 1 });
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
        Debug::PushLine(WorldTrajectoryPointP,
                        WorldTrajectoryPointP + GoalDirectionLength * WorldSpaceFacingDirection,
                        { DirectionColor, 1 });
      }
    }
  }
}

transform
GetAnimRootMotionDelta(Anim::animation* RootMotionAnim, const Anim::animation_controller* C,
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
    mat4 Mat4CurrentHip = Math::MulMat4(C->Skeleton->Bones[HipBoneIndex].BindPose,
                                        TransformToMat4(CurrentHipTransform));
    mat4 Mat4CurrentRoot;
    mat4 Mat4InvCurrentRoot;
    Anim::GetRootAndInvRootMatrices(&Mat4CurrentRoot, &Mat4InvCurrentRoot, Mat4CurrentHip);
    {
      // mat4 Mat4NextHip = TransformToMat4(NextHipTransform);
      mat4 Mat4NextHip =
        Math::MulMat4(C->Skeleton->Bones[HipBoneIndex].BindPose, TransformToMat4(NextHipTransform));
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
        // TODO(Lukas) Remember to apply the entity's rotation to the delta
        // Mat4Entity.T    = {};
        // mat4 Mat4DeltaRoot = Math::MulMat4(Mat4Entity, Mat4LocalRootDelta);

        vec3 dT     = Mat4LocalRootDelta.T;
        RootDelta.T = dT;
        RootDelta.R = dR;
        return RootDelta;
      }
    }
  }
}
void
SortMMEntityDataByUsage(int32_t* OutInputControlledCount, int32_t* OutTrajectoryControlledStart,
                        int32_t* OutTrajectoryControlledCount, mm_entity_data* MMEntityData)
{
  for(int i = 0; i < MMEntityData->Count - 1; i++)
  {
    int SmallestIndex = i;
    for(int j = i + 1; j < MMEntityData->Count; j++)
    {
      if(MMEntityData->MMControllerRIDs[j].Value > 0)
      {
        if(MMEntityData->MMControllerRIDs[SmallestIndex].Value <= 0 ||
           (MMEntityData->FollowSpline[SmallestIndex] && !MMEntityData->FollowSpline[j]))
        {
          SmallestIndex = j;
        }
      }
    }
    if(SmallestIndex != i)
    {
      mm_aos_entity_data A = GetEntityAOSMMData(i, MMEntityData);
      mm_aos_entity_data B = GetEntityAOSMMData(SmallestIndex, MMEntityData);
      SwapMMEntityData(&A, &B);
      continue;
    }
  }
  *OutInputControlledCount      = 0;
  *OutTrajectoryControlledCount = 0;
  *OutTrajectoryControlledStart = 0;
  for(int i = 0; i < MMEntityData->Count; i++)
  {
    if(MMEntityData->MMControllerRIDs[i].Value > 0)
    {
      if(MMEntityData->FollowSpline[i])
      {
        (*OutTrajectoryControlledCount)++;
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
