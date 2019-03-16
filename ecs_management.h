#pragma once
#include "ecs.h"
#include "memory.h"

struct ecs_runtime;

// Initialization
void InitializeChunkHeap(ecs_runtime* Runtime, uint8_t* ChunkMemory, size_t MemorySize);
void InitializeComponentTable(ecs_world* World, const component_struct_info* ComponentInfos,
                              const char** ComponentNames, int ComponentCount);

ecs_world* InitializeWorld(ecs_world* World);

//Integrating saved worlds (used importing deserialize'ing)
bool AdaptWorldToNewRuntime(ecs_world* World, const ecs_runtime* OldRuntime,
                            const ecs_runtime* NewRuntime);
