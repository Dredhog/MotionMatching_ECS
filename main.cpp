#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <stdio.h>

bool
Init(SDL_Window** Window)
{
  bool Success = true; // Init SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
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
      SDL_CreateWindow("ngpe - Non general-purpose engine", 0, 0, 640, 480, SDL_WINDOW_OPENGL);
    if (!Window)
    {
      printf("SDL error: failed to load window. %s\n", SDL_GetError());
      Success = false;
    }
    else
    {
      // Establish an OpenGL context with SDL
      SDL_GLContext GraphicsContext = SDL_GL_CreateContext(*Window);
      if (!GraphicsContext)
      {
        printf("SDL error: failed to establish an opengl context. %s\n", SDL_GetError());
        Success = false;
      }
      else
      {
        glewExperimental = GL_TRUE;
        GLenum GlewError = glewInit();
        if (GlewError != GLEW_OK)
        {
          printf("GLEW error: initialization failed. %s\n", glewGetErrorString(GlewError));
          Success = false;
        }

        if (SDL_GL_SetSwapInterval(1) < 0)
        {
          printf("SDL error: unable to set swap interval. %s\n", glewGetErrorString(GlewError));
          Success = false;
        }
      }
    }
  }
  return Success;
}

int
main(int argc, char* argv[])
{
  SDL_Window* Window = nullptr;
  if (!Init(&Window))
  {
    return -1;
  }
  printf("Everything went well...!\n");
  glViewport(0, 0, 640, 480);

  SDL_Event Event;
  // Main loop
  while (true)
  {
    SDL_PollEvent(&Event);
    if (Event.type == SDL_QUIT)
    {
      break;
    }
    else if (Event.type == SDL_KEYDOWN && Event.key.keysym.sym == SDLK_ESCAPE)
    {
      break;
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(Window);
    SDL_Delay(16);
  }

  SDL_DestroyWindow(Window);
  SDL_Quit();
  return 0;
}
