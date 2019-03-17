#include "ecs.h"
#include "ecs_management.h"
#include "ecs_internal.h"
#include "assert.h"
#include "string.h"

uint8_t*
GetComponentAddress(const chunk* Chunk, int32_t IndexInChunk, int32_t ComponentOffset,
                    int32_t ComponentSize)
{
  uint8_t* DesiredAddress = ((uint8_t*)Chunk) + ComponentOffset + IndexInChunk * ComponentSize;
  return DesiredAddress;
}

uint8_t*
GetComponentAddress(const ecs_world* World, entity_storage_info EntityStorage,
                    int32_t ComponentOffsetInChunk, component_struct_info ComponentInfo)
{
  return GetComponentAddress(GetChunkAtIndex(World->Runtime, EntityStorage.ChunkIndex),
                             EntityStorage.IndexInChunk, ComponentOffsetInChunk,
                             ComponentInfo.Size);
}


// Note(Lukas) all of this data is known at compile time and could be cached
bool
DoesArchetypeMatchRequest(const archetype& Archetype, const archetype_request& Request)
{
  for(int r = 0; r < Request.ComponentCount; r++)
  {
    component_request CurrentRequest = Request.ComponentRequests[r];
    bool              MatchedRequest = false;
    for(int c = 0; c < Archetype.ComponentTypes.Count; c++)
    {
      if(CurrentRequest.ID == Archetype.ComponentTypes[c].ID &&
         CurrentRequest.Type != REQUEST_Subtractive)
      {
        MatchedRequest = true;
      }
    }
    if(!MatchedRequest)
    {
      return false;
    }
  }
  return true;
}

// Note(Lukas) this data is known at compile time and could be cached
void
GetUsedComponents(fixed_stack<component_id, ECS_ARCHETYPE_COMPONENT_MAX_COUNT>* OutUsedComponents,
                  const archetype_request&                                      ArchetypeRequest)
{
  OutUsedComponents->Clear();
  for(int i = 0; i < ArchetypeRequest.ComponentCount; i++)
  {
    if((ArchetypeRequest.ComponentRequests[i].Type == REQUEST_Permission_R) ||
       (ArchetypeRequest.ComponentRequests[i].Type == REQUEST_Permission_RW))
    {
      OutUsedComponents->Push(ArchetypeRequest.ComponentRequests[i].ID);
    }
  }
  assert(OutUsedComponents->Count != 0);
}

uintptr_t
GetComponentOffset(const archetype& Archetype, component_id ComponentID)
{
  for(int i = 0; i < Archetype.ComponentTypes.Count; i++)
  {
    if(Archetype.ComponentTypes[i].ID == ComponentID)
    {
      uintptr_t Result = Archetype.ComponentTypes[i].Offset;
      return Result;
    }
  }
  assert(0 && "GetComponentOffset: Invalid codepath");
}

// Initialization
void
InitializeChunkHeap(ecs_runtime* Runtime, uint8_t* ChunkMemory, int32_t AvailableMemory)
{
  Runtime->ChunkHeap.Create(ChunkMemory, AvailableMemory);
}

void
InitializeArchetypeAndComponentTables(ecs_runtime*                 Runtime,
                                      const component_struct_info* ComponentInfos,
                                      const char** ComponentNames, int ComponentCount)
{
  Runtime->ComponentStructInfos.Clear();
  Runtime->ComponentNames.Clear();
  Runtime->Archetypes.Clear();
  Runtime->VacantArchetypeIndices.Clear();
  for(int i = 0; i < ComponentCount; i++)
  {
    Runtime->ComponentStructInfos.Push(ComponentInfos[i]);
    Runtime->ComponentNames.Push(ComponentNames[i]);
  }
}

void
InitializeWorld(ecs_world* World, const ecs_runtime* Runtime)
{
  World->Entities.Clear();
  World->VacantEntityIndices.Clear();
  World->EntityCommands.Clear();
  World->Runtime = (ecs_runtime*)Runtime;
}

// Integrating saved worlds (used importing deserialize'ing)
void
AdaptWorldToNewRuntime(ecs_world* World, uint8_t* OldRuntimMemory, const ecs_runtime* OldRuntime,
                       const ecs_runtime* NewRuntime)
{
}

