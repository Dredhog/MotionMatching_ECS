#pragma once

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "stack_allocator.h"

struct debug_read_file_result
{
  void*    Contents;
  uint32_t ContentsSize;
};

#if 0
#define PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char* Filename)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void name(debug_read_file_result FileHandle)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

#define PLATFORM_WRITE_ENTIRE_FILE(name)                                                           \
  int32_t name(char* Filename, uint64_t MemorySize, void* Memory)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);
#endif

uint32_t
SafeTruncateUint64(uint64_t Value)
{
  assert(Value <= 0xffffffff);
  uint32_t Result = (uint32_t)Value;
  return Result;
}

debug_read_file_result
ReadEntireFile(Memory::stack_allocator *Allocator, const char* FileName)
{
	assert(Allocator);
  debug_read_file_result Result     = {};
  int                    FileHandle = open(FileName, O_RDONLY);
  if(FileHandle == -1)
  {
		printf("runtime error: cannot find file: %s\n", FileName);
    return Result;
  }

  struct stat FileStatus;
  if(fstat(FileHandle, &FileStatus) == -1)
  {
		printf("runtime error: cannot obtain status for file: %s\n", FileName);
    close(FileHandle);
    return Result;
  }
  Result.ContentsSize = SafeTruncateUint64(FileStatus.st_size);

  Result.Contents = Allocator->Alloc(Result.ContentsSize);
  if(!Result.Contents)
  {
		printf("runtime error: allocation returns null\n");
    Result.ContentsSize = 0;
    close(FileHandle);
    return Result;
  }

  uint64_t BytesStoread     = Result.ContentsSize;
  uint8_t* NextByteLocation = (uint8_t*)Result.Contents;
  while(BytesStoread)
  {
    int64_t BytesRead = read(FileHandle, NextByteLocation, BytesStoread);
    if(BytesRead == -1)
    {
			printf("runtime error: went over end while reading file\n");
      Result.Contents     = 0;
      Result.ContentsSize = 0;
      close(FileHandle);
      return Result;
    }
    BytesStoread -= BytesRead;
    NextByteLocation += BytesRead;
  }
  close(FileHandle);
  return Result;
}

bool
WriteEntireFile(char* Filename, uint64_t MemorySize, void* Memory)
{
  int FileHandle = open(Filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if(FileHandle == -1)
  {
    return false;
  }

  uint64_t BytesToWrite     = MemorySize;
  uint8_t* NextByteLocation = (uint8_t*)Memory;
  while(BytesToWrite)
  {
    int64_t BytesWritten = write(FileHandle, NextByteLocation, BytesToWrite);
    if(BytesWritten == -1)
    {
      close(FileHandle);
      return false;
    }
    BytesToWrite -= BytesWritten;
    NextByteLocation += BytesWritten;
  }
  close(FileHandle);
  return true;
}
