#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#include "load_obj.h"
#include "load_shader.h"

bool
Init(SDL_Window** Window)
{
  bool Success = true; // Init SDL
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
      SDL_CreateWindow("ngpe - Non general-purpose engine", 0, 0, 1080, 820, SDL_WINDOW_OPENGL);
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
SetUpDrawing(GLuint* VAO, GLuint* ShaderProgram, Mesh::mesh* Mesh)
{
  const char* ShaderPath = "./shaders/shader";

  // Setting up vertex array object
  glGenVertexArrays(1, VAO);
  glBindVertexArray(*VAO);

  // Setting up vertex buffer object
  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, Mesh->VerticeCount * Mesh->FloatsPerVertex * sizeof(float),
               Mesh->Floats, GL_STATIC_DRAW);

  // Setting up element buffer object
  GLuint EBO;
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, Mesh->IndiceCount * sizeof(uint32_t), Mesh->Indices,
               GL_STATIC_DRAW);

  int Success = LoadShader(ShaderProgram, ShaderPath);
  if(!Success)
  {
    printf("Shader loading failed!\n");
  }
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

  Mesh::mesh Mesh = Mesh::LoadOBJMesh("./armadillo.obj\0", false, true, false);

  if(!Mesh.VerticeCount)
  {
    printf("ReadOBJ error: no vertices read\n");
  }

  SDL_Event Event;

  GLuint VAO;
  GLuint shaderProgram;
  SetUpDrawing(&VAO, &shaderProgram, &Mesh);
	glEnable(GL_DEPTH_TEST);
	

  // Main loop
  while(true)
  {
    SDL_PollEvent(&Event);
    if(Event.type == SDL_QUIT)
    {
      break;
    }
    else if(Event.type == SDL_KEYDOWN && Event.key.keysym.sym == SDLK_ESCAPE)
    {
      break;
    }
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    // Drawing square
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, Mesh.IndiceCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    SDL_GL_SwapWindow(Window);
    SDL_Delay(16);
  }

  SDL_DestroyWindow(Window);
  SDL_Quit();
  return 0;
}