void
ExecuteECSJob(ecs_runtime* runtime, const ecs_world* World,
              const archetype_request& ArchetypeRequest, ECS_JOB_FUNCTION_PARAMETERS(JobFunc))
{
  // Find matching archetypes
  fixed_stack<archetype*, ECS_ARCHETYPE_MAX_COUNT> MatchedArchetypes;
  MatchedArchetypes.Clear();

  for(int a = 0; a < runtime->Archetypes.Count; a++)
  {
    if(DoesArchetypeMatchRequest(runtime->Archetypes[a], ArchetypeRequest))
    {
      MatchedArchetypes.Push(&runtime->Archetypes[a]);
      a++;
    }
  }

  fixed_stack<component_id, ECS_ARCHETYPE_COMPONENT_MAX_COUNT> UsedComponents;
  UsedComponents.Clear();
  GetUsedComponents(&UsedComponents, ArchetypeRequest);
  assert(UsedComponents.Count < ECS_ARCHETYPE_COMPONENT_MAX_COUNT);
  for(int a = 0; a < MatchedArchetypes.Count; a++)
  {
    uintptr_t ComponentOffsetArray[ECS_ARCHETYPE_COMPONENT_MAX_COUNT];
    for(int CompInd = 0; CompInd < UsedComponents.Count; CompInd++)
    {
      ComponentOffsetArray[CompInd] =
        GetComponentOffset(*MatchedArchetypes[a], UsedComponents[CompInd]);
    }

    // Schedule jobs for execution
    // TODO(Lukas): compute the component offset arrays and shedule the chunk updates in a different
    for(chunk* CurrentChunk = MatchedArchetypes[a]->FirstChunk; CurrentChunk != NULL;
        CurrentChunk        = CurrentChunk->Header.NextChunk)
    {
      for(int CompInd = 0; CompInd < UsedComponents.Count; CompInd++)
      {
        ComponentOffsetArray[CompInd] += (uintptr_t)CurrentChunk;
      }
      JobFunc(&ComponentOffsetArray[0], UsedComponents.Count);
    }
  }
}

archetype*
GetEntityArchetype(const ecs_world* World, entity_id EntityID)
{
  assert(0 <= EntityID && EntityID < World->Entities.Count);
  archetype* Archetype = NULL;
  if(World->Entities[EntityID].ChunkIndex != -1)
  {
    chunk*              Chunks        = (chunk*)World->Runtime->ChunkHeap.GetBase();
    entity_storage_info EntityStorage = World->Entities[EntityID];
    Archetype = &World->Runtime->Archetypes[Chunks[EntityStorage.ChunkIndex].Header.ArchetypeIndex];
  }
  return Archetype;
}

// Entity API
// ChunkIndex == -1 && IndexInChunk == 0 means that entity is created but has no components
// ChunkIndex == -1 && IndexInChunk == -1 means that has been destroyed
entity_id
CreateEntity(ecs_world* World)
{
  assert(!World->Entities.Full() || !World->VacantEntityIndices.Empty());
  entity_id NewEntityID = {};
  if(!World->VacantEntityIndices.Empty())
  {
    NewEntityID = World->VacantEntityIndices.Pop();
  }
  else
  {
    NewEntityID = (entity_id)World->Entities.Count;
    World->Entities.Push({});
  }

  entity_storage_info* NewEntity = &World->Entities[NewEntityID];
  NewEntity->ChunkIndex          = -1;
  NewEntity->IndexInChunk        = 0;

  return (entity_id)NewEntityID;
}

bool
DoesEntityExist(const ecs_world* World, entity_id EntityID)
{
  if(0 <= EntityID && EntityID < World->Entities.Count)
  {
    entity_storage_info EntityStorage = World->Entities[EntityID];
    if(0 <= EntityStorage.ChunkIndex)
    {
      return true;
    }
    else if(EntityStorage.ChunkIndex == -1 && EntityStorage.IndexInChunk == 0)
    {
      return true;
    }
  }
  return false;
}

archetype*
GetChunkArchetype(const ecs_world* World, const chunk* Chunk)
{
	return &World->Runtime->Archetypes[Chunk->Header.ArchetypeIndex];
}

