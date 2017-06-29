#include <windows.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "file_io.h"
#include "file_queries.h"

debug_read_file_result
Platform::ReadEntireFile(Memory::stack_allocator* Allocator, const char* FileName)
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
Platform::ReadEntireFile(Memory::heap_allocator* Allocator, const char* FileName)
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
Platform::WriteEntireFile(const char* Filename, uint64_t MemorySize, void* Memory)
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

asset_diff* g_DiffPaths;
path*       g_Paths;
file_stat*  g_Stats;
int32_t*    g_ElementCount;
int32_t*    g_DiffCount;
const char* g_Extension;
bool        g_WasTraversed[1000];
int32_t     g_MAX_ALLOWED_ELEMENT_COUNT;

int32_t
FileNameIndex(const char* Path)
{
  int Index  = 0;
  int Length = strlen(Path);
  for(int i = 0; i < Length; i++)
  {
    if(Path[i] == '/')
    {
      Index = i;
    }
  }

  return Index + 1;
}

time_t
FileTimeToTime(FILETIME FileTime)
{
  ULARGE_INTEGER FullValue;
  FullValue.LowPart  = FileTime.dwLowDateTime;
  FullValue.HighPart = FileTime.dwHighDateTime;

  return FullValue.QuadPart / 10000000 - 116444736000000000;
}

int32_t
CheckFile(const WIN32_FIND_DATA* Stat, const char* Path)
{
  if(Stat->cFileName[0] == '.')
  {
    return 0;
  }

  if(Stat->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    return 1;
  }

  if(!(Stat->dwFileAttributes & (FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_VIRTUAL)))
  {
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
        if(!(Path[ExtensionStartIndex - 1] == '.' &&
             strcmp(&Path[ExtensionStartIndex], g_Extension) == 0))
        {
          return 0;
        }
      }
      else
      {
        assert(0 && "Extension length is less than or equal to zero! Did you "
                    "mean to type NULL?");
        return 0;
      }
    }

    assert(*g_ElementCount < g_MAX_ALLOWED_ELEMENT_COUNT);
    assert(*g_DiffCount < 2 * g_MAX_ALLOWED_ELEMENT_COUNT);

    int PathIndex = GetPathIndex(g_Paths, *g_ElementCount, Path);
    if(PathIndex == -1)
    {
      strcpy(g_Paths[*g_ElementCount].Name, Path);
      g_Stats[*g_ElementCount].LastTimeModified = FileTimeToTime(Stat->ftLastWriteTime);
      g_WasTraversed[*g_ElementCount]           = true;
      ++(*g_ElementCount);

      strcpy(g_DiffPaths[*g_DiffCount].Path.Name, Path);
      g_DiffPaths[*g_DiffCount].Type = DIFF_Added;
      ++(*g_DiffCount);
    }
    else
    {
      double TimeDiff =
        difftime(FileTimeToTime(Stat->ftLastWriteTime), g_Stats[PathIndex].LastTimeModified);
      if(TimeDiff > 0)
      {
        g_Stats[PathIndex].LastTimeModified = FileTimeToTime(Stat->ftLastWriteTime);

        strcpy(g_DiffPaths[*g_DiffCount].Path.Name, Path);
        g_DiffPaths[*g_DiffCount].Type = DIFF_Modified;
        ++(*g_DiffCount);
      }
      g_WasTraversed[PathIndex] = true;
    }
  }
  return 0;
}

void
FindFilesRecursively(const char* StartPath)
{
  char SubPath[100];
  strcpy(SubPath, StartPath);
  strcat(SubPath, "/*");

  WIN32_FIND_DATA FileData;
  HANDLE          FileHandle = FindFirstFile(SubPath, &FileData);

  while(FileHandle != INVALID_HANDLE_VALUE)
  {
    char FilePath[100];
    strcpy(FilePath, StartPath);
    strcat(FilePath, "/");
    strcat(FilePath, FileData.cFileName);

    if(CheckFile(&FileData, FilePath))
    {
      FindFilesRecursively(FilePath);
    }

    if(!FindNextFile(FileHandle, &FileData))
    {
      break;
    }
  }

  if(FileHandle != INVALID_HANDLE_VALUE)
  {
    FindClose(FileHandle);
  }
}

int32_t
Platform::ReadPaths(asset_diff* DiffPaths, path* Paths, file_stat* Stats, int32_t MAX_ELEMENT_COUNT, int32_t* ElementCount, const char* StartPath, const char* Extension)
{
  int32_t DiffCount = 0;

  g_DiffPaths    = DiffPaths;
  g_Paths        = Paths;
  g_Stats        = Stats;
  g_ElementCount = ElementCount;
  g_DiffCount    = &DiffCount;
  g_Extension    = Extension;
  g_MAX_ALLOWED_ELEMENT_COUNT = MAX_ELEMENT_COUNT;

  int Length = sizeof(g_WasTraversed) / sizeof(g_WasTraversed[0]);
  for(int i = 0; i < Length; i++)
  {
    g_WasTraversed[i] = false;
  }

  WIN32_FIND_DATA FileData;
  HANDLE          FileHandle = FindFirstFile(StartPath, &FileData);

  while(FileHandle != INVALID_HANDLE_VALUE)
  {
    if(CheckFile(&FileData, StartPath))
    {
      FindFilesRecursively(StartPath);
    }

    if(!FindNextFile(FileHandle, &FileData))
    {
      break;
    }
  }

  if(FileHandle != INVALID_HANDLE_VALUE)
  {
    FindClose(FileHandle);

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
