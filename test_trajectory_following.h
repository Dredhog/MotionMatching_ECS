#pragma once

struct trajectory_follow_data_row
{
	float t;
	float dt;
  float SignedDistFromLine;
  float SignedAngleFromLine;
  float SignedAngleFromSpline;
  float SignedDistFromSpline;
  float TotalTraveled;
  float TotalMotionAlongProjection;
  float TotalDistancePercentage;
  float NextWaypointIndex;
  float DesiredVelocity;
  float ActualVelocity;
};

struct follow_test
{
	float DistanceTraveled;
	float ProjectedDistanceTraveled;
};

inline movement_spline
GenerateSplineFromAnimation(Anim::skeleton* Skeleton, Anim::animation* Anim, float WaypointPeriod)
{
	movement_spline Result;
	//Sample Root at anim points
	
	//Trajsform everything the space of the first root

	//Trajsform everything the space of the first root
	
  return Result;
}

inline trajectory_follow_data_row
MeasureTrajectoryFollowing(Anim::skeleton* Skeleton, Anim::animation, float WaypointCount,
                           movement_spline* TargetTrajectory)
{
  trajectory_follow_data_row Result = {};
  return Result;
}

inline trajectory_follow_data_row
MeasureTrajectoryFollowing(Anim::skeleton* Skeleton, blend_stack* BlendStack, entity* Entity,
                           movement_spline* TargetTrajectory)
{
  trajectory_follow_data_row Result = {};
  return Result;
}
