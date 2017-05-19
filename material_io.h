#pragma once

#include <stdio.h>
#include <string.h>

#include "resource_manager.h"
#include "render_data.h"

material
ImportMaterial(Resource::resource_manager* Resources, const char* Path)
{
  if(!strcmp(&Path[strlen(Path) - strlen(".mat")], ".mat"))
  {
    printf("%s is not a .mat file!\n", Path);
    assert(0);
  }

  material Material          = {};
  Material.Common.ShaderType = SHADER_Phong;

  bool LoadingMaterial = false;

  FILE*  FilePointer = fopen(Path, "r");
  char*  Line        = NULL;
  size_t LineLength  = 0;

  while(getline(&Line, &LineLength, FilePointer) != -1)
  {
    int32_t Offset = 0;
    rid     RID;

    while((Line[Offset] == ' ') || (Line[Offset] == '\t'))
    {
      ++Offset;
    }
    assert(Offset < LineLength);

    if(strncmp(&Line[Offset], "newmtl ", strlen("newmtl ")) == 0)
    {
      assert((!LoadingMaterial) && "There can be only one material in single file!\n");

      Offset += strlen("newmtl ");
      printf("Importing material %s\n", &Line[Offset]);
      LoadingMaterial = true;
    }
    else if(strncmp(&Line[Offset], "Ns", strlen("Ns")) == 0)
    {
      Offset += strlen("Ns");
      sscanf(&Line[Offset], " %f", &Material.Phong.Shininess);
    }
    else if(strncmp(&Line[Offset], "Ka", strlen("Ka")) == 0)
    {
      Offset += strlen("Ka");
      sscanf(&Line[Offset], " %f %f %f", &Material.Phong.AmbientColor.R,
             &Material.Phong.AmbientColor.G, &Material.Phong.AmbientColor.B);
    }
    else if(strncmp(&Line[Offset], "Kd", strlen("Kd")) == 0)
    {
      Offset += strlen("Kd");
      sscanf(&Line[Offset], " %f %f %f", &Material.Phong.DiffuseColor.R,
             &Material.Phong.DiffuseColor.G, &Material.Phong.DiffuseColor.B);
    }
    else if(strncmp(&Line[Offset], "Ks", strlen("Ks")) == 0)
    {
      Offset += strlen("Ks");
      sscanf(&Line[Offset], " %f %f %f", &Material.Phong.SpecularColor.R,
             &Material.Phong.SpecularColor.G, &Material.Phong.SpecularColor.B);
    }
    else if(strncmp(&Line[Offset], "map_Kd", strlen("map_Kd")) == 0)
    {
      Material.Phong.Flags |= PHONG_UseDiffuseMap;

      Offset += strlen("map_Kd");
      while((Line[Offset] == ' ') || (Line[Offset] == '\t'))
      {
        ++Offset;
      }

      char   Path[TEXT_LINE_MAX_LENGTH];
      size_t PathLength = strcspn(&Line[Offset], " \t\n");
      if(PathLength >= TEXT_LINE_MAX_LENGTH)
      {
        printf("Path in line\n%sis too long\n", Line);
      }
      assert(PathLength < TEXT_LINE_MAX_LENGTH);

      strncpy(Path, &Line[Offset], PathLength);

      if(Resources->GetTexturePathRID(&RID, Path))
      {
        Material.Phong.DiffuseMapID = RID;
      }
      else
      {
        Material.Phong.DiffuseMapID = Resources->RegisterTexture(Path);
      }
    }
    else if(strncmp(&Line[Offset], "map_Ns", strlen("map_Ns")) == 0)
    {
      Material.Phong.Flags |= PHONG_UseSpecularMap;

      Offset += strlen("map_Ns");
      while((Line[Offset] == ' ') || (Line[Offset] == '\t'))
      {
        ++Offset;
      }

      char   Path[TEXT_LINE_MAX_LENGTH];
      size_t PathLength = strcspn(&Line[Offset], " \t\n");
      if(PathLength >= TEXT_LINE_MAX_LENGTH)
      {
        printf("Path in line\n%sis too long\n", Line);
      }
      assert(PathLength < TEXT_LINE_MAX_LENGTH);

      strncpy(Path, &Line[Offset], PathLength);

      if(Resources->GetTexturePathRID(&RID, Path))
      {
        Material.Phong.SpecularMapID = RID;
      }
      else
      {
        Material.Phong.SpecularMapID = Resources->RegisterTexture(Path);
      }
    }
    else if(strncmp(&Line[Offset], "bump", strlen("bump")) == 0)
    {
      Material.Phong.Flags |= PHONG_UseNormalMap;

      Offset += strlen("bump");
      while((Line[Offset] == ' ') || (Line[Offset] == '\t'))
      {
        ++Offset;
      }

      char   Path[TEXT_LINE_MAX_LENGTH];
      size_t PathLength = strcspn(&Line[Offset], " \t\n");
      if(PathLength >= TEXT_LINE_MAX_LENGTH)
      {
        printf("Path in line\n%sis too long\n", Line);
      }
      assert(PathLength < TEXT_LINE_MAX_LENGTH);

      strncpy(Path, &Line[Offset], PathLength);

      if(Resources->GetTexturePathRID(&RID, Path))
      {
        Material.Phong.NormalMapID = RID;
      }
      else
      {
        Material.Phong.NormalMapID = Resources->RegisterTexture(Path);
      }
    }
    else if(strncmp(&Line[Offset], "skeletal", strlen("skeletal")) == 0)
    {
      Material.Common.IsSkeletal = true;
      Material.Phong.Flags |= PHONG_UseSkeleton;
    }
    else if(strncmp(&Line[Offset], "blend", strlen("blend")) == 0)
    {
      Material.Common.UseBlending = true;
    }

    free(Line);
    Line       = NULL;
    LineLength = 0;
  }

  fclose(FilePointer);

  return Material;
}

