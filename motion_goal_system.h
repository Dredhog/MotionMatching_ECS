#pragma once

#include "trajectory.h"
#include "motion_matching.h"
#include "rid.h"

#include "resource_manager.h"
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
  mm_controller_data*         MMControllers[MM_CONTROLLER_MAX_COUNT]; // One Frame
  Anim::animation_controller* AnimControllers[MM_CONTROLLER_MAX_COUNT]; // One Frame
  mm_frame_info               AnimGoals[MM_CONTROLLER_MAX_COUNT];     // One Frame

  // Output data
  transform OutDeltaRootMotions[MM_CONTROLLER_MAX_COUNT]; // One Frame
};

// Used in the editor to access the soa fields
struct mm_aos_entity_data
{
  rid* const                     MMControllerRID;
  blend_stack* const             BlendStack;
  int32_t* const                 EntityIndex;
  entity_trajectory_state* const TrajectoryState;
  bool* const                    FollowTrajectory;
  mm_input_control_params* const InputControlParams;

  // Intermediate data
  mm_controller_data** const         MMController;
  Anim::animation_controller** const AnimController;
  mm_frame_info* const               AnimGoal;
};

inline void
SetDefaultMMControllerFileds(mm_aos_entity_data* MMEntityData)
{

  *MMEntityData->MMControllerRID    = {};
  *MMEntityData->BlendStack         = {};
  *MMEntityData->EntityIndex        = -1;
  *MMEntityData->TrajectoryState    = { .SplineIndex       = -1,
                                     .NextWaypointIndex = 0,
                                     .LoopType          = 0,
                                     .MovingInPositive  = true };
  *MMEntityData->FollowTrajectory   = false;
  *MMEntityData->InputControlParams = { .MaxSpeed     = 1.0f,
                                        .Acceleration = 2.0f,
                                        .UseStrafing  = false };

  *MMEntityData->MMController = NULL;
  *MMEntityData->AnimGoal     = {};
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

  *Dest->MMController   = *Src->MMController;
  *Dest->AnimController = *Src->AnimController;
  *Dest->AnimGoal       = *Src->AnimGoal;
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

  mm_controller_data*         TempMMController   = *A->MMController;
  Anim::animation_controller* TempAnimController = *A->AnimController;
  mm_frame_info               TempAnimGoal       = *A->AnimGoal;

  *A->MMControllerRID    = TempMMControllerRID;
  *A->BlendStack         = TempBlendStack;
  *A->EntityIndex        = TempEntityIndex;
  *A->TrajectoryState    = TempTrajectoryState;
  *A->FollowTrajectory   = TempFollowTrajectory;
  *A->InputControlParams = TempInputControlParams;
  *A->MMController       = TempMMController;
  *A->AnimController     = TempAnimController;
  *A->AnimGoal           = TempAnimGoal;
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
RemoveMMControllerDataAtIndex(int32_t MMControllerIndex, mm_entity_data* MMEntityData)
{
  assert(0 <= MMControllerIndex && MMControllerIndex < MMEntityData->Count);
  mm_aos_entity_data RemovedController = GetEntityAOSMMData(MMControllerIndex, MMEntityData);
  // TODO(Lukas) remove any assocaited data references e.g. resource references
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

inline void
GenerateGoalsFromSplines(mm_frame_info* OutGoals, const entity_trajectory_state* TrajectoryStates,
                         const Anim::animation_controller** const* AnimControllers,
                         const blend_stack* BlendStacks, int32_t TrajectoryStateCount,
                         const movement_spline* Splines)
{
  for(int i = 0; i < TrajectoryStateCount; i++)
  {
    mm_frame_info Goal            = {};
    vec3          CurrentVelocity = {};

    // GetPoseGoal(&Goal, &CurrentVelocity, TempAlloc, CurrentAnimIndex, Mirror, Controller,
    // Params);  vec3 DesiredVelocity = GetLongtermGoal(&Goal, CurrentVelocity);
  }
}

//Input controlled on the left, trajectory controlled on the right
inline int32_t
SortMMEntityDataByTrajectoryUsage(mm_entity_data* MMEntityData)
{
  int32_t ResultIndex = MMEntityData->Count;

  for(int i = 0; i < MMEntityData->Count-1; i++)
  {
    if(!MMEntityData->FollowTrajectory[i])
      continue;

    for (int j = i + 1; j < MMEntityData->Count; j++)
    {
      if(i != j && !MMEntityData->FollowTrajectory[j])
      {
        mm_aos_entity_data A = GetEntityAOSMMData(i, MMEntityData);
        mm_aos_entity_data B = GetEntityAOSMMData(j, MMEntityData);
        SwapMMEntityData(&A, &B);
        continue;
      }
    }
  }
	for(int i = 0; i < MMEntityData->Count; i++)
	{
		if(MMEntityData->FollowTrajectory[i])
		{
			ResultIndex = i;
		}
	}
	return ResultIndex;
}

inline void
FetchMMControllerDataPointers(Resource::resource_manager* Resources,
                              mm_controller_data** OutMMControllers, rid* MMControllerRIDs,
                              int32_t Count)
{
}

inline void
FetchAnimationPointers(Resource::resource_manager* Resources, Anim::animation_controller** InOutACs,
                       int32_t Count)
{
}

inline void
GenerateGoalsFromInput(mm_frame_info* OutGoals, const Anim::animation_controller* const* ACs,
                       const blend_stack* BlendStacks, int32_t Count, const game_input* Input)
{
}

inline void
GenerateGoalsFromSplines(mm_frame_info* OutGoals, const entity_trajectory_state* TrajectoryStates,
                         const Anim::animation_controller* const* ACs,
                         const blend_stack* BlendStacks, int32_t Count,
                         const movement_spline* Splines)
{
}

inline void
MotionMatchGoals(blend_stack* OutBlendStacks, const mm_frame_info* AnimGoals, int32_t Count)
{
}

inline void
ComputeRootMotion(transform* OutDeltaRootMotions, const blend_stack* BlendStacks, int32_t Count)
{
}

inline void
ApplyRootMotion(entity* InOutEntites, const transform* DeltaRootMotions, int32_t* EntityIndices,
                int32_t Count)
{
}
