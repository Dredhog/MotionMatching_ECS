
struct spline_displacement_data_row
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

spline_displacement_data_row
ComputeTrajectoryMeasurements()
{
}

movement_spline
GenerateSplineFromAnimation(Anim::skeleton* Skeleton, Aniim::animation* Anim, float WaypointPeriod)
{
	movement_spline Result;
	//Sample Root at anim points
	
	//Trajsform everything the space of the first root

	//Trajsform everything the space of the first root
	
  return result;
}

spline_displacement_data_row
ComputeAnimationOffsetFromSpline(Anim::skeleton*Skeleton, Anim::animation, float WaypointCount)
{
  movement_spline AnimSpline = GenerateSplineFromAnimation(Skeleton, AnimSpline, WaypointPeriod);
}
