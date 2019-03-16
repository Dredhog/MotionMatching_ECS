#pragma once
#include "stdint.h"

typedef uint8_t component_id;
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
entity_id DestroyEntity(ecs_world* World);

bool HasComponent(const ecs_world* World, entity_id Entity, component_id ComponentID);
bool AddComponent(ecs_world* World, entity_id Entity, component_id ComponentID);
bool RemoveComponent(ecs_world* World, entity_id Entity, component_id ComponentID);

bool GetComponent(ecs_world* World, entity_id Entity, component_id ComponentID);

bool SetComponent_(ecs_world* World, entity_id Entity, const void* ComponentValue,
                   uint16_t ComponentSize, component_id ComponentID);
#define SetComponent(World, Entity, ComponentName, Value)                                          \
  static_assert(sizeof(ComponentName) == sizeof(Value),                                            \
                "compile time assertions: SetComponent() ComponentName and Value mismatch");       \
  SetComponent_(World, Entity, Value, sizeof(Value), COMPONENT_##ComponentName)
