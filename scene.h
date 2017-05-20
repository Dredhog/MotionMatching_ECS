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

  int32_t ModelCount;
  rid*    ModelID;
  path    ModelPaths;

  int32_t AnimationCount;
  rid*    AnimationID;
  path    AnimationPaths;

  int32_t MaterialCount;
  rid*    MaterialID;
  path    MaterialPaths;

  int32_t TextureCount;
  rid*    TextureID;
  path    TexturePaths;
};

void ExportScene(game_state* GameState, const char* path);
void ImportScene(game_state* GameState, const char* Path);
