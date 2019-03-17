#pragma once

#include <assert.h>
#include <stdlib.h>

template<typename T, int Capacity, typename IndexType = int32_t>
struct fixed_stack
{
  T   Elements[Capacity];
  int Count;

  void
  Push(const T& NewElement)
  {
    assert(this->Count < Capacity);
    assert(0 <= this->Count);

    this->Elements[this->Count++] = NewElement;
  }

  T&
  Pop()
  {
    assert(0 < this->Count);
    --this->Count;
    return *(this->Elements + this->Count);
  }

  void
  Remove(IndexType Index)
  {
    assert(0 < this->Count);
    assert(0 <= Index && Index < this->Count);

    for(IndexType i = Index; i < Count - 1; i++)
    {
      this->Elements[i] = this->Elements[i + 1];
    }
    this->Count--;
  }

  void
  Insert(T NewElement, IndexType Index)
  {
    assert(0 <= this->Count && this->Count < Capacity);
    assert(0 <= Index && Index < this->Count);
    for(IndexType i = this->Count; Index < i; i--)
    {
      this->Elements[i] = this->Elements[i - 1];
    }

    this->Count++;
    this->Elements[Index] = NewElement;
  }

  bool
  Empty()
  {
    return Count == 0;
  }

  bool
  Full()
  {
    return this->Count == Capacity;
  }

  T
  Back()
  {
    assert(0 < this->Count);
    return this->Elements[this->Count - 1];
  }

  void
  Resize(int NewSize)
  {
    assert(NewSize <= Count);
    this->Count = NewSize;
  }

  int
  Clear()
  {
    int TempCount = this->Count;
    this->Count   = 0;
    return TempCount;
  }

  T& operator[](IndexType Index)
  {
    assert(0 <= Index && Index < Count);
    return this->Elements[Index];
  }

  T operator[](IndexType Index) const
  {
    assert(0 <= Index && Index < Count);
    return this->Elements[Index];
  }

  int
  GetCapacity()
  {
    return Capacity;
  }
};

template<typename T, int Capacity>
struct fixed_array
{
  T   Elements[Capacity];
  int Count;

  T*
  Append(const T& NewElement)
  {
    assert(this->Count < Capacity);
    assert(0 <= this->Count);

    this->Elements[this->Count++] = NewElement;
    return &this->Elements[this->Count - 1];
  }

  void
  Clear()
  {
    this->Count = 0;
  }

  void
  HardClear()
  {
    memset(this, 0, sizeof(*this));
  }

  int
  GetCount()
  {
    return this->Count;
  }

  int
  GetCapacity()
  {
    return Capacity;
  }

  T& operator[](int Index)
  {
    assert(0 <= Index && Index < Count);
    return this->Elements[Index];
  }
};

template<typename T, int Capacity>
struct circular_stack
{
  T   m_Entries[Capacity];
  int m_StartIndex;
  int m_Count;

  void
  Push(const T& NewEntry)
  {
    assert(0 <= m_Count && m_Count <= Capacity);

    int WriteIndex = (m_StartIndex + m_Count) % Capacity;
    if(m_Count == Capacity)
    {
      m_StartIndex = (m_StartIndex + 1) % Capacity;
    }
    else
    {
      m_Count++;
    }
    m_Entries[WriteIndex] = NewEntry;
  }

  T
  Pop()
  {
    assert(0 < m_Count);
    int RemovedIndex = (m_StartIndex + (m_Count - 1)) % Capacity;
    m_Count--;
    return m_Entries[RemovedIndex];
  }

  T
  PopBack()
  {
    assert(0 < m_Count);
    int RemovedIndex = m_StartIndex;
    m_StartIndex     = (m_StartIndex + 1) % Capacity;
    m_Count--;

    return m_Entries[RemovedIndex];
  }

  T
  PeekBack()
  {
    assert(0 < m_Count);
    T Result = m_Entries[m_StartIndex];
    return Result;
  }

  T
  Peek()
  {
    assert(0 < m_Count);
    int Index = (m_StartIndex + Capacity + (m_Count - 1)) % Capacity;
    return m_Entries[Index];
  }

  void
  Clear()
  {
    m_StartIndex = 0;
    m_Count      = 0;
  }

  T operator[](int Index) const
  {
    assert(Index < m_Count);
    int SampleIndex = (m_StartIndex + Index) % Capacity;
    return m_Entries[SampleIndex];
  }

  bool
  Full()
  {
    return m_Count == Capacity;
  }

  bool
  Empty()
  {
    return m_Count == 0;
  }

  int
  GetCount()
  {
    return m_Count;
  }

  int
  GetCapacity()
  {
    return Capacity;
  }
};
