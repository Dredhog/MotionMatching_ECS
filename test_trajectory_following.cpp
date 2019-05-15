#include "test_trajectory_following.h"
#include "debug_drawing.h"

vec3
ClosestPointToSegment(float* OutT, vec3 P, vec3 A, vec3 B)
{
  vec3 AB = B - A;
  vec3 AP = P - A;

  float AB_Squared = Math::Dot(AB, AB);

  float t = (AB_Squared != 0) ? ClampFloat(0, Math::Dot(AB, AP) / AB_Squared, 1) : 0;
  if(OutT)
	{
    *OutT = t;
  }
  return A + t * AB;
}

trajectory_follow_data_row
MeasureTrajectoryFollowing(transform EntityTransform, const spline_follow_state* FollowState,
                           const movement_spline* Target, float t, float dt)
{
  trajectory_follow_data_row Result = { .t                 = t,
                                        .dt                = dt,
                                        .NextWaypointIndex = FollowState->NextWaypointIndex };
  // Get Current Line
  movement_line CurrentLine =
    Target->GetCurrentLine(FollowState->NextWaypointIndex, FollowState->Loop);

  float tParam;
  // Get Signed Distance To Line
  vec3 ClosestPointOnLine  = ClosestPointToSegment(&tParam, EntityTransform.T, CurrentLine.A, CurrentLine.B);
  vec3 ClosestPointOnSpline =
    Target->CatmullRomPoint(FollowState->NextWaypointIndex, tParam, FollowState->Loop);
  Result.DistanceToSegment = Math::Length(EntityTransform.T - ClosestPointOnLine);
  Result.DistanceToSpline  = Math::Length(EntityTransform.T - ClosestPointOnSpline);

  float LineAngle = 0.0f;
  vec3  LineDir   = CurrentLine.B - CurrentLine.B;
  if(Math::Length(LineDir) != 0)
  {
		LineDir.Y = 0;
    LineDir   = Math::Normalized(LineDir);
    LineAngle = atan2f(LineDir.X, LineDir.Z);
  }

  // Compute Inverse 3x3 Line Transform
  mat3 InverseLineBasis = Math::Mat3RotateY(-LineAngle);

  // Draw green segment to closest point on line
  Debug::PushWireframeSphere(ClosestPointOnLine, 0.05f, { 0, 1, 0, 1 });
  Debug::PushLine(EntityTransform.T, ClosestPointOnLine, { 0, 1, 0, 1 });

  // Draw yellow segment to closest point on spline
  Debug::PushWireframeSphere(ClosestPointOnSpline, 0.05f, { 1, 1, 0, 1 });
  Debug::PushLine(EntityTransform.T, ClosestPointOnSpline, { 1, 1, 0, 1 });

  //Compute Signed Angle Between Line And Current Facing
  vec3 LineLocalFacing = Math::MulMat3Vec3(InverseLineBasis, { 0, 0, 1 });
  Result.SignedAngleFromLine = float(180.0f / M_PI) * atan2f(LineLocalFacing.X, LineLocalFacing.Z);

  //Do the same with the spline
	{
	}
	
  return Result;
}
