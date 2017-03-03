#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>

static GLchar*
ReadASCIIFileIntoMemory(const char* FileName, GLint* Success)
{
  FILE* FilePointer = fopen(FileName, "r");

  if(!FilePointer)
  {
    printf("File could not be opened!\n");
    *Success = 0;
  }

  fseek(FilePointer, 0, SEEK_END);
  int FileSize = (int)ftell(FilePointer);
  fseek(FilePointer, 0, SEEK_SET);

  if(!FileSize)
  {
    printf("ASCII file read error: %s is empty!\n", FileName);
    *Success = 0;
  }

  const size_t TextSizeInBytes = (size_t)FileSize * sizeof(GLchar) + 1;

  GLchar* ShaderCode = (GLchar*)malloc(TextSizeInBytes);
  if(!ShaderCode)
  {
    printf("ASCII file read error: failed to allocate %lu bytes for %s!\n", TextSizeInBytes,
           FileName);
    *Success = 0;
  }

  for(int i = 0; i < FileSize; i++)
  {
    ShaderCode[i] = (char)fgetc(FilePointer);
  }

  ShaderCode[FileSize] = '\0';
  fclose(FilePointer);
  return ShaderCode;
}

GLint
LoadShader(GLuint* ShaderProgram, const char* ShaderPath)
{
  char* VertexShaderPath = (char*)malloc(strlen(ShaderPath) + 6);
  strcpy(VertexShaderPath, ShaderPath);
  strcat(VertexShaderPath, ".vert\0");

  char* FragmentShaderPath = (char*)malloc(strlen(ShaderPath) + 6);
  strcpy(FragmentShaderPath, ShaderPath);
  strcat(FragmentShaderPath, ".frag\0");

  GLint  Success = 1;
  GLchar InfoLog[512];

  GLuint VertexShader;
  GLuint FragmentShader;

  GLchar* VertexShaderSource = ReadASCIIFileIntoMemory(VertexShaderPath, &Success);
  if(!Success)
  {
    printf("Shader at %s could not be read!\n", VertexShaderPath);
    return 0;
  }

  // Setting up shader
  VertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(VertexShader, 1, &VertexShaderSource, NULL);
  glCompileShader(VertexShader);
  free(VertexShaderSource);

  // Checking if shader compiled successfully
  glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &Success);
  if(!Success)
  {
    glGetShaderInfoLog(VertexShader, 512, NULL, InfoLog);
    printf("Shader at %s compilation failed!\n%s\n", VertexShaderPath, InfoLog);
    return 0;
  }
  free(VertexShaderPath);

  GLchar* FragmentShaderSource = ReadASCIIFileIntoMemory(FragmentShaderPath, &Success);
  if(!Success)
  {
    printf("Shader at %s could not be read!\n", FragmentShaderPath);
    return 0;
  }

  // Setting up fragment shader
  FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(FragmentShader, 1, &FragmentShaderSource, NULL);
  glCompileShader(FragmentShader);
  free(FragmentShaderSource);

  // Checking if fragment shader compiled successfully
  glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &Success);
  if(!Success)
  {
    glGetShaderInfoLog(FragmentShader, 512, NULL, InfoLog);
    printf("Shader at %s compilation failed!\n%s\n", FragmentShaderPath, InfoLog);
    return 0;
  }
  free(FragmentShaderPath);

  // Setting up shader program
  *ShaderProgram = glCreateProgram();
  glAttachShader(*ShaderProgram, VertexShader);
  glAttachShader(*ShaderProgram, FragmentShader);

  // Linking shader program
  glLinkProgram(*ShaderProgram);
  glGetProgramiv(*ShaderProgram, GL_LINK_STATUS, &Success);
  if(!Success)
  {
    glGetProgramInfoLog(*ShaderProgram, 512, NULL, InfoLog);
    printf("Shader linking failed!\n%s\n", InfoLog);
    return 0;
  }

  // Deleting linked shaders
  glDeleteShader(VertexShader);
  glDeleteShader(FragmentShader);

  return Success;
}
