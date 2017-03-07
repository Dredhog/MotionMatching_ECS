#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "matrix.h"
#include "vector.h"

namespace Math
{
  inline mat3
  MulMat3(mat3 A, mat3 B)
  {
    mat3 Result;
    int  Row    = 0;
    int  Column = 0;

    for(int i = 0; i < 9; i++)
    {
      Column = (i / 3) * 3;
      Row    = i % 3;

      Result.e[i] =
        A.e[Row] * B.e[Column] + A.e[Row + 3] * B.e[Column + 1] + A.e[Row + 6] * B.e[Column + 2];
    }
    return Result;
  }

  inline mat4
  MulMat4(mat4 A, mat4 B)
  {
    mat4 Result;
    int  Row    = 0;
    int  Column = 0;

    for(int i = 0; i < 16; i++)
    {
      Column = (i / 4) * 4;
      Row    = i % 4;

      Result.e[i] = A.e[Row] * B.e[Column] + A.e[Row + 4] * B.e[Column + 1] +
                    A.e[Row + 8] * B.e[Column + 2] + A.e[Row + 12] * B.e[Column + 3];
    }
    return Result;
  }

  vec3
  MulMat3Vec3(mat3 Mat, vec3 Vec)
  {
    vec3 Result;
    Result.X = Mat._11 * Vec.X + Mat._12 * Vec.Y + Mat._13 * Vec.Z;
    Result.X = Mat._21 * Vec.X + Mat._22 * Vec.Y + Mat._23 * Vec.Z;
    Result.X = Mat._31 * Vec.X + Mat._32 * Vec.Y + Mat._33 * Vec.Z;
    return Result;
  }

  vec4
  MulMat4Vec4(mat4 Mat, vec4 Vec)
  {
    vec4 Result;
    Result.X = Mat._11 * Vec.X + Mat._12 * Vec.Y + Mat._13 * Vec.Z + Mat._14 * Vec.W;
    Result.X = Mat._21 * Vec.X + Mat._22 * Vec.Y + Mat._23 * Vec.Z + Mat._24 * Vec.W;
    Result.X = Mat._31 * Vec.X + Mat._32 * Vec.Y + Mat._33 * Vec.Z + Mat._34 * Vec.W;
    Result.X = Mat._41 * Vec.X + Mat._42 * Vec.Y + Mat._43 * Vec.Z + Mat._44 * Vec.W;
    return Result;
  }

  void
  PrintMat3(mat3 Mat)
  {
    for(int j = 0; j < 3; j++)
    {
      for(int i = 0; i < 3; i++)
      {
        printf("%3.2f ", (double)Mat.e[i * 3 + j]);
      }
      printf("\n");
    }
  }

  void
  PrintMat4(mat4 Mat)
  {
    for(int j = 0; j < 4; j++)
    {
      for(int i = 0; i < 4; i++)
      {
        printf("%3.2f ", (double)Mat.e[i * 4 + j]);
      }
      printf("\n");
    }
  }

  mat3
  Mat3Ident()
  {
    mat3 Result = {};
    Result._11  = 1;
    Result._22  = 1;
    Result._33  = 1;
    return Result;
  }

  mat4
  Mat4Ident()
  {
    mat4 Result = {};
    Result._11  = 1;
    Result._22  = 1;
    Result._33  = 1;
    Result._44  = 1;
    return Result;
  }

  mat4
  Mat4Translate(float Tx, float Ty, float Tz)
  {
    return mat4{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, Tx, Ty, Tz, 1 };
  }

  mat4
  Mat4Translate(vec3 T)
  {
    return Mat4Translate(T.X, T.Y, T.Z);
  }

  mat4
  Mat4RotateY(float Angle)
  {
    Angle     = (Angle * 3.14159f) / 180.0f;
    float Sin = sinf(Angle);
    float Cos = cosf(Angle);

    return mat4{ Cos, 0, -Sin, 0, 0, 1, 0, 0, Sin, 0, Cos, 0, 0, 0, 0, 1 };
  }
  mat4
  Mat4RotateZ(float Angle)
  {
    Angle     = (Angle * 3.14159f) / 180.0f;
    float Sin = sinf(Angle);
    float Cos = cosf(Angle);

    return mat4{ Cos, Sin, 0, 0, -Sin, Cos, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
  }
  mat4
  Mat4RotateX(float Angle)
  {
    Angle     = (Angle * 3.14159f) / 180.0f;
    float Sin = sinf(Angle);
    float Cos = cosf(Angle);

    return mat4{ 1, 0, 0, 0, 0, Cos, Sin, 0, 0, -Sin, Cos, 0, 0, 0, 0, 1 };
  }

  mat4
  Mat4Scale(float Sx, float Sy, float Sz)
  {
    return mat4{ Sx, 0, 0, 0, 0, Sy, 0, 0, 0, 0, Sz, 0, 0, 0, 0, 1 };
  }

  mat4
  Mat4Scale(float S)
  {
    return mat4{ S, 0, 0, 0, 0, S, 0, 0, 0, 0, S, 0, 0, 0, 0, 1 };
  }

  mat4
  Mat4Camera(vec3 P, vec3 Dir, vec3 YOrUp)
  {
    Dir        = Normalized(Dir);
    vec3 Right = Normalized(Cross(Dir, YOrUp));
    vec3 Up    = Cross(Right, Dir);

    mat4 OrientCamera = { Right.X, Up.X, -Dir.X, 0, Right.Y, Up.Y, -Dir.Y, 0,
                          Right.Z, Up.Z, -Dir.Z, 0, 0,       0,    0,      1 };
    return MulMat4(OrientCamera, Mat4Translate(-P));
  }

  mat4
  Mat4Perspective(float ViewAngle, float AspectRatio, float Near, float Far)
  {
    assert(Near > 0.0f && Far > 0.0f);
    assert(ViewAngle > 0.0f && AspectRatio > 0.0f);

    float NearOverRight = 1.0f / tanf((3.14159f / 180.0f) * (ViewAngle * 0.5f));
    float NearOverTop   = NearOverRight * AspectRatio;

    mat4 Result = {};
    Result._11  = NearOverRight;
    Result._22  = NearOverTop;
    Result._33  = -(Far + Near) / (Far - Near);
    Result._34  = (-2.0f * Far * Near) / (Far - Near);
    Result._43  = -1.0f;
    return Result;
  }

  mat3
  Transpose3(const mat3* Mat)
  {
    mat3 Transpose;
    for(int i = 0; i < 3; i++)
    {
      for(int j = 0; j < 3; j++)
      {
        Transpose.e[3 * i + j] = Mat->e[3 * j + i];
      }
    }
    return Transpose;
  }

  mat4
  Transpose4(const mat4* Mat)
  {
    mat4 Transpose;
    for(int i = 0; i < 4; i++)
    {
      for(int j = 0; j < 4; j++)
      {
        Transpose.e[4 * i + j] = Mat->e[4 * j + i];
      }
    }
    return Transpose;
  }
}