chunk*
GetChunkAtIndex(ecs_runtime* Runtime, int32_t ChunkIndex)
{
  chunk* Chunk = ((chunk*)Runtime->ChunkHeap.GetBase()) + ChunkIndex;
  return Chunk;
}

// ChunkIndex == -1 && IndexInChunk == 0 means that entity is created but has no components
// ChunkIndex == -1 && IndexInChunk == -1 means that has been destroyed
void
DestroyEntity(ecs_world* World, entity_id RemovedEntityID)
{
  assert(DoesEntityExist(World, RemovedEntityID));
  entity_storage_info RemovedEntity = World->Entities[RemovedEntityID];

	//If the entity is stored anywhere (has any components)
  if(RemovedEntity.ChunkIndex != -1)
  {
    chunk* Chunk = GetChunkAtIndex(World->Runtime, RemovedEntity.ChunkIndex);

    entity_storage_info LastEntity = {};
    LastEntity.ChunkIndex          = RemovedEntity.ChunkIndex;
    LastEntity.IndexInChunk        = Chunk->Header.EntityCount - 1;

    // Find the last chunk entity's ID
    // TODO(Lukas): Need a hash table for reverse lookup
    int32_t LastEntityID = -1;
    for(int i = 0; i < World->Entities.Count; i++)
    {
      if(World->Entities[i].ChunkIndex == LastEntity.ChunkIndex &&
         World->Entities[i].IndexInChunk == LastEntity.IndexInChunk)
      {
        LastEntityID = i;
        break;
      }
    }
    assert(LastEntityID != -1);

    archetype* Archetype = GetChunkArchetype(World, Chunk);

    // Copy data from last entity to the removed spot
    if(RemovedEntity.IndexInChunk != LastEntity.IndexInChunk)
    {
      for(int i = 0; i < Archetype->ComponentTypes.Count; i++)
      {
        component_id_and_offset OffsetAndID = Archetype->ComponentTypes[i];
        component_struct_info ComponentInfo = World->Runtime->ComponentStructInfos[OffsetAndID.ID];

        uint8_t* RemovedComponent = GetComponentAddress(Chunk, RemovedEntity.IndexInChunk,
                                                        OffsetAndID.Offset, ComponentInfo.Size);
        uint8_t* LastComponent    = GetComponentAddress(Chunk, LastEntity.IndexInChunk,
                                                     OffsetAndID.Offset, ComponentInfo.Size);

        memcpy(RemovedComponent, LastComponent, ComponentInfo.Size);
      }
      World->Entities[LastEntityID] = RemovedEntity;
    }
    // Remove the last entity in the chunk
    Chunk->Header.EntityCount--;

    // If the chunk became empty - free it
		
    if(Chunk->Header.EntityCount == 0)
		{
			chunk** PrevChunkPtrPtr = &Archetype->FirstChunk;
      for(chunk* C = Archetype->FirstChunk; C != Chunk; C = C->Header.NextChunk)
      {
        PrevChunkPtrPtr = &C->Header.NextChunk;
      }
      *PrevChunkPtrPtr = Chunk->Header.NextChunk;

      // If it was the only chunk, remove the archetype
      if(Archetype->FirstChunk == NULL)
      {
				RemoveArchetype(World->Runtime, Archetype);
			}
			
			//Free the memory
      World->Runtime->ChunkHeap.Dealloc((uint8_t*)Chunk);
		}
  }

  World->Entities[RemovedEntityID] = { -1, -1 };
  World->VacantEntityIndices.Push(RemovedEntityID);

  if(RemovedEntityID == World->Entities.Count - 1)
  {
    World->Entities.Pop();
  }
}

int32_t
GetComponentIndexInArchetype(const archetype& Archetype, component_id ComponentID)
{
  for(int i = 0; i < Archetype.ComponentTypes.Count; i++)
  {
    if(Archetype.ComponentTypes[i].ID == ComponentID)
    {
      return i;
    }
  }
  return -1;
}

bool
HasComponent(const ecs_world* World, entity_id EntityID, component_id ComponentID)
{
  assert(DoesEntityExist(World, EntityID));
  archetype* Archetype = GetEntityArchetype(World, EntityID);
  return (GetComponentIndexInArchetype(*Archetype, ComponentID) == -1) ? false : true;
}

