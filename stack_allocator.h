#pragma once

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <cassert>
#include <stdio.h>

#define Kibibytes(Count) (1024 * (Count))
#define Mibibytes(Count) (1024 * Kibibytes(Count))
#define Gibibytes(Count) (1024 * Mibibytes(Count))

#define PushStruct(Allocator, Type) (Type*)(Allocator)->Alloc(sizeof(Type))

#define PushArray(Allocator, Count, Type) (Type*)(Allocator)->Alloc(sizeof(Type) * (Count))
namespace Memory
{
  struct marker
  {
    uint8_t* Address;
  };

  class stack_allocator
  {
    uint8_t* m_Base;
    uint8_t* m_Top;

    int32_t m_AllocCount;
    int32_t m_CapacityBytes;

  public:
    void Create(void* Base, uint32_t Capacity);

    uint8_t* Alloc(uint32_t SizeBytes);
    uint8_t* AlignedAlloc(uint32_t SizeBytes, uint32_t Alignment);

    void FreeToMarker(marker Marker);
    void Clear();

    marker GetMarker() const;
    size_t GetByteCountAboveMarker(marker Marker) const;
    int32_t GetAllocCount() const;
    int32_t GetUsedSize() const;
    int32_t GetCapacity() const;

    inline void
    PrintStats()
    {
      printf("STACK ALLOCATOR INFO\n");
      printf("Allocation count: %d\n", GetAllocCount());
      printf("Memory available: %dB\n", GetCapacity());
      printf("Memory used: %dB\n", GetUsedSize());
    }
  };

  stack_allocator* CreateStackAllocatorInPlace(void* Base, uint32_t Capacity);
  inline uint32_t
  SafeTruncate_size_t_To_uint32_t(size_t Value)
  {
    assert(Value < SIZE_MAX);
    return (uint32_t)Value;
  }
}
