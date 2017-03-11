#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#include "load_obj.h"
#include "load_shader.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "stack_allocator.h"

bool
Init(SDL_Window** Window)
{
  bool Success = true;
  if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    printf("SDL error: init failed. %s\n", SDL_GetError());
    Success = false;
  }
  else
  {
    // Set Opengl contet version to 3.3 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create an SDL window
    *Window =
      SDL_CreateWindow("ngpe - Non general-purpose engine", 0, 0, 1024, 800, SDL_WINDOW_OPENGL);
    if(!Window)
    {
      printf("SDL error: failed to load window. %s\n", SDL_GetError());
      Success = false;
    }
    else
    {
      // Establish an OpenGL context with SDL
      SDL_GLContext GraphicsContext = SDL_GL_CreateContext(*Window);
      if(!GraphicsContext)
      {
        printf("SDL error: failed to establish an opengl context. %s\n", SDL_GetError());
        Success = false;
      }
      else
      {
        glewExperimental = GL_TRUE;
        GLenum GlewError = glewInit();
        if(GlewError != GLEW_OK)
        {
          printf("GLEW error: initialization failed. %s\n", glewGetErrorString(GlewError));
          Success = false;
        }

        if(SDL_GL_SetSwapInterval(1) < 0)
        {
          printf("SDL error: unable to set swap interval. %s\n", glewGetErrorString(GlewError));
          Success = false;
        }
      }
    }
  }
  return Success;
}

void
SetUpMesh(Mesh::mesh* Mesh)
{
  // Setting up vertex array object
  glGenVertexArrays(1, &Mesh->VAO);
  glBindVertexArray(Mesh->VAO);

  // Setting up vertex buffer object
  glGenBuffers(1, &Mesh->VBO);
  glBindBuffer(GL_ARRAY_BUFFER, Mesh->VBO);
  glBufferData(GL_ARRAY_BUFFER, Mesh->VerticeCount * Mesh->FloatsPerVertex * sizeof(float),
               Mesh->Floats, GL_STATIC_DRAW);

  // Setting up element buffer object
  glGenBuffers(1, &Mesh->EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, Mesh->IndiceCount * sizeof(uint32_t), Mesh->Indices,
               GL_STATIC_DRAW);

  // Setting vertex attribute pointers
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Mesh->FloatsPerVertex * sizeof(float),
                        (GLvoid*)(uint64_t)(Mesh->Offsets[0] * sizeof(float)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, Mesh->FloatsPerVertex * sizeof(float),
                        (GLvoid*)(uint64_t)(Mesh->Offsets[2] * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Unbind VAO
  glBindVertexArray(0);
}

int
main(int argc, char* argv[])
{
  SDL_Window* Window = nullptr;
  if(!Init(&Window))
  {
    return -1;
  }

  void*                    TemporaryMemory = malloc(Mibibytes(20));
  Memory::stack_allocator* TemporaryMemStack =
    Memory::CreateStackAllocatorInPlace(TemporaryMemory, Mibibytes(20));

  void*                    PersistentMemory = malloc(Mibibytes(20));
  Memory::stack_allocator* PersistentMemStack =
    Memory::CreateStackAllocatorInPlace(PersistentMemory, Mibibytes(20));

  // BEGIN ASSET LOADING
  Memory::marker LoadStart = TemporaryMemStack->GetMarker();
  Mesh::mesh     Mesh      = Mesh::LoadOBJMesh(TemporaryMemStack, PersistentMemStack,
                                      "./data/armadillo.obj\0", Mesh::MAM_UseNormals);
  if(!Mesh.VerticeCount)
  {
    printf("ReadOBJ error: no vertices read\n");
    return -1;
  }
  else
  {
    SetUpMesh(&Mesh);
  }
  TemporaryMemStack->FreeToMarker(LoadStart);


  LoadStart             = TemporaryMemStack->GetMarker();
  int ShaderVertexColor = LoadShader(TemporaryMemStack, "./shaders/color");
  TemporaryMemStack->FreeToMarker(LoadStart);
  if(ShaderVertexColor < 0)
  {
    printf("Shader loading failed!\n");
  }

  LoadStart           = TemporaryMemStack->GetMarker();
  int ShaderWireframe = LoadShader(TemporaryMemStack, "./shaders/wireframe");
  TemporaryMemStack->FreeToMarker(LoadStart);
  if(ShaderVertexColor < 0)
  {
    printf("Shader loading failed!\n");
  }

  // END ASSET LOADING

  SDL_Event Event;

  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  // glFrontFace(GL_CCW);
  glEnable(GL_CULL_FACE);

  float AngleDeg = 0.0f;
  // Main loop
  while(true)
  {
    // Input polling
    SDL_PollEvent(&Event);
    if(Event.type == SDL_QUIT)
    {
      break;
    }
    else if(Event.type == SDL_KEYDOWN && Event.key.keysym.sym == SDLK_ESCAPE)
    {
      break;
    }

    // Update
    mat4 ModelMatrix   = Math::MulMat4(Math::Mat4RotateY(AngleDeg), Math::Mat4Scale(0.25f));
    mat4 CameraMatrix  = Math::Mat4Camera({ 0, 0, 1 }, { 0, 0, -1 }, { 0, 1, 0 });
    mat4 ProjectMatrix = Math::Mat4Perspective(60.f, 1028 / 800, 0.1f, 100.0f);
    mat4 MVPMatrix     = Math::MulMat4(ProjectMatrix, Math::MulMat4(CameraMatrix, ModelMatrix));

    // Rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(Mesh.VAO);

    // Switch to normal color shader
    glUseProgram(ShaderVertexColor);
#if 1
    glUniformMatrix4fv(glGetUniformLocation(ShaderVertexColor, "mat_mvp"), 1, GL_FALSE,
                       &MVPMatrix.e[0]);

    // draw model
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElements(GL_TRIANGLES, Mesh.IndiceCount, GL_UNSIGNED_INT, 0);
#endif

#if 1
    // Switch to wireframe shader
    glUseProgram(ShaderWireframe);
    glUniformMatrix4fv(glGetUniformLocation(ShaderVertexColor, "mat_mvp"), 1, GL_FALSE,
                       &MVPMatrix.e[0]);

    // draw wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, Mesh.IndiceCount, GL_UNSIGNED_INT, 0);
#endif
    glBindVertexArray(0);

    SDL_GL_SwapWindow(Window);
    SDL_Delay(16);
    AngleDeg++;
  }

  free(TemporaryMemory);
  free(PersistentMemory);
  SDL_DestroyWindow(Window);
  SDL_Quit();
  return 0;
}
