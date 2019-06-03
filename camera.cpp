#include "camera.h"
#define _USE_MATH_DEFINES
#include "math.h"

const vec3 YAxis = { 0, 1, 0 };

void
UpdateCameraMatrices(camera* Camera)
{
  Camera->ViewMatrix = Math::Mat4Camera(Camera->Position, Camera->Forward, YAxis);
  Camera->ProjectionMatrix =
    Math::Mat4Perspective(Camera->FieldOfView, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
                          Camera->NearClipPlane, Camera->FarClipPlane);
  Camera->VPMatrix    = Math::MulMat4(Camera->ProjectionMatrix, Camera->ViewMatrix);
  Camera->InvVPMatrix = Math::InvMat4(Camera->VPMatrix);
}

void
UpdateCameraDerivedFields(camera* Camera)
{
  Camera->Rotation.X = ClampFloat(-Camera->MaxTiltAngle, Camera->Rotation.X, Camera->MaxTiltAngle);
  Camera->Forward    = Math::Normalized(
    Math::MulMat3Vec3(Math::Mat3Rotate(Camera->Rotation.X, Camera->Rotation.Y, 0), { 0, 0, -1 }));
  Camera->Right      = Math::Normalized(Math::Cross(Camera->Forward, YAxis));
  UpdateCameraMatrices(Camera);
}

void
UpdateCamera(camera* Camera, vec3 FollowPoint, const game_input* Input)
{
  const float DegToRad = float(M_PI) / 180.0f;
  if(!Input->IsMouseInEditorMode)
  {
    Camera->OrbitRotation.Y =
      ClampFloat(-10 * DegToRad, Camera->OrbitRotation.Y - 0.003f * (float)Input->dMouseY,
                 70 * DegToRad);
    Camera->OrbitRotation.X += 0.05f * (float)Input->dMouseX;
    Camera->OrbitRadius = MaxFloat(0, Camera->OrbitRadius + 0.1f * Input->dMouseWheelScreen);
  }
  vec3 Dir = Math::Normalized(
    vec3{ cosf(Camera->OrbitRotation.X * DegToRad), 0, sinf(Camera->OrbitRotation.X * DegToRad) });
  Dir.Y            = tan(Camera->OrbitRotation.Y);
	Dir = -Math::Normalized(Dir);
  Camera->Position = FollowPoint + -Dir * Camera->OrbitRadius;
  Camera->Forward  = Dir;
  Camera->Right    = Math::Normalized(Math::Cross(Camera->Forward, YAxis));

  UpdateCameraMatrices(Camera);
}

void
UpdateCamera(camera* Camera, const game_input* Input)
{
  if(Input->LeftShift.EndedDown)
  {
    if(Input->w.EndedDown)
    {
      Camera->Position += Input->dt * Camera->Speed * YAxis;
    }
    if(Input->s.EndedDown)
    {
      Camera->Position -= Input->dt * Camera->Speed * YAxis;
    }
  }
  else
  {
    if(Input->w.EndedDown)
    {
      Camera->Position += Input->dt * Camera->Speed * Camera->Forward;
    }
    if(Input->s.EndedDown)
    {
      Camera->Position -= Input->dt * Camera->Speed * Camera->Forward;
    }
  }
  if(Input->a.EndedDown)
  {
    Camera->Position -= Input->dt * Camera->Speed * Camera->Right;
  }
  if(Input->d.EndedDown)
  {
    Camera->Position += Input->dt * Camera->Speed * Camera->Right;
  }
  if(!Input->IsMouseInEditorMode)
  {
    Camera->Rotation.X += 0.05f * (float)Input->dMouseY;
    Camera->Rotation.Y -= 0.05f * (float)Input->dMouseX;
  }
  UpdateCameraDerivedFields(Camera);
}
