#pragma once
#include "ecs.h"
#include "ecs_management.h"

// Internal archetype API
bool DoesArchetypeMatchRequest(const archetype& Archetype, const archetype_request& Request);
int32_t    GetArchetypeIndex(const ecs_runtime* Runtime, component_id* ComponentIDs,
                             int32_t ComponentCount);
archetype* GetEntityArchetype(const ecs_world* World, entity_id EntityID);
void CreateArchetype(ecs_runtime* Runtime, component_id* ComponentIDs, int32_t ComponentCount);
void GetUsedComponents(
  fixed_stack<component_id, ECS_ARCHETYPE_COMPONENT_MAX_COUNT>* OutUsedComponents,
  const archetype_request&                                      ArchetypeRequest);
uintptr_t  GetComponentOffset(const archetype& Archetype, component_id ComponentID);
uint8_t*   GetComponentAddress(const chunk* Chunk, int32_t IndexInChunk, int32_t ComponentOffset,
                               int32_t ComponentSize);
uint8_t*   GetComponentAddress(const ecs_world* World, entity_storage_info EntityStorage,
                               int32_t ComponentOffsetInChunk, component_struct_info ComponentInfo);
archetype* GetEntityArchetype(const ecs_world* World, entity_id EntityID);

int32_t    GetChunkEntityCapacity(const ecs_runtime* Runtime, const archetype& Archetype);
archetype* GetChunkArchetype(const ecs_world* World, const chunk* Chunk);

int32_t GetComponentIndexInArchetype(const archetype& Archetype, component_id ComponentID);
int32_t AddComponentWithoutLosingCanonicalForm(archetype* Archetype, component_id NewComponentID,
                                               const ecs_runtime* Runtime);
void RemoveComponentWithoutLosingCanonicalForm(archetype* Archetype, component_id RemoveComponentID,
                                               const ecs_runtime* Runtime);

int32_t    GetArchetypeIndex(const ecs_runtime* Runtime, const archetype* Archetype);
void       ComputeArchetypeComponentOffsets(archetype* Archetype, const ecs_runtime* Runtime);
archetype* AddArchetype(ecs_runtime* Runtime, const archetype* Archetype);
void       RemoveArchetypeAtIndex(ecs_runtime* Runtime, int32_t RemoveIndex);
void RemoveArchetype(ecs_runtime* Runtime, archetype* Archetype);

int32_t GetChunkIndex(const ecs_runtime* Runtime, const chunk* C);
chunk*  GetChunkAtIndex(ecs_runtime* Runtime, int32_t ChunkIndex);

entity_storage_info CreateNewArchetypeInstance(ecs_world* World, archetype* Archetype);

void CopyMatchingComponentValues(ecs_world* World, entity_storage_info DstStorage,
                                 entity_storage_info SrcStorage, const archetype& DstArchetype,
                                 const archetype& SrcArchetype);
