#pragma once

#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "misc.h"

struct raycast_result
{
  vec3 IntersectP;
  bool Success;
};

raycast_result
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

vec3
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

raycast_result
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
    Result.IntersectP = Orig + t * Dir;
  }
  return Result;
}
