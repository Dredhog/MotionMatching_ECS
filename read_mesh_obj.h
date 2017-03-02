#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <cstdio>
#include <cassert>

struct mesh
{
  float*  Vertices;
  int32_t VerticeCount;

  float*  Normals;
  int32_t NormalCount;

  int32_t* Indices;
  int32_t  IndiceCount;
};

void
PrintMesh(const mesh* Mesh)
{
  printf("\nVert Count: %d\nNormal Count: %d\nIndice Count: %d\n", Mesh->VerticeCount,
         Mesh->NormalCount, Mesh->IndiceCount);
  for(int i = 0; i < Mesh->VerticeCount; i++)
  {
    printf("%d: %f %f %f\n", i, (double)Mesh->Vertices[3 * i], (double)Mesh->Vertices[3 * i + 1],
           (double)Mesh->Vertices[3 * i + 2]);
  }
}

mesh
ReadOBJMesh(const char* FileName)
{
  FILE* FilePointer = fopen(FileName, "r");

  char Line[100];
  int  LineLength;

  int  i;
  char FirstC;
  char SecondC;

  float* Floats;
  mesh   Mesh = {};

  if(!FilePointer)
  {
    assert(FilePointer);
  }
  Floats = (float*)malloc(100000 * sizeof(float));
  if(!Floats)
  {
    assert(Floats);
  }
  else
  {
    Mesh.Vertices = Floats;
  }

  for(i = 0;; i++)
  {
    printf("%d: ", i);
    LineLength = 0;
    do
    {
      Line[LineLength++] = (char)fgetc(FilePointer);
      printf("%c", Line[LineLength - 1]);
    } while((Line[LineLength - 1] != '\n') && (Line[LineLength - 1] != EOF));

    if(Line[LineLength - 1] == EOF)
    {
      break;
    }

    FirstC  = Line[0];
    SecondC = '\0';
    if(LineLength > 1)
    {
      SecondC = Line[1];
    }

    if(FirstC == 'v' && SecondC == ' ')
    {
      sscanf(&Line[2], " %f %f %f", Mesh.Vertices + 3 * Mesh.VerticeCount,
             Mesh.Vertices + 3 * Mesh.VerticeCount + 1, Mesh.Vertices + 3 * Mesh.VerticeCount + 2);
      Mesh.VerticeCount++;
    }
    else if(FirstC == 'v' && SecondC == 'n')
    {
      if(Mesh.NormalCount <= 0)
      {
        Mesh.Normals = Floats + (Mesh.VerticeCount) * (3 * sizeof(float));
      }
      sscanf(&Line[2], " %f %f %f", Mesh.Normals + 3 * Mesh.NormalCount,
             Mesh.Normals + 3 * Mesh.NormalCount + 1, Mesh.Normals + 3 * Mesh.NormalCount + 2);
      Mesh.NormalCount++;
    }
    else if(FirstC == 'v' && SecondC == 't')
    {
      assert(false);
    }
    else if(FirstC == 'f')
    {
      if(Mesh.IndiceCount <= 0)
      {
        Mesh.Indices =
          (int32_t*)(Floats + (Mesh.VerticeCount + Mesh.NormalCount) * (3 * sizeof(float)));
      }
      sscanf(&Line[2], " %d %d %d", Mesh.Indices + 3 * Mesh.IndiceCount,
             Mesh.Indices + 3 * Mesh.IndiceCount + 1, Mesh.Indices + 3 * Mesh.IndiceCount + 2);
      Mesh.IndiceCount += 3;
    }
  }
  fclose(FilePointer);
  PrintMesh(&Mesh);
  return Mesh;
}
