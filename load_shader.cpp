#include "load_shader.h"
#include "stack_alloc.h"
#include "file_io.h"
#include "profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Shader
{
  static GLchar*
  ReadASCIIFileIntoMemory(Memory::stack_allocator* const Allocator, const char* FileName,
                          GLint* Success)
  {
    debug_read_file_result ReadFile = Platform::ReadEntireFile(Allocator, FileName);
    assert(ReadFile.ContentsSize && ReadFile.Contents);
    char* LastChar = PushStruct(Allocator, char);
    *LastChar      = '\0';
    return (GLchar*)ReadFile.Contents;
  }

  static GLuint
  ImportShader(Memory::stack_allocator* const Allocator, const char* ShaderPath)
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
      return 0;
    }

    // Setting up shader
    VertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(VertexShader, 1, &VertexShaderSource, NULL);
    glCompileShader(VertexShader);

    static const char* HorizontalSeparator =
      "------------------------------------------------------------";

    // Checking if shader compiled successfully
    glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &Success);
    if(!Success)
    {
      printf("%s\n", HorizontalSeparator);
      glGetShaderInfoLog(VertexShader, 512, NULL, InfoLog);
      printf("Shader at %s compilation failed!%s", VertexShaderPath, InfoLog);
      printf("%s\n", HorizontalSeparator);
      return 0;
    }

    GLchar* FragmentShaderSource = ReadASCIIFileIntoMemory(Allocator, FragmentShaderPath, &Success);
    if(!Success)
    {
      printf("Shader at %s could not be read!\n", FragmentShaderPath);
      return 0;
    }

    // Setting up fragment shader
    FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(FragmentShader, 1, &FragmentShaderSource, NULL);
    glCompileShader(FragmentShader);

    // Checking if fragment shader compiled successfully
    glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &Success);
    if(!Success)
    {
      printf("%s\n", HorizontalSeparator);
      glGetShaderInfoLog(FragmentShader, 512, NULL, InfoLog);
      printf("Shader file %s compilation error: %s", FragmentShaderPath, InfoLog);
      printf("%s\n", HorizontalSeparator);
      return 0;
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
      printf("%s\n", HorizontalSeparator);
      glGetProgramInfoLog(ShaderProgram, 512, NULL, InfoLog);
      printf("Shader linker error: %s", InfoLog);
      printf("%s\n", HorizontalSeparator);
      return 0;
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
      return 0;
    }
  }

  GLuint
  CheckedLoadCompileFreeShader(Memory::stack_allocator* Alloc, const char* RelativePath)
  {
		BEGIN_TIMED_BLOCK(LoadShader);
    Memory::marker LoadStart = Alloc->GetMarker();
    GLuint         Result    = Shader::ImportShader(Alloc, RelativePath);
    Alloc->FreeToMarker(LoadStart);

    if(Result == 0)
    {
      printf("Shader %s failed to load correctly!\n", RelativePath);
    }
		END_TIMED_BLOCK(LoadShader);
    return Result;
  }
}
