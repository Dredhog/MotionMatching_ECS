#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ftw.h>
#include <time.h>

#include "../file_io.h"
#include "../file_queries.h"

debug_read_file_result
Platform::ReadEntireFile(Memory::stack_allocator* Allocator, const char* FileName)
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

debug_read_file_result
Platform::ReadEntireFile(Memory::heap_allocator* Allocator, const char* FileName)
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
Platform::WriteEntireFile(const char* Filename, uint64_t MemorySize, void* Memory)
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

asset_diff* g_DiffPaths;
path*       g_Paths;
file_stat*  g_Stats;
int32_t*    g_ElementCount;
int32_t*    g_DiffCount;
const char* g_Extension;
bool        g_WasTraversed[1000];
int32_t     g_MAX_ALLOWED_ELEMENT_COUNT;

int32_t
CheckFile(const char* Path, const struct stat* Stat, int32_t Flag, struct FTW* FileName)
{
  if((Flag == FTW_D) && (Path[FileName->base] == '.'))
  {
    return FTW_SKIP_SUBTREE;
  }

  if(Flag == FTW_F)
  {
    if(Path[FileName->base] == '.')
    {
      return FTW_CONTINUE;
    }

    size_t PathLength = strlen(Path);
    if(PathLength > PATH_MAX_LENGTH)
    {
      printf("Cannot fit: length: %lu, %s\n", strlen(Path), Path);
    }
    assert(PathLength <= PATH_MAX_LENGTH);

    if(g_Extension)
    {
      size_t ExtensionLength = strlen(g_Extension);
      if(0 < ExtensionLength)
      {
        if(PathLength <= ExtensionLength + 1)
        {
          return 0;
        }

        size_t ExtensionStartIndex = PathLength - ExtensionLength;
        if(!(Path[ExtensionStartIndex - 1] == '.' && strcmp(&Path[ExtensionStartIndex], g_Extension) == 0))
        {
          return 0;
        }
      }
      else
      {
        assert(0 && "Extension length is less than or equal to zero! Did you mean to type NULL?");
        return 0;
      }
    }

    assert(*g_ElementCount < g_MAX_ALLOWED_ELEMENT_COUNT);
    assert(*g_DiffCount < 2 * g_MAX_ALLOWED_ELEMENT_COUNT);

    int PathIndex = GetPathIndex(g_Paths, *g_ElementCount, Path);
    if(PathIndex == -1)
    {
      strcpy(g_Paths[*g_ElementCount].Name, Path);
      g_Stats[*g_ElementCount].LastTimeModified = Stat->st_mtime;
      g_WasTraversed[*g_ElementCount]           = true;
      ++(*g_ElementCount);

      strcpy(g_DiffPaths[*g_DiffCount].Path.Name, Path);
      g_DiffPaths[*g_DiffCount].Type = DIFF_Added;
      ++(*g_DiffCount);
    }
    else
    {
      double TimeDiff = difftime(Stat->st_mtime, g_Stats[PathIndex].LastTimeModified);
      if(TimeDiff > 0)
      {
        g_Stats[PathIndex].LastTimeModified = Stat->st_mtime;

        strcpy(g_DiffPaths[*g_DiffCount].Path.Name, Path);
        g_DiffPaths[*g_DiffCount].Type = DIFF_Modified;
        ++(*g_DiffCount);
      }
      g_WasTraversed[PathIndex] = true;
    }
  }
  return 0;
}

int32_t
Platform::ReadPaths(asset_diff* DiffPaths, path* Paths, file_stat* Stats, int32_t MAX_ELEMENT_COUNT, int32_t* ElementCount, const char* StartPath, const char* Extension)
{
  int32_t DiffCount = 0;

  g_DiffPaths                 = DiffPaths;
  g_Paths                     = Paths;
  g_Stats                     = Stats;
  g_ElementCount              = ElementCount;
  g_DiffCount                 = &DiffCount;
  g_Extension                 = Extension;
  g_MAX_ALLOWED_ELEMENT_COUNT = MAX_ELEMENT_COUNT;

  int Size = sizeof(g_WasTraversed) / sizeof(g_WasTraversed[0]);
  for(int i = 0; i < Size; i++)
  {
    g_WasTraversed[i] = false;
  }

  if(nftw(StartPath, CheckFile, 100, FTW_ACTIONRETVAL) != -1)
  {
    for(int i = 0; i < *ElementCount; i++)
    {
      if(!g_WasTraversed[i])
      {
        --(*ElementCount);
        Paths[i] = Paths[*ElementCount];
        Stats[i] = Stats[*ElementCount];

        DiffPaths[DiffCount].Path = Paths[i];
        DiffPaths[DiffCount].Type = DIFF_Deleted;
        ++DiffCount;
      }
    }
  }
  else
  {
    printf("Error occured when trying to access path %s\n", StartPath);
  }

  return DiffCount;
}
