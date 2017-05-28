#pragma once

#include <unistd.h>

#include "stack_alloc.h"
#include "heap_alloc.h"

struct debug_read_file_result
{
  void*    Contents;
  uint32_t ContentsSize;
};

uint32_t SafeTruncateUint64(uint64_t Value);

debug_read_file_result ReadEntireFile(Memory::stack_allocator* Allocator, const char* FileName);
debug_read_file_result ReadEntireFile(Memory::heap_allocator* Allocator, const char* FileName);
bool                   WriteEntireFile(const char* Filename, uint64_t MemorySize, void* Memory);
