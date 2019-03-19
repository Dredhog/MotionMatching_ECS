#pragma once

#include "entity.h"
#include "common.h"
#include "camera.h"
#include "motion_matching.h"
#include "resource_manager.h"

namespace Gameplay
{
  void ResetPlayer();
  void UpdatePlayer(entity* Player, Resource::resource_manager* Resources, const game_input* Input,
                    const camera* Camera, const mm_controller_data* MMSet, float Speed);
}
