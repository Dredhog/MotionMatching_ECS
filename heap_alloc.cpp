#include "heap_alloc.h"

#include <cassert>
#include <stdlib.h>

namespace Memory
{
  void
  heap_allocator::Create(void* Base, int32_t Capacity)
  {
    assert(Base);
    assert(Capacity >= sizeof(allocation_info));

    m_Base     = (uint8_t*)Base;
    m_Capacity = Capacity;
    m_Barrier  = m_Base;

    m_FirstFreeBlock       = (free_block*)m_Base;
    m_FirstFreeBlock->Next = NULL;

    m_FirstFreeBlock->Size = m_Capacity;

    // Grows down from the end of available memory
    m_AllocInfos = (allocation_info*)(m_Base + m_Capacity);
    m_AllocCount = 0;
  }

  void
  heap_allocator::Clear()
  {
    this->Create(this->m_Base, this->m_Capacity);
  }

  uint8_t*
  heap_allocator::Alloc(int32_t Size)
  {
    assert(Size >= sizeof(free_block));

    free_block* PreviousBlock = NULL;
    free_block* CurrentBlock  = m_FirstFreeBlock;
    while(CurrentBlock != NULL)
    {
      // Currently first fit strategy
      if(Size <= CurrentBlock->Size)
      {
        if(CurrentBlock->Next == NULL)
        {
          m_Barrier = (uint8_t*)CurrentBlock + Size;
#if 0
          // Restrict size up to the end of allocation_infos size_t TempStore =
          (uint8_t*)(m_AllocInfos - m_AllocCount - 1) - (uint8_t*)CurrentBlock;
          assert(TempStore <= 0x7FFFFFFF);
          Size = (int32_t)TempStore;
#endif
        }

        if(m_Barrier + sizeof(free_block) > (uint8_t*)(m_AllocInfos - m_AllocCount - 1))
        {
          CurrentBlock = NULL;
        }
        break;
      }

      PreviousBlock = CurrentBlock;
      CurrentBlock  = CurrentBlock->Next;
    }

    if(CurrentBlock != NULL)
    {
      ++m_AllocCount;

      allocation_info* NewInfo = &m_AllocInfos[-m_AllocCount];
      NewInfo->Base            = (uint8_t*)CurrentBlock;
      NewInfo->Size            = Size;
      NewInfo->AlignmentOffset = 0;

      if((CurrentBlock->Size - Size) >= sizeof(free_block))
      {
        free_block* NewBlock = (free_block*)(NewInfo->Base + NewInfo->Size);
        NewBlock->Next       = CurrentBlock->Next;
        NewBlock->Size       = CurrentBlock->Size - Size;

        if(PreviousBlock != NULL)
        {
          PreviousBlock->Next = NewBlock;
        }
        else
        {
          m_FirstFreeBlock = NewBlock;
        }
      }
      else
      {
        NewInfo->Size = CurrentBlock->Size;
        if(PreviousBlock != NULL)
        {
          PreviousBlock->Next = CurrentBlock->Next;
        }
        else
        {
          m_FirstFreeBlock = CurrentBlock->Next;
        }
      }
      return NewInfo->Base;
    }
    assert(0 && "heap error: adequate block not found");
    return NULL;
  }

  void
  heap_allocator::Dealloc(uint8_t* Start)
  {
    int32_t AllocIndex = -1;
    for(int i = 0; i < m_AllocCount; i++)
    {
      allocation_info* AllocInfo = &m_AllocInfos[-m_AllocCount + i];
      if(AllocInfo->Base == Start)
      {
        AllocIndex = i;
        break;
      }
    }

    if(AllocIndex != -1)
    {
      allocation_info* AllocInfo     = &m_AllocInfos[-m_AllocCount + AllocIndex];
      free_block*      PreviousBlock = NULL;
      free_block*      CurrentBlock  = m_FirstFreeBlock;

      while(CurrentBlock != NULL)
      {
        if((uint8_t*)CurrentBlock >= (AllocInfo->Base + AllocInfo->Size))
        {
          break;
        }
        PreviousBlock = CurrentBlock;
        CurrentBlock  = CurrentBlock->Next;
      }

      if(PreviousBlock == NULL)
      {
        PreviousBlock       = (free_block*)Start;
        PreviousBlock->Size = AllocInfo->Size;
        PreviousBlock->Next = m_FirstFreeBlock;

        m_FirstFreeBlock = PreviousBlock;
      }
      else if(((uint8_t*)PreviousBlock + PreviousBlock->Size) == Start)
      {
        PreviousBlock->Size += AllocInfo->Size;

        // resetting barrier for safety checks
        if(PreviousBlock->Next == NULL)
        {
          m_Barrier = (uint8_t*)PreviousBlock;
        }
      }
      else
      {
        free_block* TempBlock = (free_block*)Start;
        TempBlock->Size       = AllocInfo->Size;
        TempBlock->Next       = CurrentBlock;
        PreviousBlock->Next   = TempBlock;
        PreviousBlock         = TempBlock;
      }

      if(CurrentBlock != NULL && (uint8_t*)CurrentBlock == (AllocInfo->Base + AllocInfo->Size))
      {
        PreviousBlock->Size += CurrentBlock->Size;
        PreviousBlock->Next = CurrentBlock->Next;

        // resetting barrier for safety checks
        if(PreviousBlock->Next == NULL)
        {
          m_Barrier = (uint8_t*)PreviousBlock;
        }
      }

      m_AllocInfos[-m_AllocCount + AllocIndex] = m_AllocInfos[-m_AllocCount];
      --m_AllocCount;
    }
    else
    {
      assert(0 && "heap error: no block to free at specified address");
    }
  }

  uint8_t*
  heap_allocator::GetBase() const
  {
    return this->m_Base;
  }

  int32_t
  heap_allocator::GetAllocationCount() const
  {
    return this->m_AllocCount;
  }

  allocation_info*
  heap_allocator::GetAllocationInfos() const
  {
    return &m_AllocInfos[-m_AllocCount];
  }
}
