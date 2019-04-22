#pragma once
#include "trajectory.h"
#include "motion_matching.h"

#include <stdint.h>

enum spline_loop_type
{
  SPLINE_LoopToStart,
  SPLINE_ReverseWhenEnded,
};

struct entity_trajectory_state
{
  int32_t EntityIndex;
  int32_t SplineIndex;

  int32_t  NextSplineIndex;
  uint32_t LoopType;
  bool     MovingInPositive;
};

void
GenerateGoalsFromSplines(mm_frame_info* OutGoals, Memory::stack_allocator* TempAlloc,
                         entity_trajectory_state* TrajectoryStates int32_t TrajectoryStateCount,
                         const entity* Entities, const spline* Splines,
                         const& mm_matching_params Params)
{
  for(int i = 0; i < TrajectoryStateCount; i++)
  {
    mm_frame_info Goal            = {};
    vec3          CurrentVelocity = {};

    GetPoseGoal(&Goal, &CurrentVelocity, TempAlloc, CurrentAnimIndex, Mirror, Controller, Params)
			vec3 DesiredVelocity = 
    GetLongtermGoal(&Goal, CurrentVelocity)
  }
}
