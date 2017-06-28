#include "file_io.h"

#include <windows.h>
#include <sys/stat.h>
#include <fcntl.h>

uint32_t
SafeTruncateUint64(uint64_t Value)
{
  assert(Value <= 0xffffffff);
  uint32_t Result = (uint32_t)Value;
  return Result;
}

debug_read_file_result
ReadEntireFile(Memory::stack_allocator* Allocator, const char* FileName)
{
  assert(Allocator);
  debug_read_file_result Result     = {};
  HANDLE FileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if(FileHandle == INVALID_HANDLE_VALUE)
  {
    printf("runtime error: cannot find file: %s\n", FileName);
    return Result;
  }

  //Assuming we only need file size
  LARGE_INTEGER FileSize;
  if(!GetFileSizeEx(FileHandle, &FileSize))
  {
    printf("runtime error: cannot obtain file size for file: %s\n", FileName);
    CloseHandle(FileHandle);
    return Result;
  }
  Result.ContentsSize = SafeTruncateUint64((uint64_t)FileSize.QuadPart);

  Result.Contents = Allocator->Alloc(Result.ContentsSize);
  if(!Result.Contents)
  {
    printf("runtime error: allocation returns null\n");
    Result.ContentsSize = 0;
    CloseHandle(FileHandle);
    return Result;
  }

  uint64_t BytesStoread     = Result.ContentsSize;
  uint8_t* NextByteLocation = (uint8_t*)Result.Contents;
  DWORD BytesRead;
  while(BytesStoread)
  {
    if(!ReadFile(FileHandle, NextByteLocation, BytesStoread, &BytesRead, 0))
    {
      printf("runtime error: went over end while reading file\n");
      Result.Contents     = 0;
      Result.ContentsSize = 0;
      CloseHandle(FileHandle);
      return Result;
    }
    BytesStoread -= (uint64_t)BytesRead;
    NextByteLocation += BytesRead;
  }
  CloseHandle(FileHandle);
  return Result;
}

debug_read_file_result
ReadEntireFile(Memory::heap_allocator* Allocator, const char* FileName)
{
  assert(Allocator);
  debug_read_file_result Result     = {};
  HANDLE FileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if(FileHandle == INVALID_HANDLE_VALUE)
  {
    printf("runtime error: cannot find file: %s\n", FileName);
    return Result;
  }

  //Assuming we only need file size
  LARGE_INTEGER FileSize;
  if(!GetFileSizeEx(FileHandle, &FileSize))
  {
    printf("runtime error: cannot obtain file size for file: %s\n", FileName);
    CloseHandle(FileHandle);
    return Result;
  }
  Result.ContentsSize = SafeTruncateUint64((uint64_t)FileSize.QuadPart);

  Result.Contents = Allocator->Alloc(Result.ContentsSize);
  if(!Result.Contents)
  {
    printf("runtime error: allocation returns null\n");
    Result.ContentsSize = 0;
    CloseHandle(FileHandle);
    return Result;
  }

  uint64_t BytesStoread     = Result.ContentsSize;
  uint8_t* NextByteLocation = (uint8_t*)Result.Contents;
  DWORD BytesRead;
  while(BytesStoread)
  {
    if(!ReadFile(FileHandle, NextByteLocation, BytesStoread, &BytesRead, 0))
    {
      printf("runtime error: went over end while reading file\n");
      Result.Contents     = 0;
      Result.ContentsSize = 0;
      CloseHandle(FileHandle);
      return Result;
    }
    BytesStoread -= (uint64_t)BytesRead;
    NextByteLocation += BytesRead;
  }
  CloseHandle(FileHandle);
  return Result;
}

bool
WriteEntireFile(const char* Filename, uint64_t MemorySize, void* Memory)
{
  HANDLE FileHandle = CreateFile(Filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  if(FileHandle == INVALID_HANDLE_VALUE)
  {
    return false;
  }

  uint64_t BytesToWrite     = MemorySize;
  uint8_t* NextByteLocation = (uint8_t*)Memory;
  DWORD BytesWritten;
  while(BytesToWrite)
  {
    if(!WriteFile(FileHandle, NextByteLocation, BytesToWrite, &BytesWritten, 0))
    {
      CloseHandle(FileHandle);
      return false;
    }
    BytesToWrite -= (uint64_t)BytesWritten;
    NextByteLocation += BytesWritten;
  }
  CloseHandle(FileHandle);
  return true;
}
