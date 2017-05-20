#pragma once

namespace Resource
{
  template<typename T, int MAX_PATH_COUNT>
  class resource_hash_table // Disregard the current implementation (currently array)
  {
    path Paths[MAX_PATH_COUNT];
    T    Assets[MAX_PATH_COUNT];

  public:
    void
    Set(rid RID, const T Asset, const char* Path)
    {
      assert(0 < RID.Value && RID.Value <= TEXT_LINE_MAX_LENGTH);

      this->Assets[RID.Value - 1] = Asset;

      if(Path)
      {
        size_t PathLength = strlen(Path);
        assert(PathLength < TEXT_LINE_MAX_LENGTH);
        strcpy(this->Paths[RID.Value - 1].Name, Path);
      }
      else
      {
        this->Paths[RID.Value - 1] = {};
      }
    }

    bool
    Get(rid RID, T* Asset, char** Path)
    {
      assert(0 < RID.Value && RID.Value <= TEXT_LINE_MAX_LENGTH);

      if(Asset)
      {
        *Asset = this->Assets[RID.Value - 1];
      }

      if(Path)
      {
        *Path = this->Paths[RID.Value - 1].Name;
      }
      return true;
    }

    bool
    GetPathRID(rid* RID, const char* Path)
    {
      if(!Path)
      {
        assert(Path && "hash table error: ueried path is NULL");
        return false;
      }
      for(int i = 0; i < MAX_PATH_COUNT; i++)
      {
        if(strcmp(Path, this->Paths[i].Name) == 0)
        {
          RID->Value = { i + 1 };
          return true;
        }
      }
      return false;
    }

    bool
    NewRID(rid* RID)
    {
      for(int i = 0; i < MAX_PATH_COUNT; i++)
      {
        if(this->Paths[i].Name[0] == '\0' && !this->Assets[i])
        {
          rid NewRID = { i + 1 };
          *RID       = NewRID;
          return true;
        }
      }
      assert(0 && "hash table error: unable to find new rid");
      return false;
    }
  };
}

