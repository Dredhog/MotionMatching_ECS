#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

// Memory
#include <sys/mman.h>

#include "common.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define FRAME_TIME_MS 16

#include "update_render.h"

static bool
ProcessInput(game_input* OldInput, game_input* NewInput, SDL_Event* Event)
{
  *NewInput = *OldInput;
  while(SDL_PollEvent(Event) != 0)
  {
    switch(Event->type)
    {
      case SDL_QUIT:
      {
        return false;
      }
      case SDL_KEYDOWN:
      {
        if(Event->key.keysym.sym == SDLK_ESCAPE)
        {
          NewInput->Escape.EndedDown = true;
          return false;
        }
        if(Event->key.keysym.sym == SDLK_SPACE)
        {
          NewInput->Space.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_DELETE)
        {
          NewInput->Delete.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_LCTRL)
        {
          NewInput->LeftCtrl.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_LSHIFT)
        {
          NewInput->LeftShift.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_a)
        {
          NewInput->a.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_b)
        {
          NewInput->b.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_d)
        {
          NewInput->d.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_e)
        {
          NewInput->e.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_f)
        {
          NewInput->f.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_g)
        {
          NewInput->g.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_h)
        {
          NewInput->h.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_i)
        {
          NewInput->i.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_p)
        {
          NewInput->p.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_r)
        {
          NewInput->r.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_s)
        {
          NewInput->s.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_t)
        {
          NewInput->t.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_m)
        {
          NewInput->m.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_n)
        {
          NewInput->n.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_o)
        {
          NewInput->o.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_w)
        {
          NewInput->w.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_x)
        {
          NewInput->x.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_UP)
        {
          NewInput->ArrowUp.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_DOWN)
        {
          NewInput->ArrowDown.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_RIGHT)
        {
          NewInput->ArrowRight.EndedDown = true;
        }
        if(Event->key.keysym.sym == SDLK_LEFT)
        {
          NewInput->ArrowLeft.EndedDown = true;
        }
        break;
      }
      case SDL_KEYUP:
      {
        if(Event->key.keysym.sym == SDLK_SPACE)
        {
          NewInput->Space.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_DELETE)
        {
          NewInput->Delete.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_LCTRL)
        {
          NewInput->LeftCtrl.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_LSHIFT)
        {
          NewInput->LeftShift.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_a)
        {
          NewInput->a.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_b)
        {
          NewInput->b.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_d)
        {
          NewInput->d.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_e)
        {
          NewInput->e.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_f)
        {
          NewInput->f.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_g)
        {
          NewInput->g.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_h)
        {
          NewInput->h.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_i)
        {
          NewInput->i.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_p)
        {
          NewInput->p.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_r)
        {
          NewInput->r.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_s)
        {
          NewInput->s.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_t)
        {
          NewInput->t.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_m)
        {
          NewInput->m.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_n)
        {
          NewInput->n.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_o)
        {
          NewInput->o.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_w)
        {
          NewInput->w.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_x)
        {
          NewInput->x.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_UP)
        {
          NewInput->ArrowUp.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_DOWN)
        {
          NewInput->ArrowDown.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_RIGHT)
        {
          NewInput->ArrowRight.EndedDown = false;
        }
        if(Event->key.keysym.sym == SDLK_LEFT)
        {
          NewInput->ArrowLeft.EndedDown = false;
        }
        break;
      }
      case SDL_MOUSEBUTTONDOWN:
      {
        if(Event->button.button == SDL_BUTTON_LEFT)
        {
          NewInput->MouseLeft.EndedDown = true;
        }
        if(Event->button.button == SDL_BUTTON_RIGHT)
        {
          NewInput->MouseRight.EndedDown = true;
        }
        break;
      }
      case SDL_MOUSEBUTTONUP:
      {
        if(Event->button.button == SDL_BUTTON_LEFT)
        {
          NewInput->MouseLeft.EndedDown = false;
        }
        if(Event->button.button == SDL_BUTTON_RIGHT)
        {
          NewInput->MouseRight.EndedDown = false;
        }
        break;
      }
    }
  }
  SDL_GetMouseState(&NewInput->MouseX, &NewInput->MouseY);
  NewInput->dMouseX = NewInput->MouseX - SCREEN_WIDTH / 2;
  NewInput->dMouseY = NewInput->MouseY - SCREEN_HEIGHT / 2;

  for(uint32_t Index = 0; Index < sizeof(NewInput->Buttons) / sizeof(game_button_state); Index++)
  {
    NewInput->Buttons[Index].Changed =
      (OldInput->Buttons[Index].EndedDown == NewInput->Buttons[Index].EndedDown) ? false : true;
  }
  return true;
}

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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create an SDL window
    SDL_ShowCursor(SDL_DISABLE);
    *Window = SDL_CreateWindow("ngpe - Non general-purpose engine", 0, 0, SCREEN_WIDTH,
                               SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
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

int
main(int argc, char* argv[])
{
  SDL_Window* Window = nullptr;
  if(!Init(&Window))
  {
    return -1;
  }

  game_memory GameMemory;
  {
    GameMemory.TemporaryMemorySize  = Mibibytes(10);
    GameMemory.PersistentMemorySize = Mibibytes(10);

    GameMemory.TemporaryMemory  = malloc(GameMemory.TemporaryMemorySize);
    GameMemory.PersistentMemory = malloc(GameMemory.PersistentMemorySize);

    if(GameMemory.TemporaryMemory && GameMemory.PersistentMemory)
    {
      GameMemory.HasBeenInitialized = true;
    }
    else
    {
      assert("error: unable to initialize memory" && 0);
      GameMemory.TemporaryMemorySize  = 0;
      GameMemory.PersistentMemorySize = 0;
      GameMemory.HasBeenInitialized   = true;
    }
  }

  SDL_Event Event;

  game_input OldInput = {};
  game_input NewInput = {};
  SDL_WarpMouseInWindow(Window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

  ProcessInput(&OldInput, &NewInput, &Event);
  OldInput = NewInput;

  // Main loop
  while(true)
  {

    ProcessInput(&OldInput, &NewInput, &Event);
    SDL_WarpMouseInWindow(Window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    if(NewInput.Escape.EndedDown)
    {
      break;
    }

    NewInput.dt = 0.001f * (float)FRAME_TIME_MS;
    GameUpdateAndRender(GameMemory, &NewInput);

    SDL_GL_SwapWindow(Window);
    SDL_Delay(FRAME_TIME_MS);

    OldInput = NewInput;
  }

  free(GameMemory.TemporaryMemory);
  free(GameMemory.PersistentMemory);
  SDL_DestroyWindow(Window);
  SDL_Quit();

  return 0;
}
