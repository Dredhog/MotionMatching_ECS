#pragma once

#include "goal_gen.h"
#include "movement_spline.h"

struct trajectory_follow_data_row
{
  float t;
  float dt;

  float   DistanceToSegment;
  float   DistanceToSpline;
  float   SignedDistFromLine;
  float   SignedAngleFromLine;
  float   SignedAngleFromSpline;
  float   SignedDistFromSpline;
  float   TotalDistanceTraveled;
  float   TotalProjectedDistanceTraveled;
  float   PercentageInTrajectory;
  int32_t NextWaypointIndex;
  float   DesiredVelocity;
  float   ActualVelocity;
};

struct follow_test
{
  float ElapsedTime;
  float DistanceTraveled;
	float ProjectedDistanceTraveled;
};

trajectory_follow_data_row MeasureTrajectoryFollowing(transform                  EntityTransform,
                                                      const spline_follow_state* FollowState,
                                                      const movement_spline* Target, float t,
                                                      float dt);
