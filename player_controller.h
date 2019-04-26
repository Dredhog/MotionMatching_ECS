#pragma once

#include "entity.h"
#include "common.h"
#include "camera.h"
#include "motion_matching.h"
#include "resource_manager.h"
#include "blend_stack.h"

namespace Gameplay
{
  void ResetPlayer(entity* Player, blend_stack* BlendStack, Resource::resource_manager* Resources,
                   const mm_controller_data* MMData);
  void UpdatePlayer(entity* Player, blend_stack* BlendStack, Memory::stack_allocator* TempAlocator,
                    const game_input* Input, const camera* Camera, const mm_controller_data* MMData,
                    const mm_debug_settings* MMDebug, float Speed);
}
