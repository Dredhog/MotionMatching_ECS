#pragma once

#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "misc.h"

struct raycast_result
{
  vec3 IntersectP;
  float t;
  bool Success;
};

union parametric_plane {
  vec3 v;
  struct
  {
    vec3 n;
    vec3 u;
    vec3 o;
  };
};

inline raycast_result
RayIntersectPlane(vec3 Orig, vec3 Dir, vec3 PlaneP, vec3 Norm)
{
  raycast_result Result = {};

  float DirDotNorm = Math::Dot(Dir, Norm);
  if(DirDotNorm != 0)
  {
    float t = -(Math::Dot(Orig, Norm) + Math::Dot(PlaneP, Norm)) / DirDotNorm;

    if(t > 0)
    {
      Result.Success    = true;
      Result.IntersectP = Orig + t * Dir;
    }
  }

  return Result;
}

inline vec3
GetRayDirFromScreenP(vec3 NormScreenP, mat4 ProjectionMatrix, mat4 CameraMatrix)
{
  // Normalised Device Coords
  vec3 NDC = 2 * NormScreenP + vec3{ -1, -1, 0 };
  // Homogennous Clip Coords
  vec4 ClipDir  = { NDC.X, NDC.Y, -1, 1 };
  vec4 EyeDir   = Math::MulMat4Vec4(Math::InvMat4(ProjectionMatrix), ClipDir);
  EyeDir        = vec4{ EyeDir.X, EyeDir.Y, -1, 0 };
  vec3 WorldDir = Math::MulMat4Vec4(Math::InvMat4(CameraMatrix), EyeDir).XYZ;

  return Math::Normalized(WorldDir);
}

inline raycast_result
RayIntersectSphere(vec3 Orig, vec3 Dir, vec3 SphereP, float Radius)
{
  raycast_result Result = {};

  vec3 RayOrigToSphereP = Orig - SphereP;

  float b = Math::Dot(Dir, RayOrigToSphereP);
  float c = Math::Dot(RayOrigToSphereP, RayOrigToSphereP) - Radius * Radius;

  float RootDet = sqrtf(b * b - c);
  float t1      = -b + RootDet;
  float t2      = -b - RootDet;
  float t       = MinFloat(t1, t2);
  if(t > 0)
  {
    Result.Success    = true;
    Result.t          = t;
    Result.IntersectP = Orig + t * Dir;
  }
  return Result;
}

struct line
{
  vec3 P0;
  vec3 P1;
};

#define AssertUnit(expression)                                                                     \
  assert(FloatsEqualByThreshold(Math::Length((expression)), 1.0f, 0.001))

#define EPSILON 0.0001f
inline float
MinDistBetweenLines(float* sc, float* tc, line L1, line L2)
{
  vec3  u = L1.P1 - L1.P0;
  vec3  v = L2.P1 - L2.P0;
  vec3  w = L1.P0 - L2.P0;
  float a = Math::Dot(u, u); // always >= 0
  float b = Math::Dot(u, v);
  float c = Math::Dot(v, v); // always >= 0
  float d = Math::Dot(u, w);
  float e = Math::Dot(v, w);
  float D = a * c - b * b; // always >= 0

  // compute the line parameters of the two closest points
  if(D < EPSILON)
  { // the lines are almost parallel
    *sc = 0.0;
    *tc = (b > c ? d / b : e / c); // use the largest denominator
  }
  else
  {
    *sc = (b * e - c * d) / D;
    *tc = (a * e - b * d) / D;
  }

  // get the difference of the two closest points
  vec3 dP = w + (*sc * u) - (*tc * v); // =  L1(sc) - L2(tc)

  return Math::Length(dP); // return the closest distance
}

inline bool
IntersectRayParametricPlane(float* u, vec3 o, vec3 dir, parametric_plane p)
{
  float d = Math::Dot(p.n, p.o);
  float t = (d - Math::Dot(p.n, o)) / Math::Dot(p.n, dir);

  if(0 <= t)
  {
    vec3 planeSpaheHitP = (o + t * dir) - p.o;

    // Compute position in plane space
    *u = Math::Dot(p.u, planeSpaheHitP);
    return true;
  }

  return false;
}

inline bool
IntersectRayParametricPlane(float* u, float* v, vec3 o, vec3 dir, parametric_plane p,
                            float RangeMin, float RangeMax)
{
  assert(u);
  assert(v);
  vec3  Normal = Math::Normalized(Math::Cross(p.u, p.v));
  float d      = Math::Dot(Normal, p.o);
  float t      = (d - Math::Dot(Normal, o)) / Math::Dot(Normal, dir);

  if(0 <= t)
  {
    vec3 PlaneSpaceHitP = (o + t * dir) - p.o;

    // Compute position in plane space
    {
      *u = Math::Dot(p.u, PlaneSpaceHitP) / Math::Length(p.u);
      if(*u < RangeMin || RangeMax < *u)
      {
        return false;
      }
    }

    {
      *v = Math::Dot(p.v, PlaneSpaceHitP) / Math::Length(p.v);
      if(*v < RangeMin || RangeMax < *v)
      {
        return false;
      }
    }

    return true;
  }

  return false;
}

