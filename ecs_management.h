#pragma once
#include "ecs.h"
#include "basic_data_structures.h"
#include "heap_alloc.h"

union chunk;

struct chunk_header
{
  chunk*   NextChunk;
  uint16_t ArchetypeIndex;
  uint16_t EntityCount;
  uint16_t EntityCapacity;
};

union chunk {
  chunk_header Header;
  uint8_t      Memory[ECS_CHUNK_SIZE];
};

struct component_id_and_offset
{
  component_id ID;
  uint16_t     Offset;
};

struct archetype
{
  chunk*                                                                  FirstChunk;
  fixed_stack<component_id_and_offset, ECS_ARCHETYPE_COMPONENT_MAX_COUNT> ComponentTypes;
};

struct component_struct_info
{
  uint8_t  Alignment;
  uint16_t Size;
};

struct ecs_runtime
{
  fixed_stack<archetype, ECS_ARCHETYPE_MAX_COUNT> Archetypes;
  fixed_stack<int32_t, ECS_ARCHETYPE_MAX_COUNT>   VacantArchetypeIndices;

  fixed_stack<const char*, ECS_COMPONENT_MAX_COUNT>           ComponentNames;
  fixed_stack<component_struct_info, ECS_COMPONENT_MAX_COUNT> ComponentStructInfos;

  Memory::heap_allocator ChunkHeap;
};

union entity_storage_info {
  struct
  {
    int16_t ChunkIndex;
    int16_t IndexInChunk;
  };
  int32_t NextFreeEntityIndex;
};

struct entity_command
{
  uint8_t*  Data;
  entity_id Index;
  uint16_t  Size;
  uint16_t  Flags;
};

struct ecs_world
{
  ecs_runtime* Runtime;

  fixed_stack<entity_storage_info, ECS_ENTITY_MAX_COUNT> Entities;
  fixed_stack<entity_id, ECS_ENTITY_MAX_COUNT>           VacantEntityIndices;

  fixed_stack<entity_command, ECS_WORLD_ENTITY_COMMAND_BUFFER_CAPACITY> EntityCommands;
};

// Initialization
void InitializeChunkHeap(ecs_runtime* Runtime, uint8_t* ChunkMemory, int32_t MemorySize);
void InitializeArchetypeAndComponentTables(ecs_runtime*                 Runtime,
                                           const component_struct_info* ComponentInfos,
                                           const char** ComponentNames, int ComponentCount);
void InitializeWorld(ecs_world* World, const ecs_runtime* Runtime);
void RunWorldCommandBuffer(ecs_runtime* Runtime, ecs_world* World);

// Integrating saved worlds (used importing deserialize'ing)
void AdaptWorldToNewRuntime(ecs_world* World, const ecs_runtime* OldRuntime,
                            const ecs_runtime* NewRuntime);
