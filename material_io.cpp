#include "material_io.h"
#include "file_io.h"

#include <stdio.h>
#include <string.h>

int32_t
GetLine(char** Line, int32_t* LineLength, char* CharArray, int32_t ArraySize, int32_t* CharCounter)
{
  *Line       = &CharArray[*CharCounter];
  *LineLength = 0;
  while((CharArray[*CharCounter] != '\n') /*&& (CharArray[*CharCounter] != '\0')*/)
  {
    ++(*LineLength);
    ++(*CharCounter);
    if(*CharCounter >= ArraySize)
    {
      //*Line       = NULL;
      //*LineLength = -1;
      return -1;
    }
  }
  ++(*CharCounter);
  return *LineLength;
}

material
ImportMaterial(Memory::stack_allocator* Allocator, Resource::resource_manager* Resources,
               const char* Path)
{
  if(strlen(Path) <= strlen(".mat"))
  {
    printf("%s is not a .mat file!\n", Path);
    assert(0);
  }
  else if(!strcmp(&Path[strlen(Path) - strlen(".mat") - 1], ".mat"))
  {
    printf("%s is not a .mat file!\n", Path);
    assert(0);
  }

  material Material          = {};
  Material.Common.ShaderType = SHADER_Phong;

  bool LoadingMaterial = false;

  debug_read_file_result FileData    = Platform::ReadEntireFile(Allocator, Path);
  int32_t                CharCounter = 0;

  if(FileData.ContentsSize <= 0)
  {
    printf("File %s is empty!\n", Path);
  }

  char*   Line       = NULL;
  int32_t LineLength = 0;

  while(GetLine(&Line, &LineLength, (char*)FileData.Contents, FileData.ContentsSize,
                &CharCounter) != -1)
  {
    int32_t Offset = 0;
    rid     RID;

    while((Line[Offset] == ' ') || (Line[Offset] == '\t'))
    {
      ++Offset;
    }

    if(Offset + 2 <= LineLength)
    {
      if((Line[Offset] == 'N') && (Line[Offset + 1] == 's'))
      {
        Offset += strlen("Ns");
        sscanf(&Line[Offset], " %f", &Material.Phong.Shininess);
        continue;
      }
      else if((Line[Offset] == 'K') && (Line[Offset + 1] == 'a'))
      {
        Offset += strlen("Ka");
        sscanf(&Line[Offset], " %f %f %f", &Material.Phong.AmbientColor.R,
               &Material.Phong.AmbientColor.G, &Material.Phong.AmbientColor.B);
        continue;
      }
      else if((Line[Offset] == 'K') && (Line[Offset + 1] == 'd'))
      {
        Offset += strlen("Kd");
        sscanf(&Line[Offset], " %f %f %f", &Material.Phong.DiffuseColor.R,
               &Material.Phong.DiffuseColor.G, &Material.Phong.DiffuseColor.B);
        continue;
      }
      else if((Line[Offset] == 'K') && (Line[Offset + 1] == 's'))
      {
        Offset += strlen("Ks");
        sscanf(&Line[Offset], " %f %f %f", &Material.Phong.SpecularColor.R,
               &Material.Phong.SpecularColor.G, &Material.Phong.SpecularColor.B);
        continue;
      }
      else if(Line[Offset] == 'd')
      {
        Offset += strlen("d");
        sscanf(&Line[Offset], " %f", &Material.Phong.DiffuseColor.A);
        continue;
      }
    }

    if(Offset + 5 <= LineLength)
    {
      if(strncmp(&Line[Offset], "blend", strlen("blend")) == 0)
      {
        Material.Common.UseBlending = true;
        continue;
      }
    }

    if(Offset + 6 <= LineLength)
    {
      if(strncmp(&Line[Offset], "newmtl", strlen("newmtl")) == 0)
      {
        assert((!LoadingMaterial) && "There can be only one material in single file!\n");

        Offset += strlen("newmtl");
        LoadingMaterial = true;
        continue;
      }
      else if(strncmp(&Line[Offset], "map_Kd", strlen("map_Kd")) == 0)
      {
        Material.Phong.Flags |= PHONG_UseDiffuseMap;

        Offset += strlen("map_Kd");
        while((Line[Offset] == ' ') || (Line[Offset] == '\t'))
        {
          ++Offset;
        }

        if(Offset >= LineLength)
        {
          printf("Line\n%s contains no path\n", Line);
          assert(0);
        }

        path   Path       = {};
        size_t PathLength = strcspn(&Line[Offset], " \t\n");
        if(PathLength >= PATH_MAX_LENGTH)
        {
          printf("Path in line\n%sis too long\n", Line);
        }
        assert(PathLength < PATH_MAX_LENGTH);

        strncpy(Path.Name, &Line[Offset], PathLength);
        Path.Name[PathLength] = '\0';

        if(Resources->GetTexturePathRID(&RID, Path.Name))
        {
          Material.Phong.DiffuseMapID = RID;
        }
        else
        {
          Material.Phong.DiffuseMapID = Resources->RegisterTexture(Path.Name);
        }
        continue;
      }
      else if(strncmp(&Line[Offset], "map_Ks", strlen("map_Ks")) == 0)
      {
        Material.Phong.Flags |= PHONG_UseSpecularMap;

        Offset += strlen("map_Ks");
        while((Line[Offset] == ' ') || (Line[Offset] == '\t'))
        {
          ++Offset;
        }

        if(Offset >= LineLength)
        {
          printf("Line\n%s contains no path\n", Line);
          assert(0);
        }

        path   Path;
        size_t PathLength = strcspn(&Line[Offset], " \t\n");
        if(PATH_MAX_LENGTH <= PathLength)
        {
          printf("Path in line\n%sis too long\n", Line);
        }
        assert(PathLength < PATH_MAX_LENGTH);

        strncpy(Path.Name, &Line[Offset], PathLength);
        Path.Name[PathLength] = '\0';

        if(Resources->GetTexturePathRID(&RID, Path.Name))
        {
          Material.Phong.SpecularMapID = RID;
        }
        else
        {
          Material.Phong.SpecularMapID = Resources->RegisterTexture(Path.Name);
        }
        continue;
      }
    }

    if(Offset + 7 <= LineLength)
    {
      if(strncmp(&Line[Offset], "skeletal", strlen("skeletal")) == 0)
      {
        Material.Common.IsSkeletal = true;
        Material.Phong.Flags |= PHONG_UseSkeleton;
        continue;
      }
    }

    if(Offset + 9 <= LineLength)
    {
      if(strncmp(&Line[Offset], "map_normal", strlen("map_normal")) == 0)
      {
        Material.Phong.Flags |= PHONG_UseNormalMap;

        Offset += strlen("map_normal");
        while((Line[Offset] == ' ') || (Line[Offset] == '\t'))
        {
          ++Offset;
        }

        if(Offset >= LineLength)
        {
          printf("Line\n%s contains no path\n", Line);
          assert(0);
        }

        path   Path;
        size_t PathLength = strcspn(&Line[Offset], " \t\n");
        if(PATH_MAX_LENGTH <= PathLength)
        {
          printf("Path in line\n%sis too long\n", Line);
        }
        assert(PathLength < PATH_MAX_LENGTH);

        strncpy(Path.Name, &Line[Offset], PathLength);
        Path.Name[PathLength] = '\0';

        if(Resources->GetTexturePathRID(&RID, Path.Name))
        {
          Material.Phong.NormalMapID = RID;
        }
        else
        {
          Material.Phong.NormalMapID = Resources->RegisterTexture(Path.Name);
        }
      }
      continue;
    }

    Line       = NULL;
    LineLength = 0;
  }

  return Material;
}

