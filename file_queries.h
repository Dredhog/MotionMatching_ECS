#pragma once

#include "resource_manager.h"

int32_t ReadPaths(asset_diff* DiffPaths, path* Paths, file_stat* Stats, int32_t* ElementCount,
                  const char* StartPath, const char* Extension);
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
