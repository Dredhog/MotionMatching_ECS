#pragma once

#include "entity.h"
#include "anim.h"
#include "common.h"

extern float g_PlayerSpeed;
extern float g_PlayerVertical;
extern float g_PlyerHeadAngle;
extern float g_PlayerHeadMotionAngle;
extern float g_MaxSpeed;
extern float g_Acceleration;
extern float g_Decceleration;
extern float g_MovePlaybackRate;

namespace Gameplay
{
  void ResetPlayer();
  void UpdatePlayer(entity* Player, const game_input* Input);
}
