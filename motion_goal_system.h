#pragma once

#include "trajectory.h"
#include "motion_matching.h"
#include "rid.h"

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
	//Serializable data
  int32_t Count;
  rid                     MMControllerRIDs[MM_CONTROLLER_MAX_COUNT]; // Persistent/Const
  blend_stack             BlendStacks[MM_CONTROLLER_MAX_COUNT];      // Persistent
  int32_t                 EntityIndices[MM_CONTROLLER_MAX_COUNT];    // Persistent
  bool                    FollowTrajectory[MM_CONTROLLER_MAX_COUNT]; // Persistent
  entity_trajectory_state TrajectoryStates[MM_CONTROLLER_MAX_COUNT]; // persistent
  mm_input_control_params InputControlParams[MM_CONTROLLER_MAX_COUNT]; // persistent

  // Intermediate data
  mm_controller_data* MMControllers[MM_CONTROLLER_MAX_COUNT]; // One Frame
  mm_frame_info       AnimGoals[MM_CONTROLLER_MAX_COUNT]; // One Frame

  // Output data
  transform OutDeltaRootMotion[MM_CONTROLLER_MAX_COUNT]; // One Frame
};

//Used in the editor to access the soa fields
struct mm_aos_entity_data
{
  rid* const                     MMControllerRID;
  blend_stack* const             BlendStack;
  int32_t* const                 EntityIndex;
  entity_trajectory_state* const TrajectoryState;
  bool* const                    FollowTrajectory;
  mm_input_control_params* const InputControlParams;

  // Intermediate data
  mm_controller_data** const MMController;
  mm_frame_info* const      AnimGoal;
};

inline void SetDefaultMMControllerFileds(mm_aos_entity_data* MMEntityData)
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

  *Dest->MMController = *Src->MMController;
  *Dest->AnimGoal     = *Src->AnimGoal;
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
  };
  return Result;
}

inline void
CopyMMEntityData(int32_t DestIndex, int32_t SourceIndex, mm_entity_data* MMEntityData)
{
  mm_aos_entity_data Dest = GetEntityAOSMMData(DestIndex, MMEntityData);
  mm_aos_entity_data Source = GetEntityAOSMMData(SourceIndex, MMEntityData);
  CopyAOSMMEntityData(&Dest, &Source);
}

inline void
RemoveMMControllerDataAtIndex(int32_t MMControllerIndex, mm_entity_data* MMEntityData)
{
  assert(0 <= MMControllerIndex && MMControllerIndex < MMEntityData->Count);
  mm_aos_entity_data RemovedController = GetEntityAOSMMData(MMControllerIndex, MMEntityData);
  // TODO(Lukas) remove any assocaited data references e.g. resource references
  mm_aos_entity_data LastController    = GetEntityAOSMMData(MMEntityData->Count - 1, MMEntityData);
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
GenerateGoalsFromSplines(mm_frame_info* OutGoals, Memory::stack_allocator* TempAlloc,
                         entity_trajectory_state* TrajectoryStates, int32_t TrajectoryStateCount,
                         const entity* Entities, const movement_spline* Splines,
                         const mm_params& Params)
{
  for(int i = 0; i < TrajectoryStateCount; i++)
  {
    mm_frame_info Goal            = {};
    vec3          CurrentVelocity = {};

    //GetPoseGoal(&Goal, &CurrentVelocity, TempAlloc, CurrentAnimIndex, Mirror, Controller, Params);
    //vec3 DesiredVelocity = GetLongtermGoal(&Goal, CurrentVelocity);
  }
}
