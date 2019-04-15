#pragma once 

#include "basic_data_structures.h"

#define WAYPOINT_CAPACITY_PER_TRAJECTORY 20
#define TRAJECTORY_CAPACITY 10

struct waypoint
{
	vec3 Position;
	vec2 Facing;
	float Velocity;
};

enum spline_loop_type
{
  SPLINE_LoopToStart,
  SPLINE_ReverseWhenEnded,
};

struct movement_spline
{
	fixed_stack<waypoint, WAYPOINT_CAPACITY_PER_TRAJECTORY> Waypoints;
  uint32_t LoopType;
};

struct trajectory_system
{
  fixed_stack<movement_spline, TRAJECTORY_CAPACITY> Splines;

  bool    IsWaypointPlacementMode;
  int32_t SelectedSplineIndex;
  int32_t SelectedWaypointIndex;
};
