#include "scene.h"
#include "stack_allocator.h"
#include "file_io.h"
#include "asset.h"
#include "anim.h"
#include <stdlib.h>

void
ExportScene(game_state* GameState, const char* Path)
{
  GameState->TemporaryMemStack->Clear();
  Asset::asset_file_header* AssetHeader =
    PushStruct(GameState->TemporaryMemStack, Asset::asset_file_header);
  scene* Scene = PushStruct(GameState->TemporaryMemStack, scene);

  *Scene = {};

  Scene->EntityCount = GameState->EntityCount;
  Scene->Entities    = PushArray(GameState->TemporaryMemStack, Scene->EntityCount, entity);
  memcpy(Scene->Entities, GameState->Entities, Scene->EntityCount * sizeof(entity));

  Scene->AnimControllers =
    (Anim::animation_controller*)GameState->TemporaryMemStack->GetMarker().Address;
  for(int e = 0; e < Scene->EntityCount; e++)
  {
    if(Scene->Entities[e].AnimController)
    {
      Anim::animation_controller* AnimController =
        PushStruct(GameState->TemporaryMemStack, Anim::animation_controller);
      *AnimController = *Scene->Entities[e].AnimController;
      ++Scene->AnimControllerCount;
      Scene->Entities[e].AnimController =
        (Anim::animation_controller*)(uintptr_t)Scene->AnimControllerCount;
    }
  }

  uint64_t AssetBase = (uint64_t)AssetHeader;

  AssetHeader->Scene = (uint64_t)Scene - AssetBase;
  Scene->Entities    = (entity*)((uint64_t)Scene->Entities - AssetBase);
  Scene->AnimControllers =
    (Anim::animation_controller*)((uint64_t)Scene->AnimControllers - AssetBase);

  AssetHeader->Checksum  = ASSET_HEADER_CHECKSUM;
  AssetHeader->AssetType = Asset::ASSET_Scene;
  AssetHeader->TotalSize = GameState->TemporaryMemStack->GetUsedSize();

  WriteEntireFile(Path, AssetHeader->TotalSize, AssetHeader);
}

void
ImportScene(game_state* GameState, const char* Path)
{
  GameState->TemporaryMemStack->Clear();
  debug_read_file_result ReadResult = ReadEntireFile(GameState->TemporaryMemStack, Path);
  assert(ReadResult.Contents);
  Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)ReadResult.Contents;
  assert(AssetHeader->Checksum == ASSET_HEADER_CHECKSUM);

  uint64_t AssetBase = (uint64_t)AssetHeader;

  AssetHeader->Scene = (uint64_t)AssetHeader->Scene + AssetBase;
}