int32_t
GetChunkEntityCapacity(const ecs_runtime* Runtime, const archetype& Archetype)
{
  int32_t ComponentSizeSum       = 0;
  int32_t MaximalAlignmentOffset = 0;
  for(int i = 0; i < Archetype.ComponentTypes.Count; i++)
  {
    ComponentSizeSum += Runtime->ComponentStructInfos[i].Size;
    MaximalAlignmentOffset += Runtime->ComponentStructInfos[i].Alignment - 1;
  }

  int32_t EntityCapacity =
    (ECS_CHUNK_SIZE - (int32_t)sizeof(chunk_header) - MaximalAlignmentOffset) / ComponentSizeSum;
  return EntityCapacity;
}

void
ComputeArchetypeComponentOffsets(archetype* Archetype, const ecs_runtime* Runtime)
{
  int32_t EntityCapacity = GetChunkEntityCapacity(Runtime, *Archetype);

  uint32_t UnalignedPosition = (uint32_t)sizeof(chunk_header);
  for(int i = 0; i < Archetype->ComponentTypes.Count; i++)
  {
    component_struct_info ComponentStructInfo =
      Runtime->ComponentStructInfos[Archetype->ComponentTypes[i].ID];

    uint32_t Alignment = Runtime->ComponentStructInfos[i].Alignment;
    uint32_t AlignmentFixup =
      (Alignment - (uint32_t)((uint64_t)UnalignedPosition & ((uint64_t)Alignment - 1)));

    uint32_t AlignedPosition = UnalignedPosition + AlignmentFixup;

    Archetype->ComponentTypes[i].Offset = (uint16_t)AlignedPosition;
    UnalignedPosition = AlignedPosition + EntityCapacity * Runtime->ComponentStructInfos[i].Size;
  }
}

int32_t
AddComponentWithoutLosingCanonicalForm(archetype* Archetype, component_id NewComponentID,
                                       const ecs_runtime* Runtime)
{
  const char* NewComponentName  = Runtime->ComponentNames[NewComponentID];
  int32_t     NewComponentIndex = -1;
  for(int i = 0; i < Runtime->ComponentNames.Count; i++) // Add to middle
  {
    int32_t cmp = strcmp(Runtime->ComponentNames[i], NewComponentName);
    assert(cmp != 0);

    if(cmp > 0)
    {
      component_id_and_offset NewComponentOffset = {};
      NewComponentOffset.ID                      = NewComponentID;
      Archetype->ComponentTypes.Insert(NewComponentOffset, i);
      NewComponentIndex = i;
      break;
    }
  }
  if(NewComponentIndex == -1) // Add to end
  {
    component_id_and_offset NewComponentOffset = {};
    NewComponentOffset.ID                      = NewComponentID;
    Archetype->ComponentTypes.Push(NewComponentOffset);
    NewComponentIndex = Archetype->ComponentTypes.Count - 1;
  }

  ComputeArchetypeComponentOffsets(Archetype, Runtime);

  return NewComponentIndex;
}

void
RemoveComponentWithoutLosingCanonicalForm(archetype* Archetype, component_id RemoveComponentID,
                                          const ecs_runtime* Runtime)
{
  for(int i = 0; i < Archetype->ComponentTypes.Count; i++)
  {
    if(Archetype->ComponentTypes[i].ID == RemoveComponentID)
    {
      Archetype->ComponentTypes.Remove(i);
      break;
    }
  }
  ComputeArchetypeComponentOffsets(Archetype, Runtime);
}

archetype*
AddArchetype(ecs_runtime* Runtime, const archetype& Archetype)
{
  assert(!Runtime->Archetypes.Full() || !Runtime->VacantArchetypeIndices.Empty());
  int32_t NewArchetypeIndex = -1;
  if(!Runtime->VacantArchetypeIndices.Empty())
  {
    NewArchetypeIndex = Runtime->VacantArchetypeIndices.Pop();
  }
  else
  {
    NewArchetypeIndex = (entity_id)Runtime->Archetypes.Count;
    Runtime->Archetypes.Push(Archetype);
  }

  archetype* NewArchetype  = &Runtime->Archetypes[NewArchetypeIndex];
  *NewArchetype            = Archetype;
  NewArchetype->FirstChunk = NULL;

  return NewArchetype;
}



