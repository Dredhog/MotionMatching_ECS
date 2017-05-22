#pragma once

asset_diff*  g_DiffPaths;
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

  if(Flag == FTW_F)
  {
    if(Path[FileName->base] == '.')
    {
      return FTW_CONTINUE;
    }

    size_t PathLength = strlen(Path);
    if(PathLength > TEXT_LINE_MAX_LENGTH)
    {
      printf("Cannot fit: length: %lu, %s\n", strlen(Path), Path);
    }
    assert(PathLength <= TEXT_LINE_MAX_LENGTH);

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
        assert(0 && "Extension length is less than or equal to zero! Did you mean to type NULL?");
        return 0;
      }
    }

    assert(*g_ElementCount < RESOURCE_MAX_COUNT);
    assert(*g_DiffCount < 2 * RESOURCE_MAX_COUNT);

    int PathIndex = GetPathIndex(g_Paths, *g_ElementCount, Path);
    if(PathIndex == -1)
    {
      strcpy(g_Paths[*g_ElementCount].Name, Path);
      g_Stats[*g_ElementCount]        = *Stat;
      g_WasTraversed[*g_ElementCount] = true;
      ++(*g_ElementCount);

      strcpy(g_DiffPaths[*g_DiffCount].Path.Name, Path);
      g_DiffPaths[*g_DiffCount].Type = DIFF_Added;
      ++(*g_DiffCount);
    }
    else
    {
      double TimeDiff = difftime(g_Stats[PathIndex].st_mtime, Stat->st_mtime);
      if(TimeDiff > 0)
      {
        printf("%f\n", TimeDiff);
        g_Stats[PathIndex] = *Stat;

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
ReadPaths(asset_diff* DiffPaths, path* Paths, struct stat* Stats, int32_t* ElementCount,
          const char* StartPath, const char* Extension)
{
  int32_t DiffCount = 0;

  g_DiffPaths    = DiffPaths;
  g_Paths        = Paths;
  g_Stats        = Stats;
  g_ElementCount = ElementCount;
  g_DiffCount    = &DiffCount;
  g_Extension    = Extension;
  for(int i = 0; i < sizeof(g_WasTraversed) / sizeof(g_WasTraversed[0]); i++)
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
