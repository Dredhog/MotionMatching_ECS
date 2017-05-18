#pragma once

#include "text.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>

// TODO: Stat diff

enum diff_state
{
  DIFF_Added,
  DIFF_Modified,
  DIFF_Deleted,

  DIFF_EnumCount
};

struct path
{
  char Name[TEXT_LINE_MAX_LENGTH];
};

struct path_diff
{
  diff_state State;
  path       Path;
};

path_diff*   g_DiffPaths;
path*        g_Paths;
struct stat* g_Stats;
int32_t*     g_ElementCount;
int32_t*     g_DiffCount;
const char*  g_Extension;
bool         g_WasTraversed[1000];

int32_t
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

int32_t
CheckFile(const char* Path, const struct stat* Stat, int32_t Flag, struct FTW* FileName)
{
  if((Flag == FTW_D) && (Path[FileName->base] == '.'))
  {
    return FTW_SKIP_SUBTREE;
  }
  else if((Flag == FTW_F) && (Path[FileName->base] == '.'))
  {
    return FTW_CONTINUE;
  }

  if(Flag == FTW_F)
  {
    if(strlen(Path) > TEXT_LINE_MAX_LENGTH)
    {
      printf("%lu - %s\n", strlen(Path), Path);
    }
    assert(strlen(Path) <= TEXT_LINE_MAX_LENGTH);

    bool ExtensionsMatch = false;

    if(g_Extension == NULL)
    {
      ExtensionsMatch = true;
    }
    else
    {
      if(strlen(Path) < strlen(g_Extension))
      {
        printf("%lu - %s\n", strlen(Path), Path);
      }
      assert(strlen(Path) >= strlen(g_Extension));

      if(strlen(g_Extension) > 0)
      {
        size_t ExtensionStartIndex = strlen(Path) - strlen(g_Extension) - 1;
        int    i                   = 0;

        if(Path[ExtensionStartIndex] == '.')
        {
          ++ExtensionStartIndex;
          while(Path[ExtensionStartIndex] == g_Extension[i])
          {
            ExtensionStartIndex++;
            i++;
            if((Path[ExtensionStartIndex] == '\0') && (g_Extension[i] == '\0'))
            {
              ExtensionsMatch = true;
              break;
            }
          }
        }
      }
      else
      {
        assert(0 && "Extension length is less than or equal to zero! Did you mean to type NULL?");
      }
    }

    if(ExtensionsMatch)
    {
      int PathIndex = GetPathIndex(g_Paths, *g_ElementCount, Path);
      if(PathIndex == -1)
      {
        strcpy(g_Paths[*g_ElementCount].Name, Path);
        g_Stats[*g_ElementCount]        = *Stat;
        g_WasTraversed[*g_ElementCount] = true;
        ++(*g_ElementCount);

        strcpy(g_DiffPaths[*g_DiffCount].Path.Name, Path);
        g_DiffPaths[*g_DiffCount].State = DIFF_Added;
        ++(*g_DiffCount);
      }
      else
      {
        if(difftime(g_Stats->st_mtime, Stat->st_mtime) != 0.0f)
        {
          g_Stats[PathIndex] = *Stat;

          strcpy(g_DiffPaths[*g_DiffCount].Path.Name, Path);
          g_DiffPaths[*g_DiffCount].State = DIFF_Modified;
          ++(*g_DiffCount);
        }
        g_WasTraversed[PathIndex] = true;
      }
    }
  }
  return 0;
}

int32_t
ReadPaths(path_diff* DiffPaths, path* Paths, struct stat* Stats, int32_t* ElementCount,
          const char* StartPath, const char* Extension)
{
  int32_t DiffCount = 0;

  g_DiffPaths    = DiffPaths;
  g_Paths        = Paths;
  g_Stats        = Stats;
  g_ElementCount = ElementCount;
  g_DiffCount    = &DiffCount;
  g_Extension    = Extension;

  assert(*ElementCount < 1000);

  int32_t FTWReturnValue;
  FTWReturnValue = nftw(StartPath, CheckFile, 100, FTW_ACTIONRETVAL);
  if(FTWReturnValue == -1)
  {
    printf("Error occured when trying to access path %s\n", StartPath);
  }

  for(int i = 0; i < DiffCount; i++)
  {
    if(DiffPaths[i].State == DIFF_Added)
    {
      printf("Added - ");
    }
    if(DiffPaths[i].State == DIFF_Modified)
    {
      printf("Modified - ");
    }
    if(DiffPaths[i].State == DIFF_Deleted)
    {
      printf("Deleted - ");
    }
    printf("%lu - %s\n", strlen(DiffPaths[i].Path.Name), DiffPaths[i].Path.Name);
  }

  for(int i = 0; i < *ElementCount; i++)
  {
    if(!g_WasTraversed[i])
    {
      strcpy(Paths[i].Name, Paths[(*ElementCount) - 1].Name);
      Stats[i] = Stats[(*ElementCount) - 1];
      --(*ElementCount);

      strcpy(DiffPaths[DiffCount].Path.Name, Paths[i].Name);
      DiffPaths[DiffCount].State = DIFF_Deleted;
      ++DiffCount;
    }
  }

  return DiffCount;
}
