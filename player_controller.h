#pragma once

#include "entity.h"
#include "common.h"
#include "camera.h"

namespace Gameplay
{
  void ResetPlayer();
  void UpdatePlayer(entity* Player, const game_input* Input, const camera* Camera);
}
