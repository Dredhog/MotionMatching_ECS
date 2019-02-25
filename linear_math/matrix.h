#pragma once
#include "vector.h"

union mat3 {
  struct
  {
    float _11, _21, _31;
    float _12, _22, _32;
    float _13, _23, _33;
  };
  struct{
    vec3 X;
    vec3 Y;
    vec3 Z;
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
  struct
  {
    vec4 X_h;
    vec4 Y_h;
    vec4 Z_h;
    vec4 T_h;
  };
  struct{
    vec3 X; float PadX;
    vec3 Y; float padY;
    vec3 Z; float PadZ;
    vec3 T; float PadT;
  };
  
  float e[16];
};

namespace Math
{
  mat3 Mat3Ident();
  mat4 Mat4Ident();
  mat3 Mat3Basis(vec3 Right, vec3 Up, vec3 Forward);
  mat3 MulMat3(mat3 A, mat3 B);
  mat3 Mat4ToMat3(mat4 Mat4);
  mat4 Mat3ToMat4(mat3 Mat3);
  void PrintMat4(mat4 Mat);
  mat4 MulMat4(mat4 A, mat4 B);
  vec3 MulMat3Vec3(mat3 Mat, vec3 Vec);
  vec4 MulMat4Vec4(mat4 Mat, vec4 Vec);
  mat4 Mat4Translate(float Tx, float Ty, float Tz);
  mat4 Mat4Translate(vec3 T);
  vec3 GetTranslationVec3(mat4 Mat4);
	vec3 Mat4GetScaleAndNormalize(mat4* Mat4);
  vec3 Mat3GetScaleAndNormalize(mat3* Mat3);
  mat3 Mat3RotateY(float Angle);
  mat3 Mat3RotateZ(float Angle);
  mat4 Mat4RotateY(float Angle);
  mat4 Mat4RotateZ(float Angle);
  mat4 Mat4RotateX(float Angle);
  mat4 Mat4Rotate(vec3 EulerAngles);
  mat4 Mat4RotateAxisAngle(vec3 RotationAxis, float Angle);
  mat3 Mat3Scale(float Sx, float Sy, float Sz);
  mat3 Mat3Scale(vec3 S);
  mat3 Mat3Scale(float S);
  mat4 Mat4Scale(float Sx, float Sy, float Sz);
  mat4 Mat4Scale(vec3 S);
  mat4 Mat4Scale(float S);
  mat4 Mat4Camera(vec3 P, vec3 Dir, vec3 Up);
  mat4 Mat4Perspective(float ViewAngle, float AspectRatio, float FrontPlaneDist,
                       float BackPlaneDist);
  mat4 Mat4Orthogonal(float Left, float Right, float Bottom, float Top, float Near, float Far);
  mat3 Transposed3(const mat3& Mat);
  mat4 Transposed4(const mat4& Mat);
  void Transpose3(mat3* Mat);
  void Transpose4(mat4* Mat);
  vec3 GetMat4Translation(mat4 Mat4);
  mat4 InvMat4(mat4 Mat4);
}
