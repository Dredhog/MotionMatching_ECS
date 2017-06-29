#pragma once

#include <time.h>
#include <string.h>
#include <stdint.h>

#define PATH_MAX_LENGTH 64

enum asset_diff_type
{
  DIFF_Added,
  DIFF_Modified,
  DIFF_Deleted,

  DIFF_EnumCount
};

struct path
{
  char Name[PATH_MAX_LENGTH];
};

struct asset_diff
{
  uint32_t Type;
  path     Path;
};

struct file_stat
{
  time_t LastTimeModified;
};

namespace Platform
{
  int32_t ReadPaths(asset_diff* DiffPaths, path* Paths, file_stat* Stats, int32_t MaxElementCount, int32_t* ElementCount, const char* StartPath, const char* Extension);
}

inline int32_t
GetPathIndex(path* PathArray, int32_t PathCount, const char* Path)
{
  int Index = -1;
  for(int i = 0; i < PathCount; i++)
  {
    if(strcmp(PathArray[i].Name, Path) == 0)
    {
      Index = i;
      break;
    }
  }
  return Index;
}
