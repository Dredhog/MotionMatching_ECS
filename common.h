#pragma once

#include <stdint.h>
#include "mesh.h"
#include "linear_math/vector.h"
#include "stack_allocator.h"

#define ArrayCount(Array) sizeof((Array)) / sizeof(Array[0])

struct loaded_bitmap
{
  void*   Texels;
  int32_t Width;
  int32_t Height;
};

struct debug_read_file_result
{
  void*    Contents;
  uint32_t ContentsSize;
};

struct game_button_state
{
  uint8_t EndedDown;
  uint8_t Changed;
};

struct game_input
{
  int32_t MouseX;
  int32_t MouseY;
  int32_t dMouseX;
  int32_t dMouseY;
  float   dt;

  union {
    game_button_state Buttons[21];
    struct
    {
      game_button_state a;
      game_button_state d;
      game_button_state e;
      game_button_state g;
      game_button_state m;
      game_button_state n;
      game_button_state o;
      game_button_state p;
      game_button_state r;
      game_button_state s;
      game_button_state t;
      game_button_state w;
      game_button_state LeftCtrl;
      game_button_state Space;
      game_button_state ArrowUp;
      game_button_state ArrowDown;
      game_button_state ArrowLeft;
      game_button_state ArrowRight;
      game_button_state MouseLeft;
      game_button_state MouseRight;
      game_button_state Escape;
    };
  };
};

struct camera
{
  vec3 P;
  vec3 Up;
  vec3 Right;
  vec3 Forward;

  vec3 Rotation;

  float Speed;

  vec3 FieldOfView;
};

struct game_state
{
  Memory::stack_allocator* PersistentMemStack;
  Memory::stack_allocator* TemporaryMemStack;

  Mesh::mesh Mesh;

  int ShaderWireframe;
  int ShaderVertexColor;

  uint32_t MagicChecksum;
  vec3     MeshEulerAngles;
  vec3     MeshScale;

  camera Camera;
};

struct game_memory
{
  bool HasBeenInitialized;

  uint32_t PersistentMemorySize;
  void*    PersistentMemory;

  uint32_t TemporaryMemorySize;
  void*    TemporaryMemory;
};

#define GAME_UPDATE_AND_RENDER(name)                                                               \
  void name(game_memory GameMemory, game_state* GameState, bool* AssetsHaveLoaded,                 \
            const game_input* const Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
}

#define PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char* Filename)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void name(debug_read_file_result FileHandle)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

#define PLATFORM_WRITE_ENTIRE_FILE(name)                                                           \
  int32_t name(char* Filename, uint64_t MemorySize, void* Memory)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_LOAD_BITMAP_FROM_FILE(name) loaded_bitmap name(char* Filename)
typedef PLATFORM_LOAD_BITMAP_FROM_FILE(platform_load_bitmap_from_file);
