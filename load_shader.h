#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>

#include "stack_allocator.h"

namespace Shader
{
  static GLchar*
  ReadASCIIFileIntoMemory(Memory::stack_allocator* const Allocator, const char* FileName,
                          GLint* Success)
  {
    FILE* FilePointer = fopen(FileName, "r");

    if(!FilePointer)
    {
      printf("ASCII file read error: %s could not be opened!\n", FileName);
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

    GLchar* ShaderCode = (GLchar*)Allocator->Alloc((uint32_t)TextSizeInBytes);

    for(int i = 0; i < FileSize; i++)
    {
      ShaderCode[i] = (char)fgetc(FilePointer);
    }

    ShaderCode[FileSize] = '\0';
    fclose(FilePointer);
    return ShaderCode;
  }

  GLint
  LoadShader(Memory::stack_allocator* const Allocator, const char* ShaderPath)
  {
    char* VertexShaderPath =
      (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t((strlen(ShaderPath) + 6)));
    strcpy(VertexShaderPath, ShaderPath);
    strcat(VertexShaderPath, ".vert\0");

    char* FragmentShaderPath =
      (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen(ShaderPath) + 6));
    strcpy(FragmentShaderPath, ShaderPath);
    strcat(FragmentShaderPath, ".frag\0");

    int    Success = 1;
    GLchar InfoLog[512];

    GLuint VertexShader;
    GLuint FragmentShader;

    GLchar* VertexShaderSource = ReadASCIIFileIntoMemory(Allocator, VertexShaderPath, &Success);
    if(!Success)
    {
      printf("Shader at %s could not be read!\n", VertexShaderPath);
      return -1;
    }

    // Setting up shader
    VertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(VertexShader, 1, &VertexShaderSource, NULL);
    glCompileShader(VertexShader);

    // Checking if shader compiled successfully
    glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &Success);
    if(!Success)
    {
      glGetShaderInfoLog(VertexShader, 512, NULL, InfoLog);
      printf("Shader at %s compilation failed!\n%s\n", VertexShaderPath, InfoLog);
      return -1;
    }

    GLchar* FragmentShaderSource = ReadASCIIFileIntoMemory(Allocator, FragmentShaderPath, &Success);
    if(!Success)
    {
      printf("Shader at %s could not be read!\n", FragmentShaderPath);
      return -1;
    }

    // Setting up fragment shader
    FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(FragmentShader, 1, &FragmentShaderSource, NULL);
    glCompileShader(FragmentShader);

    // Checking if fragment shader compiled successfully
    glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &Success);
    if(!Success)
    {
      glGetShaderInfoLog(FragmentShader, 512, NULL, InfoLog);
      printf("Shader file %s compilation error: \n%s\n", FragmentShaderPath, InfoLog);
      return -1;
    }

    // Setting up shader program
    GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VertexShader);
    glAttachShader(ShaderProgram, FragmentShader);

    // Linking shader program
    glLinkProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
    if(!Success)
    {
      glGetProgramInfoLog(ShaderProgram, 512, NULL, InfoLog);
      printf("Shader linker error: \n%s\n", InfoLog);
      return -1;
    }

    // Deleting linked shaders
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    if(Success)
    {
      return ShaderProgram;
    }
    else
    {
      return -1;
    }
  }
}
