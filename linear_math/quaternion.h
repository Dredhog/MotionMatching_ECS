#pragma once
#include <math.h>

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

inline quat
operator+(const quat& A, const quat& B)
{
  quat Result;

  Result.S = A.S + B.S;
  Result.i = A.i + B.i;
  Result.j = A.j + B.j;
  Result.k = A.k + B.k;

  return Result;
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

  inline float
  ReciprocalSqrt(float x)
  {
    long  i;
    float y, r;
    y = x * 0.5f;
    i = *(long*)(&x);
    i = 0x5f3759df - (i >> 1);
    r = *(float*)(&i);
    r = r * (1.5f - r * r * y);
    return r;
  }

  inline quat
  Mat4ToQuat(const mat4& OriginalM)
  {
    quat         q;
    const float* m = (float*)&OriginalM;
    if(m[0 + 4 * 0] + m[1 + 4 * 1] + m[2 + 4 * 2] > 0.0f)
    {
      float t = +m[0 + 4 * 0] + m[1 + 4 * 1] + m[2 + 4 * 2] + 1.0f;
      float s = ReciprocalSqrt(t) * 0.5f;

      q.S = s * t;
      q.V.Z = (m[0 + 4 * 1] - m[1 + 4 * 0]) * s;
      q.V.Y = (m[2 + 4 * 0] - m[0 + 4 * 2]) * s;
      q.V.X = (m[1 + 4 * 2] - m[2 + 4 * 1]) * s;
    }
    else if(m[0 + 4 * 0] > m[1 + 4 * 1] && m[0 + 4 * 0] > m[2 + 4 * 2])
    {
      float t = +m[0 + 4 * 0] - m[1 + 4 * 1] - m[2 + 4 * 2] + 1.0f;
      float s = ReciprocalSqrt(t) * 0.5f;

      q.V.X = s * t;
      q.V.Y = (m[0 + 4 * 1] + m[1 + 4 * 0]) * s;
      q.V.Z = (m[2 + 4 * 0] + m[0 + 4 * 2]) * s;
      q.S = (m[1 + 4 * 2] - m[2 + 4 * 1]) * s;
    }
    else if(m[1 * 4 + 1] > m[2 * 4 + 2])
    {
      float t = -m[0 + 4 * 0] + m[1 + 4 * 1] - m[2 + 4 * 2] + 1.0f;
      float s = ReciprocalSqrt(t) * 0.5f;

      q.V.Y = s * t;
      q.V.X = (m[0 + 4 * 1] + m[1 + 4 * 0]) * s;
      q.S = (m[2 + 4 * 0] - m[0 + 4 * 2]) * s;
      q.V.Z = (m[1 + 4 * 2] + m[2 + 4 * 1]) * s;
    }
    else
    {
      float t = -m[0 + 4 * 0] - m[1 + 4 * 1] + m[2 + 4 * 2] + 1.0f;
      float s = ReciprocalSqrt(t) * 0.5f;

      q.V.Z = s * t;
      q.S = (m[0 + 4 * 1] - m[1 + 4 * 0]) * s;
      q.V.X = (m[2 + 4 * 0] + m[0 + 4 * 2]) * s;
      q.V.Y = (m[1 + 4 * 2] + m[2 + 4 * 1]) * s;
    }
    // Conjugation (hopefully this is enought for rotation quaternions :D)
    q.V *= -1;
    return q;
  }

  inline float
  Length(quat Q)
  {
    return sqrtf(Q.S * Q.S + Q.i * Q.i + Q.j * Q.j + Q.k * Q.k);
  }

  inline mat4
  Mat4Rotate(const quat& Q)
  {
    float X = Q.V.X;
    float Y = Q.V.Y;
    float Z = Q.V.Z;
    mat4  Result;
    Result._11 = 1.0f - 2.0f * (Y * Y + Z * Z);
    Result._22 = 1.0f - 2.0f * (X * X + Z * Z);
    Result._33 = 1.0f - 2.0f * (X * X + Y * Y);
    Result._12 = 2.0f * (X * Y - Q.S * Z);
    Result._13 = 2.0f * (X * Z + Q.S * Y);
    Result._23 = 2.0f * (Y * Z - Q.S * X);
    Result._21 = 2.0f * (X * Y + Q.S * Z);
    Result._31 = 2.0f * (X * Z - Q.S * Y);
    Result._32 = 2.0f * (Y * Z + Q.S * X);

    Result._44 = 1.0f;

    Result._41 = 0.0f;
    Result._42 = 0.0f;
    Result._43 = 0.0f;

    Result._14 = 0.0f;
    Result._24 = 0.0f;
    Result._34 = 0.0f;
    return Result;
  }

  inline quat
  EulerToQuat(float Roll, float Pitch, float Yaw)
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
    Result.X = atan2f(t0, t1);

    // pitch (y-axis rotation)
    float t2 = 2.0f * (Q.S * Q.j - Q.k * Q.i);
    t2       = ((t2 > 1.0f) ? 1.0f : t2);
    t2       = ((t2 < -1.0f) ? -1.0f : t2);
    Result.Y = asinf(t2);

    // yaw (z-axis rotation)
    float t3 = 2.0f * (Q.S * Q.k + Q.i * Q.j);
    float t4 = 1.0f - 2.0f * (ysQr + Q.k * Q.k);
    Result.Z = atan2f(t3, t4);
    Result *= RAD_TO_DEG;
    return Result;
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

  inline quat
  QuatIdent()
  {
    quat Result = {};
    Result.S    = 1.0f;
    return Result;
  }

  inline quat
  QuatFromTo(vec3 From, vec3 To)
  {
    quat Result;
    Result.V = Math::Cross(From, To);
    Result.S = Math::Length(From) * Math::Length(To) + Math::Dot(From, To);
    Math::Normalize(&Result);
    return Result;
  }

	inline quat
	QuatAxisAngle(vec3 Axis, float Angle)
	{
		quat Result;
    Result.V = sinf(Angle / 2) * Math::Normalized(Axis);
    Result.S = cosf(Angle / 2);
    return Result;
  }

  inline quat
  QuatLerp(quat Q0, quat Q1, float t)
  {
    quat Result = {};

    // Compute the cosine of the angle between the two vectors.
    float Dot = Math::Dot(Q0.V, Q1.V) + Q0.S * Q1.S;

    // If the dot product is negative, slerp won't take
    // the shorter path. Note that v1 and -v1 are equivalent when
    // the negation is applied to all four components. Fix by
    // reversing one quaternion.
    if(Dot < 0.0f)
    {
      Q1.S = -Q1.S;
      Q1.V = -Q1.V;

      Dot = -Dot;
    }
    Result = (1.0f - t) * Q0 + t * Q1;
    Normalize(&Result);
    return Result;
  }

	/*inline quat
	QuatSlerp(const quat& A, const quat&B, float t)
	{
    return (A * sinf(1 - t) + B * sinf(t)) * (1 / sinf(t));
  }*/
}
