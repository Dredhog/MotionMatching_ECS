#pragma once
#include "stdint.h"

const int ECS_CHUNK_SIZE                           = 16 * 1024;
const int ECS_ENTITY_MAX_COUNT                     = 200;
const int ECS_WORLD_ENTITY_COMMAND_BUFFER_CAPACITY = 200;

const int ECS_ARCHETYPE_COMPONENT_MAX_COUNT = 20;
const int ECS_COMPONENT_MAX_COUNT           = 20;
const int ECS_ARCHETYPE_MAX_COUNT           = 64;

typedef int16_t component_id;
typedef int16_t entity_id;
struct ecs_world;

enum component_request_type
{
  REQUEST_Permission_RW   = 0,
  REQUEST_Permission_R    = 1,
  REQUEST_Permission_None = 2,
  REQUEST_Subtractive     = 3,
};

struct component_request
{
  component_id ID;
  uint8_t      Type;
};

struct archetype_request
{
  component_request* ComponentRequests;
  int32_t            ComponentCount;
};

// Job API

#define ECS_JOB_FUNCTION(Name) void Name(void* Components, int32_t Count)
#define ECS_JOB_FUNCTION_PARAMETERS(Name) ECS_JOB_FUNCTION((*Name))

void ExecuteECSJob(const ecs_world* World, const archetype_request* ArchetypeRequest,
                   ECS_JOB_FUNCTION_PARAMETERS(JobFunc));

// Entity API
entity_id CreateEntity(ecs_world* World);
void      DestroyEntity(ecs_world* World, entity_id EntityID);

bool HasComponent(const ecs_world* World, entity_id EntityID, component_id ComponentID);
void AddComponent(ecs_world* World, entity_id EntityID, component_id ComponentID);
void RemoveComponent(ecs_world* World, entity_id EntityID, component_id ComponentID);

void* GetComponent(ecs_world* World, entity_id EntityID, component_id ComponentID);

void SetComponent_(ecs_world* World, entity_id EntityID, const void* ComponentValue,
                   uint16_t ComponentSize, component_id ComponentID);

#define SetComponent(World, Entity, ComponentName, Value)                                          \
  static_assert(sizeof(ComponentName) == sizeof(Value),                                            \
                "compile time assertions: SetComponent() ComponentName and Value mismatch");       \
  SetComponent_(World, Entity, Value, sizeof(Value), COMPONENT_##ComponentName)
