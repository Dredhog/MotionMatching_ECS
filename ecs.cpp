#include "ecs.h"
#include "ecs_internal.h"
#include "assert.h"

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
      uintptr_t Result = Archetype.ComponentTypes[i].OffsetInBytes;
      return Result;
    }
  }
  assert(0 && "GetComponentOffset: Invalid codepath");
}


// Initialization
void
InitializeChunkHeap(ecs_runtime* Runtime, uint8_t* ChunkMemory, size_t MemorySize)
{
	
}

void
InitializeComponentTable(ecs_world* World, const component_struct_info* ComponentInfos,
                         const char** ComponentNames, int ComponentCount)
{
}

ecs_world*
InitializeWorld(ecs_world* World)
{
}

// Integrating saved worlds (used importing deserialize'ing)
bool
AdaptWorldToNewRuntime(ecs_world* World, const ecs_runtime* OldRuntime,
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
    chunk* CurrentChunk = MatchedArchetypes[a]->FirstChunk;
    for(; CurrentChunk != NULL; CurrentChunk = CurrentChunk->Header.NextChunk)
    {
      for(int CompInd = 0; CompInd < UsedComponents.Count; CompInd++)
      {
        ComponentOffsetArray[CompInd] += (uintptr_t)CurrentChunk;
      }
      JobFunc(&ComponentOffsetArray[0], UsedComponents.Count);
    }
  }
}
