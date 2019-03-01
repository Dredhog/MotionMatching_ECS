#pragma once

#include <assert.h>
#include <stdlib.h>

template<typename T, int Capacity>
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

  T*
  Pop()
  {
    if(0 < this->Count)
    {
      --this->Count;
      return &this->Elements[this->Count];
    }
    return NULL;
  }

  void
  Delete(int Index)
  {
    assert(0 < this->Count);
    assert(0 <= Index && Index < this->Count);

    for(int i = Index; i < Count - 1; i++)
    {
      this->Elements[i] = this->Elements[i + 1];
    }
    this->Count--;
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

  T& operator[](int Index)
  {
    assert(0 <= Index && Index < Count);
    return this->Elements[Index];
  }

	int GetCapacity()
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

