#pragma once
#include "ecs.h"
#include "basic_data_structures.h"
#include "heap_alloc.h"

const size_t ECS_CHUNK_SIZE                           = 16 * 1024;
const int    ECS_ARCHETYPE_COMPONENT_MAX_COUNT        = 20;
const int    ECS_COMPONENT_MAX_COUNT                  = 20;
const int    ECS_MAX_NAME_SIZE                        = 32;
const int    ECS_ARCHETYPE_MAX_COUNT                  = 64;
const int    ECS_ENTITY_MAX_COUNT                     = 200;
const int    ECS_WORLD_ENTITY_COMMAND_BUFFER_CAPACITY = 200;

struct chunk
{
  union {
    struct
    {
      chunk*   NextChunk;
      uint16_t ArchetypeIndex;
      uint16_t EntityCapacity;
      uint16_t EntityCount;
    } Header;
    uint8_t Memory[ECS_CHUNK_SIZE];
  };
};

struct component_id_and_offset
{
  component_id ID;
  uint16_t     OffsetInBytes;
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
  fixed_stack<archetype, ECS_ARCHETYPE_COMPONENT_MAX_COUNT> Archetypes;

  fixed_stack<const char*, ECS_COMPONENT_MAX_COUNT>           ComponentNames;
  fixed_stack<component_struct_info, ECS_COMPONENT_MAX_COUNT> ComponentStructInfos;

	Memory::heap_allocator ChunkHeap;
};

struct entity_storage_info
{
  uint16_t ChunkIndex;
  uint16_t IndexInChunk;
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
  entity_storage_info EntityStorateInfos[ECS_ENTITY_MAX_COUNT];
  fixed_stack<entity_command, ECS_WORLD_ENTITY_COMMAND_BUFFER_CAPACITY> EntityCommands;
};

//Internal archetype API
bool    DoesArchetypeMatchRequest(const archetype& Archetype, const archetype_request& Request);
int32_t GetArchetypeIndex(const ecs_runtime* Runtime, component_id* ComponentIDs,
                          int32_t ComponentCount);
void    CreateArchetype(ecs_runtime* Runtime, component_id* ComponentIDs, int32_t ComponentCount);
void    GetUsedComponents(
     fixed_stack<component_id, ECS_ARCHETYPE_COMPONENT_MAX_COUNT>* OutUsedComponents,
     const archetype_request&                                      ArchetypeRequest);
uintptr_t GetComponentOffset(const archetype& Archetype, component_id ComponentID);
