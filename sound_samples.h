#pragma once

#include <cassert>
#include "game.h"

#include "load_wav.h"

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
  game_state* GameState = (game_state*)GameMemory.PersistentMemory;
  if(!GameState->WAVLoaded)
  {
    GameState->WAVLoaded = true;
    Audio::LoadWAV(&GameState->AudioBuffer, "./data/Intruder_Alert.wav");
  }

  //Audio::GameOutputSound(SoundBuffer, &GameState->AudioBuffer);
}
