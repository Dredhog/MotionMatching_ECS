#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ftw.h>

#include "resource_manager.h"

// TODO: Stat diff

Resource::path* g_PathsPointer;
struct stat*    g_StatsPointer;
int32_t*        g_CountPointer;

int32_t
FileTreeWalk(const char* Path, const struct stat* Stat, int32_t Flag)
{
  if(Flag == FTW_F)
  {
    strcpy(g_PathsPointer[*g_CountPointer].Name, Path);
    memcpy(&g_StatsPointer[*g_CountPointer], Stat, sizeof(struct stat));
    ++*g_CountPointer;
  }
  return 0;
}

int32_t
ReadPaths(Resource::path* Paths, struct stat* Stats, const char* SourceFile)
{
  int32_t PathCount = 0;

  g_PathsPointer = Paths;
  g_StatsPointer = Stats;
  g_CountPointer = &PathCount;

  char*   SingleLine = NULL;
  size_t  PathLength = 0;
  ssize_t ReadReturnValue;
  int32_t FTWReturnValue;
  FILE*   FilePointer = fopen(SourceFile, "r");

  while((ReadReturnValue = getline(&SingleLine, &PathLength, FilePointer)) != -1)
  {
    SingleLine[strlen(SingleLine) - 1] = '\0';

    FTWReturnValue = ftw(SingleLine, FileTreeWalk, 100);
    if(FTWReturnValue == -1)
    {
      printf("Error occured when trying to access path %s\n", SingleLine);
    }
    PathLength = 0;
    free(SingleLine);
    SingleLine = NULL;
  }

  fclose(FilePointer);
  return PathCount;
}
