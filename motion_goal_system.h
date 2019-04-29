#pragma once

#include "trajectory.h"
#include "motion_matching.h"
#include "rid.h"

#include "resource_manager.h"
#include "blend_stack.h"
#include "common.h"
#include "misc.h"
#include <stdint.h>

#define MM_CONTROLLER_MAX_COUNT 20

enum spline_loop_type
{
  SPLINE_LoopToStart,
  SPLINE_ReverseWhenEnded,
};

struct entity_trajectory_state
{
  int32_t SplineIndex;

  int32_t  NextWaypointIndex;
  uint32_t LoopType;
  bool     MovingInPositive;
};

struct mm_input_control_params
{
  float MaxSpeed;
  float Acceleration;
  bool  UseStrafing;
};

struct mm_entity_data
{
  // Serializable data
  int32_t                 Count;
  rid                     MMControllerRIDs[MM_CONTROLLER_MAX_COUNT];   // Persistent/Const
  blend_stack             BlendStacks[MM_CONTROLLER_MAX_COUNT];        // Persistent
  int32_t                 EntityIndices[MM_CONTROLLER_MAX_COUNT];      // Persistent
  bool                    FollowTrajectory[MM_CONTROLLER_MAX_COUNT];   // Persistent
  entity_trajectory_state TrajectoryStates[MM_CONTROLLER_MAX_COUNT];   // persistent
  mm_input_control_params InputControlParams[MM_CONTROLLER_MAX_COUNT]; // persistent

  // Intermediate data
  mm_controller_data*         MMControllers[MM_CONTROLLER_MAX_COUNT];    // One Frame
  Anim::animation_controller* AnimControllers[MM_CONTROLLER_MAX_COUNT];  // One Frame
  mm_frame_info               LastMatchedGoals[MM_CONTROLLER_MAX_COUNT]; // One Frame
  mm_frame_info               MirroredAnimGoals[MM_CONTROLLER_MAX_COUNT]; // One Frame
  mm_frame_info               AnimGoals[MM_CONTROLLER_MAX_COUNT];        // One Frame

  // Output data
  transform OutDeltaRootMotions[MM_CONTROLLER_MAX_COUNT]; // One Frame
};

// Used in the editor to access the soa fields
struct mm_aos_entity_data
{
  rid* const                     MMControllerRID;
  blend_stack* const             BlendStack;
  int32_t* const                 EntityIndex;
  bool* const                    FollowTrajectory;
  entity_trajectory_state* const TrajectoryState;
  mm_input_control_params* const InputControlParams;

  // Intermediate data
  mm_controller_data** const         MMController;
  Anim::animation_controller** const AnimController;
  mm_frame_info* const               LastMatchedGoal;
  mm_frame_info* const               MirroredAnimGoal;
  mm_frame_info* const               AnimGoal;
};

inline void
SetDefaultMMControllerFileds(mm_aos_entity_data* MMEntityData)
{
  *MMEntityData->MMControllerRID    = {};
  *MMEntityData->BlendStack         = {};
  *MMEntityData->EntityIndex        = -1;
  *MMEntityData->FollowTrajectory   = false;
  *MMEntityData->TrajectoryState    = { .SplineIndex       = -1,
                                     .NextWaypointIndex = 0,
                                     .LoopType          = 0,
                                     .MovingInPositive  = true };
  *MMEntityData->InputControlParams = { .MaxSpeed     = 1.0f,
                                        .Acceleration = 2.0f,
                                        .UseStrafing  = false };

  *MMEntityData->MMController     = NULL;
  *MMEntityData->AnimController   = NULL;
  *MMEntityData->LastMatchedGoal  = {};
  *MMEntityData->MirroredAnimGoal = {};
  *MMEntityData->AnimGoal         = {};
}

inline void
CopyAOSMMEntityData(mm_aos_entity_data* Dest, const mm_aos_entity_data* Src)
{
  *Dest->MMControllerRID    = *Src->MMControllerRID;
  *Dest->BlendStack         = *Src->BlendStack;
  *Dest->EntityIndex        = *Src->EntityIndex;
  *Dest->TrajectoryState    = *Src->TrajectoryState;
  *Dest->FollowTrajectory   = *Src->FollowTrajectory;
  *Dest->InputControlParams = *Src->InputControlParams;

  *Dest->MMController     = *Src->MMController;
  *Dest->AnimController   = *Src->AnimController;
  *Dest->LastMatchedGoal  = *Src->LastMatchedGoal;
  *Dest->MirroredAnimGoal = *Src->MirroredAnimGoal;
  *Dest->AnimGoal         = *Src->AnimGoal;
}

