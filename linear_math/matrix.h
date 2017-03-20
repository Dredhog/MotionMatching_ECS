#pragma once
#include "vector.h"

union mat3 {
  struct
  {
    float _11, _21, _31;
    float _12, _22, _32;
    float _13, _23, _33;
  };
  float e[9];
};

union mat4 {
  struct
  {
    float _11, _21, _31, _41;
    float _12, _22, _32, _42;
    float _13, _23, _33, _43;
    float _14, _24, _34, _44;
  };
  float e[16];
};

namespace Math
{
  mat3 Mat3Ident();
  mat4 Mat4Ident();
  mat3 MulMat3(mat3 A, mat3 B);
  mat3 Mat4ToMat3(mat4 Mat4);
  mat4 Mat3ToMat4(mat3 Mat3);
  mat4 MulMat4(mat4 A, mat4 B);
  vec3 MulMat3Vec3(mat3 Mat, vec3 Vec);
  vec4 MulMat4Vec4(mat4 Mat, vec4 Vec);
  void PrintMat3(mat3 Mat);
  void PrintMat4(mat4 Mat);
  mat4 Mat4Translate(float Tx, float Ty, float Tz);
  mat4 Mat4Translate(vec3 T);
  mat4 Mat4RotateY(float Angle);
  mat4 Mat4RotateZ(float Angle);
  mat4 Mat4RotateX(float Angle);
  mat4 Mat4Rotate(vec3 EulerAngles);
  mat4 Mat4Scale(float Sx, float Sy, float Sz);
  mat4 Mat4Scale(float S);
  mat4 Mat4Camera(vec3 P, vec3 Dir, vec3 Up);
  mat4 Mat4Perspective(float ViewAngle, float AspectRatio, float FrontPlaneDist,
                       float BackPlaneDist);
  mat3 Transpose3(const mat3* Mat);
  mat4 Transpose4(const mat4* Mat);
}
