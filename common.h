#pragma once

#include <stdint.h>
#include "model.h"
#include "anim.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "stack_allocator.h"

#define ArrayCount(Array) sizeof((Array)) / sizeof(Array[0])

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
    game_button_state Buttons[24];
    struct
    {
      game_button_state a;
      game_button_state b;
      game_button_state d;
      game_button_state e;
      game_button_state f;
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
      game_button_state LeftShift;
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

  float NearClipPlane;
  float FarClipPlane;
  float FieldOfView;

  float Speed;
  float MaxTiltAngle;

  vec3 Rotation;
  mat4 ViewMatrix;
  mat4 ProjectionMatrix;
  mat4 VPMatrix;
};

struct game_state
{
  Memory::stack_allocator* PersistentMemStack;
  Memory::stack_allocator* TemporaryMemStack;

  Render::model*  Model;
  Anim::skeleton* Skeleton;
  Render::model*  GizmoModel;
  uint32_t        Texture;

  int ShaderBoneColor;
  int ShaderWireframe;
  int ShaderDiffuse;
  int ShaderTexture;
  int ShaderGizmo;

  uint32_t MagicChecksum;
  vec3     MeshEulerAngles;
  vec3     MeshScale;

  bool DrawWireframe;
  bool DrawBoneWeights;
  bool DrawGizmos;

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
  void name(game_memory GameMemory, game_state* GameState, const game_input* const Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
}
