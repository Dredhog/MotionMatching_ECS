#include "scene.h"
#include "stack_allocator.h"
#include "file_io.h"

void
ExportScene(game_state* GameState, const char* path)
{
  GameState->TemporaryMemStack->Clear();
  scene* Scene = PushStruct(GameState->TemporaryMemStack, scene);
  *Scene       = {};

  Scene->EntityCount = GameState->EntityCount;
  Scene->Entities    = PushArray(GameState->TemporaryMemStack, Scene->EntityCount, entity);
  memcpy(Scene->Entities, GameState->Entities, Scene->EntityCount * sizeof(entity));

  Scene->EntityCount = GameState->EntityCount;
  Scene->Entities    = PushArray(GameState->TemporaryMemStack, Scene->EntityCount, entity);
  memcpy(Scene->Entities, GameState->Entities, Scene->EntityCount * sizeof(entity));
}