void
RemoveArchetypeAtIndex(ecs_runtime* Runtime, int32_t RemoveIndex)
{
  assert(0 <= RemoveIndex && RemoveIndex < Runtime->Archetypes.Count);
  archetype* RemovedArchetype = &Runtime->Archetypes[RemoveIndex];

  assert(RemovedArchetype->FirstChunk == NULL);
  RemovedArchetype->ComponentTypes.Clear();

  Runtime->VacantArchetypeIndices.Push(RemoveIndex);
}

void
RemoveArchetype(ecs_runtime* Runtime, archetype* Archetype)
{
  int32_t ArchetypeIndex = (int32_t)(Archetype - &Runtime->Archetypes[0]);
  RemoveArchetypeAtIndex(Runtime, ArchetypeIndex);
}

int32_t
GetChunkIndex(const ecs_runtime* Runtime, const chunk* C)
{
  int32_t ChunkIndex = (int32_t)(C - (chunk*)Runtime->ChunkHeap.GetBase());
  return ChunkIndex;
}

int32_t
GetArchetypeIndex(const ecs_runtime* Runtime, const archetype* Archetype)
{
  int32_t ArchetypeIndex = (int32_t)(Archetype - Runtime->Archetypes.Elements);
  return ArchetypeIndex;
}

entity_storage_info
CreateNewArchetypeInstance(ecs_runtime* Runtime, archetype* Archetype)
{
  assert(Archetype);
  entity_storage_info NewEntityStorage = {};

  chunk** PrevCPtrPtr = &Archetype->FirstChunk;
  chunk*  C           = Archetype->FirstChunk;
  for(; C != NULL; C = C->Header.NextChunk)
  {
    PrevCPtrPtr = &C->Header.NextChunk;
    if(C->Header.EntityCount < C->Header.EntityCapacity)
    {
      NewEntityStorage.ChunkIndex   = (int16_t)GetChunkIndex(Runtime, C);
      NewEntityStorage.IndexInChunk = C->Header.EntityCount;
      C->Header.EntityCount++;
      break;
    }
  }

  if(C == NULL)
  {
    // If no vacancies were found
    chunk* NewChunk = (chunk*)Runtime->ChunkHeap.Alloc(sizeof(chunk));
    if(Archetype->FirstChunk)
    {
      NewChunk->Header           = Archetype->FirstChunk->Header;
      NewChunk->Header.NextChunk = NULL;
    }
    else
    {
      NewChunk->Header                = {};
      NewChunk->Header.EntityCapacity = (int16_t)GetChunkEntityCapacity(Runtime, *Archetype);
      NewChunk->Header.ArchetypeIndex = (int16_t)GetArchetypeIndex(Runtime, Archetype);
    }
    NewChunk->Header.EntityCount = 1;
    *PrevCPtrPtr                 = NewChunk;

    NewEntityStorage.ChunkIndex   = (int16_t)GetChunkIndex(Runtime, NewChunk);
    NewEntityStorage.IndexInChunk = 0;
  }

  return NewEntityStorage;
}

void
CopyMatchingComponentValues(ecs_world* World, entity_storage_info DstStorage,
                            entity_storage_info SrcStorage, const archetype& DstArchetype,
                            const archetype& SrcArchetype)
{
  chunk* DstChunk = GetChunkAtIndex(World->Runtime, DstStorage.ChunkIndex);
  chunk* SrcChunk = GetChunkAtIndex(World->Runtime, SrcStorage.ChunkIndex);

  int SrcStartIndex = 0;
  for(int i = 0; i < DstArchetype.ComponentTypes.Count; i++)
  {
    int j = SrcStartIndex;
    for(; j < SrcArchetype.ComponentTypes.Count; j++)
    {
      if(DstArchetype.ComponentTypes[i].ID == SrcArchetype.ComponentTypes[j].ID)
      {
        SrcStartIndex++;

        component_struct_info ComponentInfo =
          World->Runtime->ComponentStructInfos[DstArchetype.ComponentTypes[i].ID];
        uint8_t* DstComponentAddress =
          GetComponentAddress(DstChunk, DstStorage.IndexInChunk,
                              DstArchetype.ComponentTypes[i].Offset, ComponentInfo.Size);
        uint8_t* SrcComponentAddress =
          GetComponentAddress(SrcChunk, SrcStorage.IndexInChunk,
                              SrcArchetype.ComponentTypes[j].Offset, ComponentInfo.Size);
        memcpy(DstComponentAddress, SrcComponentAddress, ComponentInfo.Size);
        break;
      }
    }
  }
}

