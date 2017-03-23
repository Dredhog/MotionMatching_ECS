#include "stack_allocator.h"

#include <cassert>

namespace Memory
{
  void
  Memory::stack_allocator::Create(void* Base, uint32_t Capacity)
  {
    assert(Base);
    assert(Capacity > 0);

    m_AllocCount    = 0;
    m_Base          = (uint8_t*)Base;
    m_Top           = (uint8_t*)Base;
    m_CapacityBytes = Capacity;
  }

  stack_allocator*
  CreateStackAllocatorInPlace(void* Base, uint32_t Capacity)
  {
    assert(Base);
    assert(Capacity > sizeof(stack_allocator));

    stack_allocator* Result     = (stack_allocator*)Base;
    uint8_t*         ActualBase = (uint8_t*)Base + sizeof(stack_allocator);
    Result->Create(ActualBase, Capacity - (uint32_t)sizeof(stack_allocator));

    return Result;
  }

  uint8_t*
  Memory::stack_allocator::Alloc(uint32_t SizeBytes)
  {
    assert(((m_Top + SizeBytes) - m_Base) <= m_CapacityBytes);

    uint8_t* Result = m_Top;
    m_Top += SizeBytes;
    ++m_AllocCount;

    return Result;
  }

  uint8_t*
  Memory::stack_allocator::AlignedAlloc(uint32_t SizeBytes, uint32_t Alignment)
  {
    assert((m_Top - m_Base) < m_CapacityBytes);
    assert(Alignment > 1);

    uint32_t Offset = (Alignment - (uint32_t)((uint64_t)m_Top & ((uint64_t)Alignment - 1)));
    uint8_t* UnalignedAddress = Alloc(SizeBytes + Offset);

    if(Offset == Alignment)
    {
      return UnalignedAddress;
    }

    return UnalignedAddress + Offset;
  }

  void
  Memory::stack_allocator::FreeToMarker(marker Marker)
  {
    assert(m_Top >= Marker.Address);
    m_Top = Marker.Address;
  }

  void
  Memory::stack_allocator::Clear()
  {
    m_Top        = m_Base;
    m_AllocCount = 0;
  }

  marker
  Memory::stack_allocator::GetMarker() const
  {
    return marker{ m_Top };
  }

  int32_t
  Memory::stack_allocator::GetAllocCount() const
  {
    return m_AllocCount;
  }

  int32_t
  Memory::stack_allocator::GetUsedSize() const
  {
    assert(m_Base <= m_Top);
    if(m_Top == m_Base)
    {
      return 0;
    }
    return (int32_t)(m_Top - m_Base);
  }

  int32_t
  Memory::stack_allocator::GetCapacity() const
  {
    return m_CapacityBytes;
  }
}
