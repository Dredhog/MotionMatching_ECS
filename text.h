#pragma once

#include <SDL2/SDL_ttf.h>
#include "game.h"

namespace Text
{
  TTF_Font* OpenFont(const char* FontName, int FontSize);
  int32_t CacheTextTexture(game_state* GameState, const char* Text, vec4 Color);
}
