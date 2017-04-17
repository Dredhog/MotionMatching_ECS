#include "camera.h"

void
UpdateCamera(camera* Camera, const game_input* Input)
{
  if(Input->LeftShift.EndedDown)
  {
    if(Input->w.EndedDown)
    {
      Camera->Position += Input->dt * Camera->Speed * Camera->Up;
    }
    if(Input->s.EndedDown)
    {
      Camera->Position -= Input->dt * Camera->Speed * Camera->Up;
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

  Camera->Rotation.X = ClampFloat(-Camera->MaxTiltAngle, Camera->Rotation.X, Camera->MaxTiltAngle);
  Camera->Forward =
    Math::MulMat3Vec3(Math::Mat4ToMat3(Math::Mat4Rotate(Camera->Rotation)), { 0, 0, -1 });
  Camera->Right   = Math::Cross(Camera->Forward, Camera->Up);
  Camera->Forward = Math::Normalized(Camera->Forward);
  Camera->Right   = Math::Normalized(Camera->Right);
  Camera->Up      = Math::Normalized(Camera->Up);

  Camera->ViewMatrix = Math::Mat4Camera(Camera->Position, Camera->Forward, Camera->Up);
  Camera->ProjectionMatrix =
    Math::Mat4Perspective(Camera->FieldOfView, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
                          Camera->NearClipPlane, Camera->FarClipPlane);
  Camera->VPMatrix = Math::MulMat4(Camera->ProjectionMatrix, Camera->ViewMatrix);
}