inline float
ClosestPtSegmentSegment(float* outS, float* outT, vec3* outC1, vec3* outC2, vec3 p1, vec3 q1,
                        vec3 p2, vec3 q2)
{
  vec3  d1 = q1 - p1; // Direction vector of segment S1
  vec3  d2 = q2 - p2; // Direction vector of segment S2
  vec3  r  = p1 - p2;
  float a  = Math::Dot(d1, d1); // Squared length of segment S1, always nonnegative
  float e  = Math::Dot(d2, d2); // Squared length of segment S2, always nonnegative
  float f  = Math::Dot(d2, r);

  float s;
  float t;
  vec3  c1;
  vec3  c2;
  // Check if either or both segments degenerate into points
  if(a <= EPSILON && e <= EPSILON)
  {
    // Both segments degenerate into points
    s = t = 0.0f;
    c1    = p1;
    c2    = p2;
    return Math::Length(c1 - c2);
  }
  if(a <= EPSILON)
  {
    // First segment degenerates into a point
    s = 0.0f;
    t = f / e; // s = 0 => t = (b*s + f) / e = f / e
    t = ClampFloat(0.0f, t, 1.0f);
  }
  else
  {
    float c = Math::Dot(d1, r);
    if(e <= EPSILON)
    {
      // Second segment degenerates into a point
      t = 0.0f;
      s = ClampFloat(0.0f, -c / a, 1.0f); // t = 0 => s = (b*t - c) / a = -c / a
    }
    else
    {
      // The general nondegenerate case starts here
      float b     = Math::Dot(d1, d2);
      float denom = a * e - b * b; // Always nonnegative
      // If segments not parallel, compute closest point on L1 to L2 and
      // clamp to segment S1. Else pick arbitrary s (here 0)
      if(denom != 0.0f)
      {
        s = ClampFloat(0.0f, (b * f - c * e) / denom, 1.0f);
      }
      else
      {
        s = 0.0f;
      }
      // Compute point on L2 closest to S1(s) using
      // t = Dot((P1 + D1*s) - P2,D2) / Dot(D2,D2) = (b*s + f) / e
      t = (b * s + f) / e;
      // If t in [0,1] done. Else clamp t, recompute s for the new value
      // of t using s = Dot((P2 + D2*t) - P1,D1) / Dot(D1,D1)= (t*b - c) / a
      // and clamp s to [0, 1]
      if(t < 0.0f)
      {
        t = 0.0f;
        s = ClampFloat(0.0f, -c / a, 1.0f);
      }
      else if(t > 1.0f)
      {
        t = 1.0f;
        s = ClampFloat(0.0f, (b - c) / a, 1.0f);
      }
    }
  }
  c1 = p1 + d1 * s;
  c2 = p2 + d2 * t;

  *outS  = s;
  *outT  = t;
  *outC1 = c1;
  *outC2 = c2;
  return Math::Length(c1 - c2);
}

inline float
MinDistRaySegment(float* DistToClosest, vec3 RayOrig, vec3 RayDir, vec3 p0, vec3 p1)
{
  vec3 c0;
  vec3 c1;

  const float RayLength = 10000.0f;

  vec3  r0 = RayOrig;
  vec3  r1 = r0 + RayLength * RayDir;
  float s;
  float t;
  float Dist     = ClosestPtSegmentSegment(&s, &t, &c0, &c1, r0, r1, p0, p1);
  *DistToClosest = Math::Length(c0 - r0);
  return Dist;
}

#if 0
bool
TestRayAxis(float* outS, float* outT, vec3 RayOrig, vec3 RayDir, vec3 AxisOrig, vec3 AxisDir, float AxisPadding)
{
  assert(0 < AxisPadding);
  RayDir  = Math::Normalized(RayDir);
  AxisDir = Math::Normalized(AxisDir);

  float s;
  float t;
  line  RayLine = { .P0 = RayOrig, .P1 = RayOrig + RayDir };
  line  AxisLine = { .P0 = AxisOrig, .P1 = AxisOrig + AxisDir };
  float MinDist  = MinDistBetweenLines(&s, &t, RayLine, AxisLine);
	if(outS)
	{
		*outS = s;
	}
	if(outT)
	{
    *outT = t;
  }
  return MinDist <= AxisPadding;
}
#endif
