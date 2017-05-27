#include "file_io.h"

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
  Result.ContentsSize = SafeTruncateUint64((uint64_t)FileStatus.st_size);

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
    BytesStoread -= (uint64_t)BytesRead;
    NextByteLocation += BytesRead;
  }
  close(FileHandle);
  return Result;
}

bool
WriteEntireFile(const char* Filename, uint64_t MemorySize, void* Memory)
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
    BytesToWrite -= (uint64_t)BytesWritten;
    NextByteLocation += BytesWritten;
  }
  close(FileHandle);
  return true;
}
