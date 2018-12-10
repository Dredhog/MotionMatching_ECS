#pragma once

#include <stdint.h>

#define ArrayCount(Array) sizeof((Array)) / sizeof(Array[0])

#define Kibibytes(Count) (1024 * (Count))
#define Mibibytes(Count) (1024 * Kibibytes(Count))
#define Gibibytes(Count) (1024 * Mibibytes(Count))

#define FRAME_TIME_MS 2
#define SLOW_MOTION_COEFFICIENT 0.2f
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

struct game_button_state
{
  uint8_t EndedDown;
  uint8_t Changed;
};

struct game_input
{
  int32_t MouseScreenX;
  int32_t MouseScreenY;
  int32_t dMouseScreenX;
  int32_t dMouseScreenY;

  int32_t MouseX;
  int32_t MouseY;
  int32_t dMouseX;
  int32_t dMouseY;
  float   dt;

  float NormMouseX;
  float NormMouseY;
  float NormdMouseX;
  float NormdMouseY;

  int32_t MouseWheelScreen;
  int32_t dMouseWheelScreen;

  bool IsMouseInEditorMode;
  bool ToggledEditorMode;

  union {
    game_button_state Buttons[32];
    struct
    {
      game_button_state a;
      game_button_state b;
      game_button_state c;
      game_button_state d;
      game_button_state e;
      game_button_state f;
      game_button_state g;
      game_button_state h;
      game_button_state i;
      game_button_state m;
      game_button_state n;
      game_button_state o;
      game_button_state p;
      game_button_state r;
      game_button_state s;
      game_button_state t;
      game_button_state v;
      game_button_state w;
      game_button_state x;
      game_button_state LeftCtrl;
      game_button_state LeftShift;
      game_button_state Space;
      game_button_state Tab;
      game_button_state Delete;
      game_button_state ArrowUp;
      game_button_state ArrowDown;
      game_button_state ArrowLeft;
      game_button_state ArrowRight;
      game_button_state MouseLeft;
      game_button_state MouseRight;
      game_button_state MouseMiddle;
      game_button_state Escape;
    };
  };
};

struct game_memory
{
  bool HasBeenInitialized;

  uint32_t PersistentMemorySize;
  void*    PersistentMemory;

  uint32_t TemporaryMemorySize;
  void*    TemporaryMemory;
};

namespace Platform
{
  float GetTimeInSeconds();
  void  SetHighDPIAwareness();
}

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory GameMemory, const game_input* const Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
game_update_and_render GameUpdateAndRender;
