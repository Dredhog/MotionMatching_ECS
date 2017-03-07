#pragma once

union vec3 {
  struct
  {
    float X, Y, Z;
  };
  struct
  {
    float R, G, B;
  };
};

union vec4 {
  struct
  {
    float X, Y, Z, W;
  };
  struct
  {
    float R, G, B, A;
  };
  vec3 XYZ;
  vec3 RGB;
};

namespace Math
{
  float Dot(vec3 v1, vec3 v2);
  float Length(vec3 V);
  vec3 Normalized(vec3 V);
  vec3 Cross(vec3 A, vec3 B);
  vec3 operator-(const vec3& V);
  vec3 operator+(const vec3& A, const vec3& B);
  vec3 operator-(const vec3& A, const vec3& B);
  vec3 operator*(const vec3& A, float s);
  vec3 operator*(float s, const vec3& A);
  vec3 operator/(const vec3 A, float s);
  vec3 operator/(float s, const vec3 A);

  bool operator==(const vec3& A, const vec3& B);
  bool operator!=(const vec3& A, const vec3& B);
  vec3& operator+=(vec3& A, const vec3& B);
  vec3& operator-=(vec3& A, const vec3& B);
  vec3& operator*=(vec3& A, float s);
  vec3& operator/=(vec3& A, float s);

  float Dot(vec4 A, vec4 B);
  inline vec4 Vec4(vec3 V3, float W);
  vec4 operator-(const vec4& V);
  vec4 operator+(const vec4& A, const vec4& B);
  vec4 operator-(const vec4& A, const vec4& B);
  bool operator==(const vec4& A, const vec4& B);
  bool operator!=(const vec4& A, const vec4& B);
  vec4& operator+=(vec4& A, const vec4& B);
  vec4& operator-=(vec4& A, const vec4& B);
  vec4 operator*(const vec4& A, float s);
  vec4 operator*(float s, const vec4& A);
  vec4 operator/(const vec4& A, float s);
  vec4 operator/(float s, const vec4& A);
  vec4& operator*=(vec4& A, float s);
  vec4& operator/=(vec4& A, float s);
}