void
AddComponent(ecs_world* World, entity_id EntityID, component_id ComponentID)
{
  assert(DoesEntityExist(World, EntityID));
  const archetype* PreviousArchetype = GetEntityArchetype(World, EntityID);

  archetype TempArchetype = {};
  if(PreviousArchetype) // If entity had any components
  {
    int32_t ComponentIndex = GetComponentIndexInArchetype(*PreviousArchetype, ComponentID);
    assert(ComponentIndex == -1 && "assert: trying to add a component for the second time");
    TempArchetype = *PreviousArchetype;
  }

  int32_t NewComponentIndex =
    AddComponentWithoutLosingCanonicalForm(&TempArchetype, ComponentID, World->Runtime);

  const archetype* MatchedArchetype = NULL;
  for(int i = 0; i < World->Runtime->Archetypes.Count; i++)
  {
    if(memcmp(&TempArchetype, &World->Runtime->Archetypes[i], sizeof(archetype)) == 0)
    {
      MatchedArchetype = &World->Runtime->Archetypes[i];
      break;
    }
  }

  archetype* NewArchetype = NULL;
  if(!MatchedArchetype)
  {
    NewArchetype = AddArchetype(World->Runtime, TempArchetype);
  }

  entity_storage_info NewEntityStorage = CreateNewArchetypeInstance(World->Runtime, NewArchetype);

  if(PreviousArchetype)
  {
    CopyMatchingComponentValues(World, NewEntityStorage, World->Entities[EntityID], *NewArchetype,
                                *PreviousArchetype);
  }
  // Zero out the new component
  {
    component_id_and_offset NewComponentIDAndOffset =
      NewArchetype->ComponentTypes[NewComponentIndex];

    component_struct_info ComponentInfo =
      World->Runtime->ComponentStructInfos[NewComponentIDAndOffset.ID];
    uint8_t* NewComponentAddress =
      GetComponentAddress(World, NewEntityStorage, NewComponentIDAndOffset.Offset, ComponentInfo);

    memset(NewComponentAddress, 0, (size_t)ComponentInfo.Size);
  }
  World->Entities[EntityID] = NewEntityStorage;
}

void
RemoveComponent(ecs_world* World, entity_id EntityID, component_id ComponentID)
{
  // TODO(Lukas) Finish this
  assert(DoesEntityExist(World, EntityID));
  const archetype* OldArchetype = GetEntityArchetype(World, EntityID);
  assert(OldArchetype && "assert: tryping to remove component from empty archetype");

  int32_t ComponentIndex = GetComponentIndexInArchetype(*OldArchetype, ComponentID);
  assert(ComponentIndex == -1 && "assert: trying to remove non-present component");
  archetype TempArhcetype = *OldArchetype;
}

void*
GetComponent_(ecs_world* World, entity_id EntityID, size_t ComponentSize, component_id ComponentID)
{
  assert(DoesEntityExist(World, EntityID));
  component_struct_info ComponentInfo = World->Runtime->ComponentStructInfos[ComponentID];
  assert(ComponentInfo.Size == ComponentSize);

  const archetype* Archetype      = GetEntityArchetype(World, EntityID);
  int32_t          ComponentIndex = GetComponentIndexInArchetype(*Archetype, ComponentID);

  entity_storage_info EntityStorage = World->Entities[EntityID];

  uint8_t* DesiredComponent =
    GetComponentAddress(GetChunkAtIndex(World->Runtime, EntityStorage.ChunkIndex),
                        EntityStorage.IndexInChunk,
                        Archetype->ComponentTypes[ComponentIndex].Offset, ComponentInfo.Size);
  return DesiredComponent;
}

void
SetComponent_(ecs_world* World, entity_id EntityID, const void* ComponentValue,
              size_t ComponentSize, component_id ComponentID)
{
  void* DesiredComponent = GetComponent_(World, EntityID, ComponentSize, ComponentID);
  memcpy(DesiredComponent, ComponentValue, ComponentSize);
}
