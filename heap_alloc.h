#pragma once

#include <stdint.h>

namespace Memory
{
  struct free_block
  {
    free_block* Next;
    int32_t     Size;
  };

  struct allocation_info
  {
    uint8_t* Base;
    int32_t  Size;
    uint8_t  AlignmentOffset;
  };

  struct heap_allocator
  {

    uint8_t*         m_Base;
    int32_t          m_Capacity;
    uint8_t*         m_Barrier;
    free_block*      m_FirstFreeBlock;
    allocation_info* m_AllocInfos;
    int32_t          m_AllocCount;

  public:
    void Create(void* Base, int32_t Capacity);

    uint8_t* Alloc(int32_t SizeBytes);
    uint8_t* AllignedAlloc(int32_t SizeBytes, int32_t Alignment);

    void Dealloc(uint8_t* Start);
    void Clear();

    int32_t GetTotalUsedSize() const;
    int32_t GetInterfalFragmentation() const;
  };

  heap_allocator* CreateHeapAllocatorInPlace(void* Base, int32_t Capacity);
}
