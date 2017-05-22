#pragma once

#include "stdint.h"
#include "rid.h"
#include "entity.h"
#include "anim.h"
#include "game.h"

void ExportScene(game_state* GameState, const char* path);
void ImportScene(game_state* GameState, const char* Path);
