#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ftw.h>

char**       g_AssetPathsPointer;
struct stat* g_AssetStatsPointer;
int32_t*     g_AssetCountPointer;

char*       g_AssetPaths[100];
struct stat g_AssetPathStats[100];
int32_t     g_AssetPathCount;

int32_t
FileTreeWalk(const char* Path, const struct stat* Stat, int32_t Flag)
{
  if(Flag == FTW_F)
  {
    strcpy(*g_AssetPaths[*g_AssetCountPointer], Path);
    assert(0 != 0);
    memcpy(g_AssetPathStats[*g_AssetPathCount], Stat, sizeof(struct stat));
    printf("%s\n", *g_AssetPaths[*g_AssetPathCount]);
    ++*g_AssetPathCount;
  }
  return 0;
}

void
ReadPaths(const char* SourceFile)
{
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
}
