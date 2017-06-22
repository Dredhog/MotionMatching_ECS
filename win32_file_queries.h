#pragma once

asset_diff* g_DiffPaths;
path*       g_Paths;
file_stat*  g_Stats;
int32_t*    g_ElementCount;
int32_t*    g_DiffCount;
const char* g_Extension;
bool        g_WasTraversed[1000];

int32_t GetPathIndex(path* PathArray, int32_t PathCount, const char* Path);

int32_t
FileNameIndex(const char* Path)
{
  int Index  = 0;
  int Length = strlen(Path);
  for(int i = 0; i < Length; i++)
  {
    if(Path[i] == '\\')
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
CheckFile(const WIN32_FIND_DATA* Stat)
{
  char* Path = Stat->cFileName;

  if((Stat->dwFileAttributes & FILE_ATTRIBUTE_DIRECROTY) && (Path[FileNameIndex(Path)] == '.'))
  {
    return 0;
  }

  if(Stat->dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
  {
    if(Path[FileNameIndex(Path)] == '.')
    {
      return 0;
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
        assert(0 && "Extension length is less than or equal to zero! Did you "
                    "mean to type NULL?");
        return 0;
      }
    }

    assert(*g_ElementCount < RESOURCE_MAX_COUNT);
    assert(*g_DiffCount < 2 * RESOURCE_MAX_COUNT);

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

int32_t
ReadPaths(asset_diff* DiffPaths, path* Paths, file_stat* Stats, int32_t* ElementCount,
          const char* StartPath, const char* Extension)
{
  int32_t DiffCount = 0;

  g_DiffPaths    = DiffPaths;
  g_Paths        = Paths;
  g_Stats        = Stats;
  g_ElementCount = ElementCount;
  g_DiffCount    = &DiffCount;
  g_Extension    = Extension;

  int Length = sizeof(g_WasTraversed) / sizeof(g_WasTraversed[0]);
  for(int i = 0; i < Length; i++)
  {
    g_WasTraversed[i] = false;
  }

  WIN32_FIND_DATA FileData;
  HANDLE          FileHandle = FindFirstFile(StartPath, &FileData);

  while(FileHandle != INVALID_HANDLE_VALUE)
  {
    CheckFile(&FileData);
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