void
ExportMaterial(Resource::resource_manager* ResourceManager, const material* Material,
               const char* Path)
{
  assert(Path);
  assert(strncmp("data/materials/", Path, strlen("data/materials/")) == 0);
  char FileName[30];

  FILE* FilePointer = fopen(Path, "w");

  fprintf(FilePointer, "newmtl %s\n", Path + strlen("data/materials/"));
  if(Material->Common.ShaderType == SHADER_Phong)
  {
    fprintf(FilePointer, "\tNs %f\n", Material->Phong.Shininess);
    fprintf(FilePointer, "\tNi %f\n", 1.0f);
    fprintf(FilePointer, "\tKa %f %f %f\n", Material->Phong.AmbientColor.R,
            Material->Phong.AmbientColor.G, Material->Phong.AmbientColor.B);
    fprintf(FilePointer, "\tKd %f %f %f\n", Material->Phong.DiffuseColor.R,
            Material->Phong.DiffuseColor.G, Material->Phong.DiffuseColor.B);
    fprintf(FilePointer, "\tKs %f %f %f\n", Material->Phong.SpecularColor.R,
            Material->Phong.SpecularColor.G, Material->Phong.SpecularColor.B);
    fprintf(FilePointer, "\td %f\n", Material->Phong.DiffuseColor.A);

    if(Material->Phong.Common.IsSkeletal)
    {
      fprintf(FilePointer, "\tskeletal\n");
    }
    if(Material->Phong.Common.UseBlending)
    {
      fprintf(FilePointer, "\tblend\n");
    }
    if(Material->Phong.Flags & PHONG_UseDiffuseMap)
    {
      fprintf(FilePointer, "\tmap_Kd %s\n",
              ResourceManager
                ->TexturePaths[ResourceManager->GetTexturePathIndex(Material->Phong.DiffuseMapID)]
                .Name);
    }
    if(Material->Phong.Flags & PHONG_UseSpecularMap)
    {
      fprintf(FilePointer, "\tmap_Ks %s\n",
              ResourceManager
                ->TexturePaths[ResourceManager->GetTexturePathIndex(Material->Phong.SpecularMapID)]
                .Name);
    }
    if(Material->Phong.Flags & PHONG_UseNormalMap)
    {
      fprintf(FilePointer, "\tmap_normal %s\n",
              ResourceManager
                ->TexturePaths[ResourceManager->GetTexturePathIndex(Material->Phong.NormalMapID)]
                .Name);
    }
  }
  else if(Material->Common.ShaderType == SHADER_Color)
  {
    fprintf(FilePointer, "\tNi %f\n", 1.0f);
    fprintf(FilePointer, "\tKa %f %f %f\n", Material->Color.Color.R, Material->Color.Color.G,
            Material->Color.Color.B);
    fprintf(FilePointer, "\tKd %f %f %f\n", Material->Color.Color.R, Material->Color.Color.G,
            Material->Color.Color.B);
    fprintf(FilePointer, "\tKs %f %f %f\n", 0.0f, 0.0f, 0.0f);
    fprintf(FilePointer, "\td %f\n", Material->Color.Color.A);
  }
  fprintf(FilePointer, "\n");

  fclose(FilePointer);
}
