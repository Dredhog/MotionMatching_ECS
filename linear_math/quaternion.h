#pragma once
#include "matrix.h"

struct quat
{
  float S;
  union {
    struct
    {
      float i;
      float j;
      float k;
    };
    vec3 V;
  };
};

namespace Math
{
  inline mat3
  QuatToMat3(const quat& Q)
  {
    float X = Q.V.X;
    float Y = Q.V.Y;
    float Z = Q.V.Z;
    mat3  Result;
    Result._11 = 1 - 2 * (Y * Y + Z * Z);
    Result._22 = 1 - 2 * (X * X + Z * Z);
    Result._33 = 1 - 2 * (X * X + Y * Y);
    Result._12 = 2 * (X * Y - Q.S * Z);
    Result._13 = 2 * (X * Z + Q.S * Y);
    Result._23 = 2 * (Y * Z - Q.S * X);
    Result._21 = 2 * (X * Y + Q.S * Z);
    Result._31 = 2 * (X * Z - Q.S * Y);
    Result._32 = 2 * (Y * Z + Q.S * X);
    return Result;
  }

  inline float
  Length(quat Q)
  {
    return sqrtf(Q.S * Q.S + Q.i * Q.i + Q.j * Q.j + Q.k * Q.k);
  }

  inline quat&
  Normalize(quat* Q)
  {
    float Length = Math::Length(*Q);
    Q->S /= Length;
    Q->V /= Length;
    return *Q;
  }

  inline vec3
  QuatToEularAngles(quat Q)
  {
    vec3  Result;
    float ysQr = Q.V.Y * Q.V.Y;

    // roll(x - axis rotation)
    double t0 = 2.0f * (Q.S * Q.i + Q.j * Q.k);
    double t1 = 1.0f - 2.0f * (Q.i * Q.i + ysQr);
    Result.X  = (float)atan2(t0, t1);

    // pitch (y-axis rotation)
    double t2 = 2.0f * (Q.S * Q.j - Q.k * Q.i);
    t2        = ((t2 > 1.0f) ? 1.0f : t2);
    t2        = ((t2 < -1.0f) ? -1.0f : t2);
    Result.Y  = (float)asin(t2);

    // yaw (z-axis rotation)
    double t3 = 2.0f * (Q.S * Q.k + Q.i * Q.j);
    double t4 = 1.0f - 2.0f * (ysQr + Q.k * Q.k);
    Result.Z  = (float)atan2(t3, t4);
    return Result;
  }
}

inline quat operator*(float S, const quat& Q)
{
  quat Result = Q;
  Result.S *= S;
  Result.V *= S;
  return Result;
}

inline quat operator*(const quat& Q, float S)
{
  quat Result = Q;
  Result.S *= S;
  Result.V *= S;
  return Result;
}

inline quat operator*(const quat& A, const quat& B)
{
  quat Result;
  Result.S = A.S * B.S - Math::Dot(A.V, B.V);
  Result.V = (A.S * B.V) + (B.S * A.V) + Math::Cross(A.V, B.V);
  return Result;
}

