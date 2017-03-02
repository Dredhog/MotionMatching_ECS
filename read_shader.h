#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>

GLchar*
ReadShader(const char* FileName)
{
  FILE* FilePointer = fopen(FileName, "r");

  if(!FilePointer)
  {
    printf("File could not be opened!\n");
    return 0;
  }

  fseek(FilePointer, 0, SEEK_END);
  int FileSize = (int)ftell(FilePointer);
  fseek(FilePointer, 0, SEEK_SET);

  if(!FileSize)
  {
    printf("File is empty!\n");
    return 0;
  }

  GLchar* ShaderCode = (GLchar*)malloc(FileSize * sizeof(GLchar));
  if(!ShaderCode)
  {
    printf("Memory for shader could not be allocated!\n");
    return 0;
  }

  for(int i = 0; i < FileSize; i++)
  {
    ShaderCode[i] = (char)fgetc(FilePointer);
  }

  return ShaderCode;
}

void
SetShader(GLuint* Shader, GLint* Success, GLchar* InfoLog, const char* ShaderPath,
          GLenum ShaderType)
{
  const GLchar* ShaderSource = ReadShader(ShaderPath);
  if(!ShaderSource)
  {
    printf("Shader at %s is empty!\n", ShaderPath);
  }

  // Setting up shader
  *Shader = glCreateShader(ShaderType);
  glShaderSource(*Shader, 1, &ShaderSource, NULL);
  glCompileShader(*Shader);

  // Checking if shader compiled successfully
  glGetShaderiv(*Shader, GL_COMPILE_STATUS, Success);
  if(!*Success)
  {
    glGetShaderInfoLog(*Shader, 512, NULL, InfoLog);
    printf("Shader at %s compilation failed!\n%s\n", ShaderPath, InfoLog);
  }
}

void
LoadShader(GLuint* ShaderProgram, const char* VertexShaderPath, const char* FragmentShaderPath)
{
  GLint  Success;
  GLchar InfoLog[512];

  GLuint VertexShader;
  SetShader(&VertexShader, &Success, InfoLog, VertexShaderPath, GL_VERTEX_SHADER);

  GLuint FragmentShader;
  SetShader(&FragmentShader, &Success, InfoLog, FragmentShaderPath, GL_FRAGMENT_SHADER);

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
  }

  // Deleting linked shaders
  glDeleteShader(VertexShader);
  glDeleteShader(FragmentShader);
}
