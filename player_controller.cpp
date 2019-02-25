#include "player_controller.h"
#include "math.h"

float g_SpeedBlend            = 0;
float g_VerticalBlend         = 0;
float g_HeadBlend             = 0;
float g_PlayerHeadMotionAngle = 30;
float g_MaxSpeed              = 10;
float g_Acceleration          = 30;
float g_Decceleration         = 15;
float g_MovePlaybackRate      = 1.5f;
float g_MaxTiltAngle          = 15;

const vec3 Forward = { 0, 0, -1 };
const vec3 Right   = { 1, 0, 0 };

const vec3 YAxis = { 0, 1, 0 };
const vec3 ZAxis = { 0, 0, 1 };

struct player
{
  vec3  P;
  vec3  dP;
  vec3  Dir;
  vec3  AccDir;
  vec3  Up;
  vec3  TiltAxis;
  bool  InAir;
  float Angle;
  float DestAngle;
  float TiltAngle;
} g_Player;

void
ThirdPersonAnimationBlendFunction(Anim::animation_controller* C)
{
  Anim::SetPlaybackRate(C, 0, g_MovePlaybackRate);
  Anim::SetPlaybackRate(C, 1, g_MovePlaybackRate);
  Anim::SampleAtGlobalTime(C, 0, 0);           // Sample Walk
  Anim::SampleAtGlobalTime(C, 1, 1);           // Sample Run
  Anim::LinearBlend(C, 0, 1, g_SpeedBlend, 1); // LERP(Walk, Run) => move
  Anim::SampleAtGlobalTime(C, 2, 0);           // Sample Idle
  Anim::LinearBlend(C, 0, 1, g_SpeedBlend, 0); // LERP(move, Idle) => ground
  // Anim::LinearAnimationSample(C, 4, g_PlayerVertical, 1); // Sample Jump
  // Anim::LinearBlend(C, 0, 1, g_PlayerVertical, 0);        // LERP(ground, Jump) => body
  // Anim::LinearAnimationSample(C, 2, g_PlyerHeadAngle, 1); // Sample head Turn
  // Anim::AdditiveBlend(C, 0, 1, 1.0, 0);                   // AdditiveBlend(body, head) => final
}

void
Gameplay::ResetPlayer()
{
  g_Player     = {};
  g_Player.Dir = -Forward;
  g_Player.Up  = YAxis;
}

void
Gameplay::UpdatePlayer(entity* Player, const game_input* Input)
{
  float InitialJumpSpeed = 5;
  float AngularVelocity  = 720;

  // Assign blend func for assurance
  if(Player->AnimController != NULL)
  {
    Player->AnimController->BlendFunc = ThirdPersonAnimationBlendFunction;
  }

  // PLAYER MOTION
  g_Player.AccDir = {};
  if(!Input->IsMouseInEditorMode)
  {
    if(Input->ArrowUp.EndedDown)
    {
      g_Player.AccDir = g_Player.AccDir + Forward;
    }
    if(Input->ArrowDown.EndedDown)
    {
      g_Player.AccDir = g_Player.AccDir - Forward;
    }
    if(Input->ArrowRight.EndedDown)
    {
      g_Player.AccDir = g_Player.AccDir + Right;
    }
    if(Input->ArrowLeft.EndedDown)
    {
      g_Player.AccDir = g_Player.AccDir - Right;
    }
  }

  float rtd         = 180 / 3.1415926535f;
  vec3  VelocityDir = Math::Normalized(g_Player.dP);
  if(Math::Length(g_Player.AccDir) != 0)
  {
    g_Player.AccDir = Math::Normalized(g_Player.AccDir);
    g_Player.Up     = Math::Normalized(g_Player.Up);
    vec3 Temp       = g_Player.Up + (g_Player.AccDir * 3 * Input->dt);
    if(rtd * acosf(Math::Dot(Math::Normalized(Temp), YAxis)) < g_MaxTiltAngle)
    {
      g_Player.Up = Temp / g_Player.Up.Y;
    }
    g_Player.dP += g_Player.AccDir * g_Acceleration * Input->dt;
    g_Player.Dir += g_Player.Dir + (g_Player.AccDir * g_Acceleration * Input->dt * 2);
  }
  else if(Math::Length(g_Player.dP) >= g_Decceleration * Input->dt)
  {
    g_Player.dP -= VelocityDir * Input->dt * g_Decceleration;
  }
  else
  {
    g_Player.dP = {};
  }

  // Determine the player's tilt
  vec3 TiltAxis = Math::Cross(YAxis, Math::Normalized(g_Player.Up));
  if(Math::Length(TiltAxis) != 0)
  {
    g_Player.TiltAngle = rtd * asinf(Math::Length(TiltAxis));
    g_Player.TiltAxis  = Math::Normalized(g_Player.TiltAxis);
  }

  // Limit the player's velocity float
  float Speed = Math::Length(g_Player.dP);
  if(Speed > 0.001f)
  {
    g_Player.Dir = g_Player.dP / Speed;
    if(g_MaxSpeed < Speed)
    {
      g_Player.dP = g_Player.Dir * g_MaxSpeed;
    }
  }

  // Determine rotation around y axis
  g_Player.DestAngle = rtd * acosf(Math::Dot(g_Player.Dir, ZAxis));
  if(g_Player.Dir.X < 0)
  {
    g_Player.DestAngle = (-1) * g_Player.DestAngle + 360;
  }
  if(g_Player.Angle < 0)
  {
    g_Player.Angle += 360;
  }
  else if(g_Player.Angle > 360)
  {
    g_Player.Angle -= 360;
  }
  float Delta = g_Player.DestAngle - g_Player.Angle;
  if(Delta > 0)
  {
    if(Delta <= 180)
    {
      if(g_Player.Angle + AngularVelocity * Input->dt < g_Player.DestAngle)
      {
        g_Player.Angle += AngularVelocity * Input->dt;
      }
      else
      {
        g_Player.Angle = g_Player.DestAngle;
      }
    }
    else if(Delta < 360)
    {
      g_Player.Angle -= AngularVelocity * Input->dt;
    }
  }
  else
  {
    if(-180 <= Delta)
    {
      if(g_Player.Angle - AngularVelocity * Input->dt > g_Player.DestAngle)
      {
        g_Player.Angle -= AngularVelocity * Input->dt;
      }
      else
      {
        g_Player.Angle = g_Player.DestAngle;
      }
    }
    else if(-360 < Delta)
    {
      g_Player.Angle += AngularVelocity * Input->dt;
    }
  }

  if(g_MaxSpeed > 0.001f)
  {
    g_SpeedBlend = Math::Length(g_Player.dP) / g_MaxSpeed;
  }
  else
  {
    g_SpeedBlend = 0;
  }
  Player->Transform.Translation += g_Player.dP * Input->dt;
	vec3 EulerRotation = {g_Player.TiltAngle, g_Player.Angle, 0};
  Player->Transform.Rotation = Math::EulerToQuat(EulerRotation);
}