inline void
SwapMMEntityData(mm_aos_entity_data* A, mm_aos_entity_data* B)
{
  rid                     TempMMControllerRID    = *A->MMControllerRID;
  blend_stack             TempBlendStack         = *A->BlendStack;
  int32_t                 TempEntityIndex        = *A->EntityIndex;
  entity_trajectory_state TempTrajectoryState    = *A->TrajectoryState;
  bool                    TempFollowTrajectory   = *A->FollowTrajectory;
  mm_input_control_params TempInputControlParams = *A->InputControlParams;

  mm_controller_data*         TempMMController     = *A->MMController;
  Anim::animation_controller* TempAnimController   = *A->AnimController;
  mm_frame_info               TempLastMatchedGoal  = *A->LastMatchedGoal;
  mm_frame_info               TempMirroredAnimGoal = *A->MirroredAnimGoal;
  mm_frame_info               TempAnimGoal         = *A->AnimGoal;

  *A->MMControllerRID    = *B->MMControllerRID;
  *A->BlendStack         = *B->BlendStack;
  *A->EntityIndex        = *B->EntityIndex;
  *A->TrajectoryState    = *B->TrajectoryState;
  *A->FollowTrajectory   = *B->FollowTrajectory;
  *A->InputControlParams = *B->InputControlParams;
  *A->MMController       = *B->MMController;
  *A->AnimController     = *B->AnimController;
  *A->LastMatchedGoal    = *B->LastMatchedGoal;
  *A->MirroredAnimGoal   = *B->MirroredAnimGoal;
  *A->AnimGoal           = *B->AnimGoal;

  *B->MMControllerRID    = TempMMControllerRID;
  *B->BlendStack         = TempBlendStack;
  *B->EntityIndex        = TempEntityIndex;
  *B->TrajectoryState    = TempTrajectoryState;
  *B->FollowTrajectory   = TempFollowTrajectory;
  *B->InputControlParams = TempInputControlParams;
  *B->MMController       = TempMMController;
  *B->AnimController     = TempAnimController;
  *B->LastMatchedGoal    = TempLastMatchedGoal;
  *B->MirroredAnimGoal   = TempMirroredAnimGoal;
  *B->AnimGoal           = TempAnimGoal;
}

inline mm_aos_entity_data
GetEntityAOSMMData(int32_t MMDataIndex, mm_entity_data* MMEntityData)
{
  assert(0 <= MMDataIndex && MMDataIndex < MMEntityData->Count);
  mm_aos_entity_data Result = {
    .MMControllerRID    = &MMEntityData->MMControllerRIDs[MMDataIndex],
    .BlendStack         = &MMEntityData->BlendStacks[MMDataIndex],
    .EntityIndex        = &MMEntityData->EntityIndices[MMDataIndex],
    .TrajectoryState    = &MMEntityData->TrajectoryStates[MMDataIndex],
    .FollowTrajectory   = &MMEntityData->FollowTrajectory[MMDataIndex],
    .InputControlParams = &MMEntityData->InputControlParams[MMDataIndex],
    .MMController       = &MMEntityData->MMControllers[MMDataIndex],
    .LastMatchedGoal    = &MMEntityData->LastMatchedGoals[MMDataIndex],
    .MirroredAnimGoal   = &MMEntityData->MirroredAnimGoals[MMDataIndex],
    .AnimGoal           = &MMEntityData->AnimGoals[MMDataIndex],
    .AnimController     = &MMEntityData->AnimControllers[MMDataIndex],
  };
  return Result;
}

inline void
CopyMMEntityData(int32_t DestIndex, int32_t SourceIndex, mm_entity_data* MMEntityData)
{
  mm_aos_entity_data Dest   = GetEntityAOSMMData(DestIndex, MMEntityData);
  mm_aos_entity_data Source = GetEntityAOSMMData(SourceIndex, MMEntityData);
  CopyAOSMMEntityData(&Dest, &Source);
}

