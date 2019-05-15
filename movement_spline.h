#pragma once

#include "basic_data_structures.h"
#include "linear_math/vector.h"
#include "misc.h"

#define WAYPOINT_CAPACITY_PER_TRAJECTORY 20
#define TRAJECTORY_CAPACITY 20

struct waypoint
{
  vec3  Position;
  vec2  Facing;
  float Velocity;
};

struct spline_follow_state
{
  int32_t SplineIndex;

  int32_t NextWaypointIndex;
  bool    Loop;
  bool    MovingInPositive;
};

struct movement_line
{
  vec3 A;
  vec3 B;
};

inline float
GetCatmullRomPoint(float A, float B, float C, float D, float t)
{
  assert(0 <= t && t <= 1);

  float ttt = t * t * t;
  float tt  = t * t;

  float wA = -ttt + 2 * tt - t;
  float wB = 3 * ttt - 5 * tt + 2;
  float wC = -3 * ttt + 4 * tt + t;
  float wD = ttt - tt;

  return 0.5f * (A * wA + B * wB + C * wC + D * wD);
}

inline float
GetCatmullRomGradient(float A, float B, float C, float D, float t)
{
  assert(0 <= t && t <= 1);

  float tt  = t * t;

  float wA = -3*tt + 4 * t - 1;
  float wB = 9 * tt - 10 * t;
  float wC = -9 * tt + 8 * t + 1;
  float wD = 3 * tt - 2 * t;

  return 0.5f * (A * wA + B * wB + C * wC + D * wD);
}

inline vec3
GetCatmullRomPoint(vec3 A, vec3 B, vec3 C, vec3 D, float t)
{
  vec3 Result = { GetCatmullRomPoint(A.X, B.X, C.X, D.X, t),
                  GetCatmullRomPoint(A.Y, B.Y, C.Y, D.Y, t),
                  GetCatmullRomPoint(A.Z, B.Z, C.Z, D.Z, t) };
  return Result;
}

inline vec3
GetCatmullRomGradient(vec3 A, vec3 B, vec3 C, vec3 D, float t)
{
  vec3 Result = { GetCatmullRomGradient(A.X, B.X, C.X, D.X, t),
                  GetCatmullRomGradient(A.Y, B.Y, C.Y, D.Y, t),
                  GetCatmullRomGradient(A.Z, B.Z, C.Z, D.Z, t) };
  return Result;
}

struct movement_spline
{
  fixed_stack<waypoint, WAYPOINT_CAPACITY_PER_TRAJECTORY> Waypoints;

  inline void
  GetSplineControlPointIndices(int* IndA, int* IndB, int* IndC, int* IndD, int NextIndex,
                               bool Loop = false) const
  {
    int Count = Waypoints.Count;
    assert(0 < Count);

    *IndA = NextIndex - 2;
    *IndB = NextIndex - 1;
    *IndC = NextIndex - 0;
    *IndD = NextIndex + 1;

    if(Loop)
    {
      *IndA = (*IndA + Count) % Count;
      *IndB = (*IndB + Count) % Count;
      *IndC = (*IndC + Count) % Count;
      *IndD = (*IndD + Count) % Count;
    }

    *IndA = ClampInt32InIn(0, *IndA, Waypoints.Count - 1);
    *IndB = ClampInt32InIn(0, *IndB, Waypoints.Count - 1);
    *IndC = ClampInt32InIn(0, *IndC, Waypoints.Count - 1);
    *IndD = ClampInt32InIn(0, *IndD, Waypoints.Count - 1);
  }

  inline vec3
  CatmullRomPoint(int NextIndex, float t, bool Loop = false) const
  {
		int IndA;
		int IndB;
		int IndC;
		int IndD;
    GetSplineControlPointIndices(&IndA, &IndB, &IndC, &IndD, NextIndex, Loop);

    vec3 A = Waypoints[IndA].Position;
    vec3 B = Waypoints[IndB].Position;
    vec3 C = Waypoints[IndC].Position;
    vec3 D = Waypoints[IndD].Position;

    return GetCatmullRomPoint(A, B, C, D, t);
  }

  inline vec3
  CatmullRomGradient(int NextIndex, float t, bool Loop = false) const
  {
    int IndA;
    int IndB;
    int IndC;
    int IndD;
    GetSplineControlPointIndices(&IndA, &IndB, &IndC, &IndD, NextIndex, Loop);

    vec3 A = Waypoints[IndA].Position;
    vec3 B = Waypoints[IndB].Position;
    vec3 C = Waypoints[IndC].Position;
    vec3 D = Waypoints[IndD].Position;

    return GetCatmullRomGradient(A, B, C, D, t);
  }

  inline movement_line
  GetCurrentLine(int32_t NextIndex, bool Looped = false) const
  {
    int IndA  = NextIndex - 1;
    int IndB  = NextIndex;
    int Count = Waypoints.Count;
    assert(Count > 0);

    if(Looped)
    {
      IndA = (IndA + Count) % Count;
      IndB = (IndB + Count) % Count;
    }

    IndA = ClampInt32InIn(0, IndA, Waypoints.Count - 1);
    IndB = ClampInt32InIn(0, IndB, Waypoints.Count - 1);

    movement_line Result = { Waypoints[IndA].Position, Waypoints[IndB].Position };
    return Result;
  }
};

struct spline_system
{
  fixed_stack<movement_spline, TRAJECTORY_CAPACITY> Splines;

  bool    IsWaypointPlacementMode;
  int32_t SelectedSplineIndex;
  int32_t SelectedWaypointIndex;
};