void
ExportMaterials(material* Materials, int32_t MaterialCount, const char* Directory)
{
  for(int i = 0; i < MaterialCount; i++)
  {
    char FileName[30];
    sprintf(FileName, "%smaterial%d.mat", Directory, i);
    FILE* FilePointer = fopen(FileName, "w");

    fprintf(FilePointer, "newmtl material%d\n", i);
    if(Materials[i].Common.ShaderType == SHADER_Phong)
    {
      fprintf(FilePointer, "\tNs %f\n", Materials[i].Phong.Shininess);
      fprintf(FilePointer, "\tNi %f\n", 1.0f);
      fprintf(FilePointer, "\tKa %f %f %f\n", Materials[i].Phong.AmbientColor.R,
              Materials[i].Phong.AmbientColor.G, Materials[i].Phong.AmbientColor.B);
      fprintf(FilePointer, "\tKd %f %f %f\n", Materials[i].Phong.DiffuseColor.R,
              Materials[i].Phong.DiffuseColor.G, Materials[i].Phong.DiffuseColor.B);
      fprintf(FilePointer, "\tKs %f %f %f\n", Materials[i].Phong.SpecularColor.R,
              Materials[i].Phong.SpecularColor.G, Materials[i].Phong.SpecularColor.B);
      fprintf(FilePointer, "\td %f\n", Materials[i].Phong.DiffuseColor.A);

      if(Materials[i].Phong.Common.IsSkeletal)
      {
        fprintf(FilePointer, "\tskeletal\n");
      }
      if(Materials[i].Phong.Common.UseBlending)
      {
        fprintf(FilePointer, "\tblend\n");
      }
      if(Materials[i].Phong.Flags & PHONG_UseDiffuseMap)
      {
        fprintf(FilePointer, "\tmap_Kd %s\n", "Path");
      }
      if(Materials[i].Phong.Flags & PHONG_UseSpecularMap)
      {
        fprintf(FilePointer, "\tmap_Ns %s\n", "Path");
      }
      if(Materials[i].Phong.Flags & PHONG_UseNormalMap)
      {
        fprintf(FilePointer, "\tbump %s\n", "Path");
      }
    }
    else if(Materials[i].Common.ShaderType == SHADER_Color)
    {
      fprintf(FilePointer, "\tNi %f\n", 1.0f);
      fprintf(FilePointer, "\tKa %f %f %f\n", Materials[i].Color.Color.R,
              Materials[i].Color.Color.G, Materials[i].Color.Color.B);
      fprintf(FilePointer, "\tKd %f %f %f\n", Materials[i].Color.Color.R,
              Materials[i].Color.Color.G, Materials[i].Color.Color.B);
      fprintf(FilePointer, "\tKs %f %f %f\n", 0.0f, 0.0f, 0.0f);
      fprintf(FilePointer, "\td %f\n", Materials[i].Color.Color.A);
    }
    fprintf(FilePointer, "\n");

    fclose(FilePointer);
  }
}
