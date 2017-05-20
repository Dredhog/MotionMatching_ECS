#pragma once

#include "stdint.h"
#include "rid.h"
#include "entity.h"
#include "anim.h"
#include "game.h"

struct scene
{
  int32_t EntityCount;
  entity* Entities;

  int32_t                     AnimControllerCount;
  Anim::animation_controller* AnimControllers;

  // Resource mappings
  int32_t ModelCount;
  rid*    ModelIDs;
  path*   ModelPaths;

  int32_t MaterialCount;
  rid*    MaterialIDs;
  path*   MaterialPaths;

  int32_t AnimationCount;
  rid*    AnimationIDs;
  path*   AnimationPaths;
};

void ExportScene(game_state* GameState, const char* path);
void ImportScene(game_state* GameState, const char* Path);
