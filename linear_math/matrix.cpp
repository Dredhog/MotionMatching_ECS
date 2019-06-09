#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "matrix.h"
#include "vector.h"

namespace Math
{
  mat3
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
  mat3
  MulMat3(mat3 A, float S)
  {
    mat3 Result = A;
    for(int i = 0; i < 9; i++)
    {
      A.e[i] *= S;
    }
    return Result;
  }

  mat3
  Mat4ToMat3(mat4 Mat4)
  {
    mat3 Result;

    for(int i = 0; i < 3; i++)
    {
      for(int j = 0; j < 3; j++)
      {
        Result.e[i * 3 + j] = Mat4.e[i * 4 + j];
      }
    }
    return Result;
  }

  mat4
  Mat3ToMat4(mat3 Mat3)
  {
    mat4 Result = {};

    for(int i = 0; i < 3; i++)
    {
      for(int j = 0; j < 3; j++)
      {
        Result.e[i * 4 + j] = Mat3.e[i * 3 + j];
      }
    }
    Result.e[15] = 1;
    return Result;
  }

  mat4
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
    Result.Y = Mat._21 * Vec.X + Mat._22 * Vec.Y + Mat._23 * Vec.Z;
    Result.Z = Mat._31 * Vec.X + Mat._32 * Vec.Y + Mat._33 * Vec.Z;
    return Result;
  }

  vec4
  MulMat4Vec4(mat4 Mat, vec4 Vec)
  {
    vec4 Result;
    Result.X = Mat._11 * Vec.X + Mat._12 * Vec.Y + Mat._13 * Vec.Z + Mat._14 * Vec.W;
    Result.Y = Mat._21 * Vec.X + Mat._22 * Vec.Y + Mat._23 * Vec.Z + Mat._24 * Vec.W;
    Result.Z = Mat._31 * Vec.X + Mat._32 * Vec.Y + Mat._33 * Vec.Z + Mat._34 * Vec.W;
    Result.W = Mat._41 * Vec.X + Mat._42 * Vec.Y + Mat._43 * Vec.Z + Mat._44 * Vec.W;
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

  mat3
  Mat3Basis(vec3 Right, vec3 Up, vec3 Forward)
  {
    mat3 Result = { Right.X, Right.Y, Right.Z, Up.X, Up.Y, Up.Z, Forward.X, Forward.Y, Forward.Z };
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

  mat3
  Mat3RotateY(float Angle)
  {
    Angle     = (Angle * 3.14159f) / 180.0f;
    float Sin = sinf(Angle);
    float Cos = cosf(Angle);

    return mat3{ Cos, 0, -Sin, 0, 1, 0, Sin, 0, Cos };
  }
  mat3
  Mat3RotateZ(float Angle)
  {
    Angle     = (Angle * 3.14159f) / 180.0f;
    float Sin = sinf(Angle);
    float Cos = cosf(Angle);

    return mat3{ Cos, Sin, 0, -Sin, Cos, 0, 0, 0, 1 };
  }

  mat3
  Mat3RotateX(float Angle)
  {
    Angle     = (Angle * 3.14159f) / 180.0f;
    float Sin = sinf(Angle);
    float Cos = cosf(Angle);

    return mat3{ 1, 0, 0, 0, Cos, Sin, 0, -Sin, Cos };
  }

  mat3
  Mat3Rotate(float X, float Y, float Z)
  {
    return MulMat3(Mat3RotateZ(Z), MulMat3(Mat3RotateY(Y), Mat3RotateX(X)));
  }

  mat3
  Mat3Rotate(vec3 EulerAngles)
  {
    return Mat3Rotate(EulerAngles.X, EulerAngles.Y, EulerAngles.Z);
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
  Mat4Rotate(vec3 EulerAngles)
  {
    return MulMat4(Mat4RotateZ(EulerAngles.Z),
                   MulMat4(Mat4RotateY(EulerAngles.Y), Mat4RotateX(EulerAngles.X)));
  }

  mat4
  Mat4RotateAxisAngle(vec3 RotationAxis, float Angle)
  {

    float x = RotationAxis.X;
    float y = RotationAxis.Y;
    float z = RotationAxis.Z;

    float DTR = 3.1415926f / 180;
    float s   = sinf(Angle * DTR);
    float c   = cosf(Angle * DTR);
    float k   = 1 - c;

    return mat4{ x * x * k + c,
                 x * y * k + z * s,
                 x * z * k - y * s,
                 0,
                 x * y * k - z * s,
                 y * y * k + c,
                 y * z * k + x * s,
                 0,
                 x * z * k + y * s,
                 y * z * k - x * s,
                 z * z * k + c,
                 0,
                 0,
                 0,
                 0,
                 1 };
  }
  mat3
  Mat3Scale(float Sx, float Sy, float Sz)
  {
    return mat3{ Sx, 0, 0, 0, Sy, 0, 0, 0, Sz };
  }

  mat3
  Mat3Scale(vec3 S)
  {
    return mat3{ S.X, 0, 0, 0, S.Y, 0, 0, 0, S.Z };
  }

  mat3
  Mat3Scale(float S)
  {
    return mat3{ S, 0, 0, 0, S, 0, 0, 0, S };
  }

  mat4
  Mat4Scale(float Sx, float Sy, float Sz)
  {
    return mat4{ Sx, 0, 0, 0, 0, Sy, 0, 0, 0, 0, Sz, 0, 0, 0, 0, 1 };
  }

  mat4
  Mat4Scale(vec3 S)
  {
    return mat4{ S.X, 0, 0, 0, 0, S.Y, 0, 0, 0, 0, S.Z, 0, 0, 0, 0, 1 };
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
    float NearOverTop   = NearOverRight;

    mat4 Result = {};
    Result._11  = NearOverRight;
    Result._22  = NearOverTop * AspectRatio;
    Result._33  = -(Far + Near) / (Far - Near);
    Result._34  = (-2.0f * Far * Near) / (Far - Near);
    Result._43  = -1.0f;
    return Result;
  }

  mat4
  Mat4Orthogonal(float Left, float Right, float Bottom, float Top, float Near, float Far)
  {
    assert(Far - Near > 0.0f);
    assert(Right > Left);
    assert(Top > Bottom);

    mat4 Result = {};
    Result._11  = 2.0f / (Right - Left);
    Result._22  = 2.0f / (Top - Bottom);
    Result._33  = -2.0f / (Far - Near);
    Result._44  = 1.0f;
    Result._14  = -(Right + Left) / (Right - Left);
    Result._24  = -(Top + Bottom) / (Top - Bottom);
    Result._34  = -(Far + Near) / (Far - Near);
    return Result;
  }

  mat3
  Transposed3(const mat3& Mat)
  {
    mat3 Transpose;
    for(int i = 0; i < 3; i++)
    {
      for(int j = 0; j < 3; j++)
      {
        Transpose.e[3 * i + j] = Mat.e[3 * j + i];
      }
    }
    return Transpose;
  }

  mat4
  Transposed4(const mat4& Mat)
  {
    mat4 Transpose;
    for(int i = 0; i < 4; i++)
    {
      for(int j = 0; j < 4; j++)
      {
        Transpose.e[4 * i + j] = Mat.e[4 * j + i];
      }
    }
    return Transpose;
  }

  void
  Transpose3(mat3* Mat)
  {
    for(int i = 0; i < 3; i++)
    {
      for(int j = 0; j < 3; j++)
      {
        Mat->e[3 * i + j] = Mat->e[3 * j + i];
      }
    }
  }

  void
  Transpose4(mat4* Mat)
  {
    for(int i = 0; i < 4; i++)
    {
      for(int j = 0; j < 4; j++)
      {
        Mat->e[4 * i + j] = Mat->e[4 * j + i];
      }
    }
  }

#if 0
  static void
  NormalizeMat3(mat3* Mat3)
  {
    vec3 X       = { Mat3->_11, Mat3->_21, Mat3->_31 };
    vec3 Y       = { Mat3->_12, Mat3->_22, Mat3->_32 };
    vec3 Z       = { Mat3->_13, Mat3->_23, Mat3->_33 };
    vec3 Lengths = { Length(X), Length(Y), Length(Z) };

    Mat3->_11 /= Lengths.X;
    Mat3->_21 /= Lengths.X;
    Mat3->_31 /= Lengths.X;

    Mat3->_12 /= Lengths.Y;
    Mat3->_22 /= Lengths.Y;
    Mat3->_32 /= Lengths.Y;

    Mat3->_13 /= Lengths.Z;
    Mat3->_23 /= Lengths.Z;
    Mat3->_33 /= Lengths.Z;
  }

  static void
  ScaleMat3ByVec3(mat3* Mat3, vec3 Scale)
  {
    Mat3->_11 *= Scale.X;
    Mat3->_21 *= Scale.X;
    Mat3->_31 *= Scale.X;

    Mat3->_12 *= Scale.Y;
    Mat3->_22 *= Scale.Y;
    Mat3->_32 *= Scale.Y;

    Mat3->_13 *= Scale.Z;
    Mat3->_23 *= Scale.Z;
    Mat3->_33 *= Scale.Z;
  }
#endif
  vec3
  GetMat4Translation(mat4 Mat4)
  {
    return { Mat4._14, Mat4._24, Mat4._34 };
  }
	
	vec3
	Mat4GetScaleAndNormalize(mat4* Mat4)
	{
    vec3 Scale = { Length({ Mat4->_11, Mat4->_21, Mat4->_31 }),
                   Length({ Mat4->_12, Mat4->_22, Mat4->_32 }),
                   Length({ Mat4->_13, Mat4->_23, Mat4->_33 }) };
    Mat4->_11 /= Scale.X;
    Mat4->_21 /= Scale.X;
    Mat4->_31 /= Scale.X;

    Mat4->_12 /= Scale.Y;
    Mat4->_22 /= Scale.Y;
    Mat4->_32 /= Scale.Y;

    Mat4->_13 /= Scale.Z;
    Mat4->_23 /= Scale.Z;
    Mat4->_33 /= Scale.Z;

    return Scale;
  }

	vec3
	Mat3GetScaleAndNormalize(mat3* Mat3)
	{
    vec3 Scale = { Length({ Mat3->_11, Mat3->_21, Mat3->_31 }),
                   Length({ Mat3->_12, Mat3->_22, Mat3->_32 }),
                   Length({ Mat3->_13, Mat3->_23, Mat3->_33 }) };
    Mat3->_11 /= Scale.X;
    Mat3->_21 /= Scale.X;
    Mat3->_31 /= Scale.X;

    Mat3->_12 /= Scale.Y;
    Mat3->_22 /= Scale.Y;
    Mat3->_32 /= Scale.Y;

    Mat3->_13 /= Scale.Z;
    Mat3->_23 /= Scale.Z;
    Mat3->_33 /= Scale.Z;

    return Scale;
  }

  float
  Determinant(const mat4& M)
  {
    return M.e[12] * M.e[9] * M.e[6] * M.e[3] - M.e[8] * M.e[13] * M.e[6] * M.e[3] -
           M.e[12] * M.e[5] * M.e[10] * M.e[3] + M.e[4] * M.e[13] * M.e[10] * M.e[3] +
           M.e[8] * M.e[5] * M.e[14] * M.e[3] - M.e[4] * M.e[9] * M.e[14] * M.e[3] -
           M.e[12] * M.e[9] * M.e[2] * M.e[7] + M.e[8] * M.e[13] * M.e[2] * M.e[7] +
           M.e[12] * M.e[1] * M.e[10] * M.e[7] - M.e[0] * M.e[13] * M.e[10] * M.e[7] -
           M.e[8] * M.e[1] * M.e[14] * M.e[7] + M.e[0] * M.e[9] * M.e[14] * M.e[7] +
           M.e[12] * M.e[5] * M.e[2] * M.e[11] - M.e[4] * M.e[13] * M.e[2] * M.e[11] -
           M.e[12] * M.e[1] * M.e[6] * M.e[11] + M.e[0] * M.e[13] * M.e[6] * M.e[11] +
           M.e[4] * M.e[1] * M.e[14] * M.e[11] - M.e[0] * M.e[5] * M.e[14] * M.e[11] -
           M.e[8] * M.e[5] * M.e[2] * M.e[15] + M.e[4] * M.e[9] * M.e[2] * M.e[15] +
           M.e[8] * M.e[1] * M.e[6] * M.e[15] - M.e[0] * M.e[9] * M.e[6] * M.e[15] -
           M.e[4] * M.e[1] * M.e[10] * M.e[15] + M.e[0] * M.e[5] * M.e[10] * M.e[15];
  }

  mat4
  InvMat4(mat4 M)
  {
    float Det = Determinant(M);
    /* there is no inverse if determinant is zero (not likely unless scale is
    broken) */
    if(0.0f == Det)
    {
      printf("error: inverse not computable - determinant is zero.\n");
      return M;
    }
    float InvDet = 1.0f / Det;

    return mat4{ InvDet * (M.e[9] * M.e[14] * M.e[7] - M.e[13] * M.e[10] * M.e[7] +
                           M.e[13] * M.e[6] * M.e[11] - M.e[5] * M.e[14] * M.e[11] -
                           M.e[9] * M.e[6] * M.e[15] + M.e[5] * M.e[10] * M.e[15]),
                 InvDet * (M.e[13] * M.e[10] * M.e[3] - M.e[9] * M.e[14] * M.e[3] -
                           M.e[13] * M.e[2] * M.e[11] + M.e[1] * M.e[14] * M.e[11] +
                           M.e[9] * M.e[2] * M.e[15] - M.e[1] * M.e[10] * M.e[15]),
                 InvDet * (M.e[5] * M.e[14] * M.e[3] - M.e[13] * M.e[6] * M.e[3] +
                           M.e[13] * M.e[2] * M.e[7] - M.e[1] * M.e[14] * M.e[7] -
                           M.e[5] * M.e[2] * M.e[15] + M.e[1] * M.e[6] * M.e[15]),
                 InvDet * (M.e[9] * M.e[6] * M.e[3] - M.e[5] * M.e[10] * M.e[3] -
                           M.e[9] * M.e[2] * M.e[7] + M.e[1] * M.e[10] * M.e[7] +
                           M.e[5] * M.e[2] * M.e[11] - M.e[1] * M.e[6] * M.e[11]),
                 InvDet * (M.e[12] * M.e[10] * M.e[7] - M.e[8] * M.e[14] * M.e[7] -
                           M.e[12] * M.e[6] * M.e[11] + M.e[4] * M.e[14] * M.e[11] +
                           M.e[8] * M.e[6] * M.e[15] - M.e[4] * M.e[10] * M.e[15]),
                 InvDet * (M.e[8] * M.e[14] * M.e[3] - M.e[12] * M.e[10] * M.e[3] +
                           M.e[12] * M.e[2] * M.e[11] - M.e[0] * M.e[14] * M.e[11] -
                           M.e[8] * M.e[2] * M.e[15] + M.e[0] * M.e[10] * M.e[15]),
                 InvDet * (M.e[12] * M.e[6] * M.e[3] - M.e[4] * M.e[14] * M.e[3] -
                           M.e[12] * M.e[2] * M.e[7] + M.e[0] * M.e[14] * M.e[7] +
                           M.e[4] * M.e[2] * M.e[15] - M.e[0] * M.e[6] * M.e[15]),
                 InvDet * (M.e[4] * M.e[10] * M.e[3] - M.e[8] * M.e[6] * M.e[3] +
                           M.e[8] * M.e[2] * M.e[7] - M.e[0] * M.e[10] * M.e[7] -
                           M.e[4] * M.e[2] * M.e[11] + M.e[0] * M.e[6] * M.e[11]),
                 InvDet * (M.e[8] * M.e[13] * M.e[7] - M.e[12] * M.e[9] * M.e[7] +
                           M.e[12] * M.e[5] * M.e[11] - M.e[4] * M.e[13] * M.e[11] -
                           M.e[8] * M.e[5] * M.e[15] + M.e[4] * M.e[9] * M.e[15]),
                 InvDet * (M.e[12] * M.e[9] * M.e[3] - M.e[8] * M.e[13] * M.e[3] -
                           M.e[12] * M.e[1] * M.e[11] + M.e[0] * M.e[13] * M.e[11] +
                           M.e[8] * M.e[1] * M.e[15] - M.e[0] * M.e[9] * M.e[15]),
                 InvDet * (M.e[4] * M.e[13] * M.e[3] - M.e[12] * M.e[5] * M.e[3] +
                           M.e[12] * M.e[1] * M.e[7] - M.e[0] * M.e[13] * M.e[7] -
                           M.e[4] * M.e[1] * M.e[15] + M.e[0] * M.e[5] * M.e[15]),
                 InvDet * (M.e[8] * M.e[5] * M.e[3] - M.e[4] * M.e[9] * M.e[3] -
                           M.e[8] * M.e[1] * M.e[7] + M.e[0] * M.e[9] * M.e[7] +
                           M.e[4] * M.e[1] * M.e[11] - M.e[0] * M.e[5] * M.e[11]),
                 InvDet * (M.e[12] * M.e[9] * M.e[6] - M.e[8] * M.e[13] * M.e[6] -
                           M.e[12] * M.e[5] * M.e[10] + M.e[4] * M.e[13] * M.e[10] +
                           M.e[8] * M.e[5] * M.e[14] - M.e[4] * M.e[9] * M.e[14]),
                 InvDet * (M.e[8] * M.e[13] * M.e[2] - M.e[12] * M.e[9] * M.e[2] +
                           M.e[12] * M.e[1] * M.e[10] - M.e[0] * M.e[13] * M.e[10] -
                           M.e[8] * M.e[1] * M.e[14] + M.e[0] * M.e[9] * M.e[14]),
                 InvDet * (M.e[12] * M.e[5] * M.e[2] - M.e[4] * M.e[13] * M.e[2] -
                           M.e[12] * M.e[1] * M.e[6] + M.e[0] * M.e[13] * M.e[6] +
                           M.e[4] * M.e[1] * M.e[14] - M.e[0] * M.e[5] * M.e[14]),
                 InvDet * (M.e[4] * M.e[9] * M.e[2] - M.e[8] * M.e[5] * M.e[2] +
                           M.e[8] * M.e[1] * M.e[6] - M.e[0] * M.e[9] * M.e[6] -
                           M.e[4] * M.e[1] * M.e[10] + M.e[0] * M.e[5] * M.e[10]) };
  }
}
