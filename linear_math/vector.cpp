#include "vector.h"
#include "math.h"

vec3
operator-(const vec3& V)
{
  return vec3{ -V.X, -V.Y, -V.Z };
}

vec3
operator+(const vec3& A, const vec3& B)
{
  return vec3{ A.X + B.X, A.Y + B.Y, A.Z + B.Z };
}

vec3
operator-(const vec3& A, const vec3& B)
{
  return vec3{ A.X - B.X, A.Y - B.Y, A.Z - B.Z };
}

bool
operator==(const vec3& A, const vec3& B)
{
  return (A.X == B.X && A.Y == B.Y && A.Z == B.Z);
}

bool
operator!=(const vec3& A, const vec3& B)
{
  return (A.X != B.X || A.Y != B.Y || A.Z != B.Z);
}

vec3&
operator+=(vec3& A, const vec3& B)
{
  A.X += B.X;
  A.Y += B.Y;
  A.Z += B.Z;
  return A;
}

vec3&
operator-=(vec3& A, const vec3& B)
{
  A.X -= B.X;
  A.Y -= B.Y;
  A.Z -= B.Z;
  return A;
}

vec3 operator*(const vec3& A, float s)
{
  return vec3{ A.X * s, A.Y * s, A.Z * s };
}

vec3 operator*(float s, const vec3& A)
{
  return A * s;
}

vec3
operator/(const vec3 A, float s)
{
  return vec3{ A.X / s, A.Y / s, A.Z / s };
}

vec3
operator/(float s, const vec3 A)
{
  return A / s;
}

vec3&
operator*=(vec3& A, float s)
{
  A.X *= s;
  A.Y *= s;
  A.Z *= s;
  return A;
}

vec3&
operator/=(vec3& A, float s)
{
  A.X /= s;
  A.Y /= s;
  A.Z /= s;
  return A;
}

float
Math::Dot(vec3 v1, vec3 v2)
{
  return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}

float
Math::Length(vec3 V)
{
  return sqrtf(V.X * V.X + V.Y * V.Y + V.Z * V.Z);
}

vec3
Math::Normalized(vec3 V)
{
  return V / Length(V);
}

vec3
Math::Cross(vec3 A, vec3 B)
{
  return vec3{ A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X };
}

vec4
operator-(const vec4& V)
{
  return vec4{ -V.X, -V.Y, -V.Z, -V.W };
}

vec4
operator+(const vec4& A, const vec4& B)
{
  return vec4{ A.X + B.X, A.Y + B.Y, A.Z + B.Z, A.W + B.W };
}
vec4
operator-(const vec4& A, const vec4& B)
{
  return vec4{ A.X - B.X, A.Y - B.Y, A.Z - B.Z, A.W - B.W };
}

bool
operator==(const vec4& A, const vec4& B)
{
  return (A.X == B.X && A.Y == B.Y && A.Z == B.Z && A.W == B.W);
}

bool
operator!=(const vec4& A, const vec4& B)
{
  return (A.X != B.X || A.Y != B.Y || A.Z != B.Z || A.W != B.W);
}

vec4&
operator+=(vec4& A, const vec4& B)
{
  A.X += B.X;
  A.Y += B.Y;
  A.Z += B.Z;
  A.W += B.W;
  return A;
}

vec4&
operator-=(vec4& A, const vec4& B)
{
  A.X -= B.X;
  A.Y -= B.Y;
  A.Z -= B.Z;
  A.W -= B.W;
  return A;
}

vec4 operator*(const vec4& A, float s)
{
  return vec4{ A.X * s, A.Y * s, A.Z * s, A.W * s };
}

vec4 operator*(float s, const vec4& A)
{
  return A * s;
}

vec4
operator/(const vec4& A, float s)
{
  return vec4{ A.X / s, A.Y / s, A.Z / s, A.W * s };
}

vec4
operator/(float s, const vec4& A)
{
  return A / s;
}

vec4&
operator*=(vec4& A, float s)
{
  A.X *= s;
  A.Y *= s;
  A.Z *= s;
  A.W *= s;
  return A;
}

vec4&
operator/=(vec4& A, float s)
{
  A.X /= s;
  A.Y /= s;
  A.Z /= s;
  A.W /= s;
  return A;
}

float
Math::Dot(vec4 A, vec4 B)
{
  return A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;
}

vec4
Math::Vec4(vec3 V3, float W)
{
  return { V3.X, V3.Y, V3.Z, W };
}

vec3
Math::Vec4ToVec3(vec4 V4)
{
  return { V4.X, V4.Y, V4.Z };
}
