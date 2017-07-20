#pragma once
#include "matrix.h"

const float LINEAR_MATH_PI = 3.1415926535f;

const float DEG_TO_RAD = LINEAR_MATH_PI / 180.0f;
const float RAD_TO_DEG = 1.0f / DEG_TO_RAD;

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
    Result._11 = 1.0f - 2.0f * (Y * Y + Z * Z);
    Result._22 = 1.0f - 2.0f * (X * X + Z * Z);
    Result._33 = 1.0f - 2.0f * (X * X + Y * Y);
    Result._12 = 2.0f * (X * Y - Q.S * Z);
    Result._13 = 2.0f * (X * Z + Q.S * Y);
    Result._23 = 2.0f * (Y * Z - Q.S * X);
    Result._21 = 2.0f * (X * Y + Q.S * Z);
    Result._31 = 2.0f * (X * Z - Q.S * Y);
    Result._32 = 2.0f * (Y * Z + Q.S * X);
    return Result;
  }

  inline quat
  EulerToQuat(float Pitch, float Roll, float Yaw)
  {
    Pitch *= DEG_TO_RAD;
    Roll *= DEG_TO_RAD;
    Yaw *= DEG_TO_RAD;

    float t0 = cosf(Yaw * 0.5f);
    float t1 = sinf(Yaw * 0.5f);
    float t2 = cosf(Roll * 0.5f);
    float t3 = sinf(Roll * 0.5f);
    float t4 = cosf(Pitch * 0.5f);
    float t5 = sinf(Pitch * 0.5f);

    quat q;
    q.S = t0 * t2 * t4 + t1 * t3 * t5;
    q.i = t0 * t3 * t4 - t1 * t2 * t5;
    q.j = t0 * t2 * t5 + t1 * t3 * t4;
    q.k = t1 * t2 * t4 - t0 * t3 * t5;
    return q;
  }

  inline quat
  EulerToQuat(vec3 Rotation)
  {
    return EulerToQuat(Rotation.X, Rotation.Y, Rotation.Z);
  }

  inline vec3
  QuatToEuler(quat Q)
  {
    vec3  Result;
    float ysQr = Q.V.Y * Q.V.Y;

    // roll(x - axis rotation)
    float t0 = 2.0f * (Q.S * Q.i + Q.j * Q.k);
    float t1 = 1.0f - 2.0f * (Q.i * Q.i + ysQr);
    Result.Y = atan2f(t0, t1);

    // pitch (y-axis rotation)
    float t2 = 2.0f * (Q.S * Q.j - Q.k * Q.i);
    t2       = ((t2 > 1.0f) ? 1.0f : t2);
    t2       = ((t2 < -1.0f) ? -1.0f : t2);
    Result.X = asinf(t2);

    // yaw (z-axis rotation)
    float t3 = 2.0f * (Q.S * Q.k + Q.i * Q.j);
    float t4 = 1.0f - 2.0f * (ysQr + Q.k * Q.k);
    Result.Z = atan2f(t3, t4);
    Result *= RAD_TO_DEG;
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
    Q->i /= Length;
    Q->j /= Length;
    Q->k /= Length;
    return *Q;
  }
}

inline quat operator*(float S, const quat& Q)
{
  quat Result = Q;
  Result.S *= S;
  Result.i *= S;
  Result.j *= S;
  Result.k *= S;
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