inline void
RemoveMMControllerDataAtIndex(int32_t MMControllerIndex, Resource::resource_manager* Resources,
                              mm_entity_data* MMEntityData)
{
  assert(0 <= MMControllerIndex && MMControllerIndex < MMEntityData->Count);

  mm_aos_entity_data RemovedController = GetEntityAOSMMData(MMControllerIndex, MMEntityData);
  if(RemovedController.MMControllerRID->Value > 0)
  {
    Resources->MMControllers.RemoveReference(*RemovedController.MMControllerRID);
  }

  if(MMEntityData->AnimControllers[MMControllerIndex])
  {
    for(int i = 0; i < ANIM_CONTROLLER_MAX_ANIM_COUNT; i++)
    {
      MMEntityData->AnimControllers[MMControllerIndex]->AnimationIDs[i] = {};
      MMEntityData->AnimControllers[MMControllerIndex]->Animations[i]   = {};
    }
    MMEntityData->AnimControllers[MMControllerIndex]->AnimStateCount = 0;
  }

  mm_aos_entity_data LastController = GetEntityAOSMMData(MMEntityData->Count - 1, MMEntityData);
  CopyAOSMMEntityData(&RemovedController, &LastController);
  MMEntityData->Count--;
}

inline int32_t
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

void DrawFrameInfo(mm_frame_info AnimGoal, mat4 CoordinateFrame,
                   mm_info_debug_settings DebugSettings, vec3 BoneColor, vec3 VelocityColor,
                   vec3 TrajectoryColor, vec3 DirectionColor);

transform GetAnimRootMotionDelta(Anim::animation*                  RootMotionAnim,
                                 const Anim::animation_controller* C, bool MirrorRootMotionInX,
                                 float LocalSampleTime, float dt);

// Input controlled on the left, trajectory controlled on the right
inline void
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
           (MMEntityData->FollowTrajectory[SmallestIndex] && !MMEntityData->FollowTrajectory[j]))
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
      if(MMEntityData->FollowTrajectory[i])
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

inline void
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

inline void
FetchAnimControllerPointers(Anim::animation_controller** OutAnimControllers,
                            const int32_t* EntityIndices, const entity* Entities, int32_t Count)
{
  for(int i = 0; i < Count; i++)
  {
    OutAnimControllers[i] = Entities[EntityIndices[i]].AnimController;
    assert(OutAnimControllers[i]);
  }
}

inline void
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

inline void
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

inline void
DrawGoalFrameInfos(const mm_frame_info* GoalInfos, const int32_t* EntityIndices, int32_t Count,
                   const entity* Entities, const mm_info_debug_settings* MMInfoDebug,
                   vec3 BoneColor = { 1, 0, 1 }, vec3 TrajectoryColor = { 0, 0, 1 },
                   vec3 DirectionColor = { 1, 0, 0 })
{
  for(int i = 0; i < Count; i++)
  {
    DrawFrameInfo(GoalInfos[i], TransformToMat4(Entities[EntityIndices[i]].Transform), *MMInfoDebug,
                  BoneColor, BoneColor, TrajectoryColor, DirectionColor);
  }
}

inline void
GenerateGoalsFromInput(mm_frame_info* OutGoals, mm_frame_info* OutMirroredGoals,
                       Memory::stack_allocator* TempAlloc, const blend_stack* BlendStacks,
                       const Anim::animation_controller* const* AnimControllers,
                       const mm_controller_data* const*         MMControllers,
                       const mm_input_control_params* ControlParams, const int32_t* EntityIndices,
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
    quat R            = Entities[EntityIndices[i]].Transform.R;
    R.V *= -1;
    vec3 GoalVelocity =
      Math::MulMat4Vec4(Math::Mat4Rotate(R), { ControlParams[i].MaxSpeed * Dir, 0 }).XYZ;

    blend_in_info DominantBlend = BlendStacks[i].Peek();

    GetMMGoal(&OutGoals[i], &OutMirroredGoals[i], TempAlloc, DominantBlend.AnimStateIndex,
              DominantBlend.Mirror, AnimControllers[i], GoalVelocity,
              MMControllers[i]->Params.FixedParams);
  }
}

inline void
GenerateGoalsFromSplines(mm_frame_info* OutGoals, const entity_trajectory_state* TrajectoryStates,
                         const Anim::animation_controller* const* ACs,
                         const blend_stack* BlendStacks, int32_t Count,
                         const movement_spline* Splines)
{
}

// Only used to visualize the mirrored match, not adequate way to flip for searching mirrors
inline mm_frame_info
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

inline void
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

inline void
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

inline void
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
