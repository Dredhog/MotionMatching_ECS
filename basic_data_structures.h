#pragma once

#include <assert.h>
#include <stdlib.h>
#include "string.h"

template<typename T, typename IndexType = int32_t>
struct array_handle
{
  T*  Elements;
  int Count;

  bool
  IsValid() const
  {
    return this->Elements != NULL && this->Count != 0;
  }

  void
  Init(T* Elements, IndexType Count)
  {
    assert(Elements);
    assert(0 < Count);
    this->Elements = Elements;
    this->Count    = Count;
  }

  void
  HardClear()
  {
    memset(this->Elements, 0, this->Count * sizeof(T));
    this->Count = 0;
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
};

template<typename T, typename IndexType = int32_t>
struct stack_handle
{
  T*      Elements;
  int32_t Capacity;
  int32_t Count;

  void
  Init(T* Elements, IndexType Count, IndexType Capacity)
  {
    assert(Elements);
    assert(Count <= Capacity);
    this->Elements = Elements;
    this->Count    = Count;
    this->Capacity = Capacity;
  }

  void
  Expand(IndexType ExtraCount)
  {
    assert(this->Count + ExtraCount <= this->Capacity);
    this->Count += ExtraCount;
  }

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
  Empty() const 
  {
    return Count == 0;
  }

  bool
  Full() const 
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
    assert(NewSize <= Capacity);
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

  array_handle<T, IndexType>
  GetArrayHandle() const
  {
    array_handle<T, IndexType> NewArrayHandle;
    NewArrayHandle.Init(Elements, Count);
    return NewArrayHandle;
  }
};

template<typename T, typename IndexType = int32_t>
stack_handle<T, IndexType>
CreateStackHandle(T* Elements, int32_t Count, int32_t Capacity)
{
  stack_handle<T, IndexType> NewStackHandle;
  NewStackHandle.Elements = Elements;
  NewStackHandle.Count    = Count;
  NewStackHandle.Capacity = Capacity;

  return NewStackHandle;
}

template<typename T, typename IndexType = int32_t>
array_handle<T, IndexType>
CreateArrayHandle(T* Elements, IndexType Count)
{
  array_handle<T, IndexType> NewHandle;
  NewHandle.Init(Elements, Count);
  return NewHandle;
}

template<typename T, int Capacity, typename IndexType = int32_t>
struct fixed_stack
{
  T   Elements[Capacity];
  int Count;

  fixed_stack() { this->Count = 0; }

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
  Empty() const
  {
    return Count == 0;
  }

  bool
  Full() const
  {
    return this->Count == Capacity;
  }

  T
  Peek() const
  {
    assert(0 < this->Count);
    return this->Elements[this->Count - 1];
  }

  void
  Resize(int NewSize)
  {
    assert(NewSize <= Capacity);
    this->Count = NewSize;
  }

  int
  Clear()
  {
    int TempCount = this->Count;
    this->Count   = 0;
    return TempCount;
  }

  int
  HardClear()
  {
    int TempCount = this->Count;
    memset(this->Elements, 0, this->Count * sizeof(T));
    this->Count = 0;
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

  array_handle<T, IndexType>
  GetArrayHandle()
  {
    array_handle<T, IndexType> NewHandle = CreateArrayHandle(this->Elements, this->Count);
    return NewHandle;
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
  int Count;

  void
  Push(const T& NewEntry)
  {
    assert(0 <= Count && Count <= Capacity);

    int WriteIndex = (m_StartIndex + Count) % Capacity;
    if(Count == Capacity)
    {
      m_StartIndex = (m_StartIndex + 1) % Capacity;
    }
    else
    {
      Count++;
    }
    m_Entries[WriteIndex] = NewEntry;
  }

  T
  Pop()
  {
    assert(0 < Count);
    int RemovedIndex = (m_StartIndex + (Count - 1)) % Capacity;
    Count--;
    return m_Entries[RemovedIndex];
  }

  T
  PopBack()
  {
    assert(0 < Count);
    int RemovedIndex = m_StartIndex;
    m_StartIndex     = (m_StartIndex + 1) % Capacity;
    Count--;

    return m_Entries[RemovedIndex];
  }

  T
  PeekBack() const
  {
    assert(0 < Count);
    T Result = m_Entries[m_StartIndex];
    return Result;
  }

  T
  Peek() const
  {
    assert(0 < Count);
    int Index = (m_StartIndex + Capacity + (Count - 1)) % Capacity;
    return m_Entries[Index];
  }

  T&
  Peek()
  {
    assert(0 < Count);
    int Index = (m_StartIndex + Capacity + (Count - 1)) % Capacity;
    return m_Entries[Index];
  }


  void
  Clear()
  {
    m_StartIndex = 0;
    Count      = 0;
  }

  T operator[](int Index) const
  {
    assert(Index < Count);
    int SampleIndex = (m_StartIndex + Index) % Capacity;
    return m_Entries[SampleIndex];
  }

  bool
  Full() const
  {
    return Count == Capacity;
  }

  bool
  Empty() const
  {
    return Count == 0;
  }

  int
  GetCount() const
  {
    return Count;
  }

  int
  GetCapacity() const
  {
    return Capacity;
  }
};
