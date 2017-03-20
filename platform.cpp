#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

// snakebird, may not all be necessary
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Memory
#include <sys/mman.h>

#include "common.h"
#include "load_obj.h"
#include "load_shader.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "stack_allocator.h"
#include "game.h"

uint32_t
SafeTruncateUint64(uint64_t Value)
{
  assert(Value <= 0xffffffff);
  uint32_t Result = (uint32_t)Value;
  return Result;
}

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
        else if(Event->key.keysym.sym == SDLK_SPACE)
        {
          NewInput->Space.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_LCTRL)
        {
          NewInput->LeftCtrl.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_e)
        {
          NewInput->e.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_g)
        {
          NewInput->g.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_p)
        {
          NewInput->p.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_r)
        {
          NewInput->r.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_s)
        {
          NewInput->s.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_t)
        {
          NewInput->t.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_m)
        {
          NewInput->m.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_n)
        {
          NewInput->n.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_o)
        {
          NewInput->o.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_UP)
        {
          NewInput->ArrowUp.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_DOWN)
        {
          NewInput->ArrowDown.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_RIGHT)
        {
          NewInput->ArrowRight.EndedDown = true;
        }
        else if(Event->key.keysym.sym == SDLK_LEFT)
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
        else if(Event->key.keysym.sym == SDLK_LCTRL)
        {
          NewInput->LeftCtrl.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_e)
        {
          NewInput->e.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_g)
        {
          NewInput->g.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_p)
        {
          NewInput->p.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_r)
        {
          NewInput->r.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_s)
        {
          NewInput->s.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_t)
        {
          NewInput->t.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_m)
        {
          NewInput->m.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_n)
        {
          NewInput->n.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_o)
        {
          NewInput->o.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_UP)
        {
          NewInput->ArrowUp.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_DOWN)
        {
          NewInput->ArrowDown.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_RIGHT)
        {
          NewInput->ArrowRight.EndedDown = false;
        }
        else if(Event->key.keysym.sym == SDLK_LEFT)
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
        else if(Event->button.button == SDL_BUTTON_RIGHT)
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
        else if(Event->button.button == SDL_BUTTON_RIGHT)
        {
          NewInput->MouseRight.EndedDown = false;
        }
        break;
      }
    }
  }
  SDL_GetMouseState(&NewInput->MouseX, &NewInput->MouseY);

  for(uint32_t Index = 0; Index < sizeof(NewInput->Buttons) / sizeof(game_button_state); Index++)
  {
    NewInput->Buttons[Index].Changed =
      (OldInput->Buttons[Index].EndedDown == NewInput->Buttons[Index].EndedDown) ? false : true;
  }
  return true;
}

debug_read_file_result
PlatformReadEntireFile(char* FileName)
{
  debug_read_file_result Result     = {};
  int                    FileHandle = open(FileName, O_RDONLY);
  if(FileHandle == -1)
  {
    return Result;
  }

  struct stat FileStatus;
  if(fstat(FileHandle, &FileStatus) == -1)
  {
    close(FileHandle);
    return Result;
  }
  Result.ContentsSize = SafeTruncateUint64(FileStatus.st_size);

  Result.Contents = malloc(Result.ContentsSize);
  if(!Result.Contents)
  {
    Result.ContentsSize = 0;
    close(FileHandle);
    return Result;
  }

  uint64_t BytesStoread     = Result.ContentsSize;
  uint8_t* NextByteLocation = (uint8_t*)Result.Contents;
  while(BytesStoread)
  {
    int64_t BytesRead = read(FileHandle, NextByteLocation, BytesStoread);
    if(BytesRead == -1)
    {
      free(Result.Contents);
      Result.Contents     = 0;
      Result.ContentsSize = 0;
      close(FileHandle);
      return Result;
    }
    BytesStoread -= BytesRead;
    NextByteLocation += BytesRead;
  }
  close(FileHandle);
  return Result;
}

bool
PlatformWriteEntireFile(char* Filename, uint64_t MemorySize, void* Memory)
{
  int FileHandle = open(Filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if(FileHandle == -1)
  {
    return false;
  }

  uint64_t BytesToWrite     = MemorySize;
  uint8_t* NextByteLocation = (uint8_t*)Memory;
  while(BytesToWrite)
  {
    int64_t BytesWritten = write(FileHandle, NextByteLocation, BytesToWrite);
    if(BytesWritten == -1)
    {
      close(FileHandle);
      return false;
    }
    BytesToWrite -= BytesWritten;
    NextByteLocation += BytesWritten;
  }
  close(FileHandle);
  return true;
}

loaded_bitmap
PlatformLoadBitmapFromFile(char* FileName)
{
  loaded_bitmap Result       = {};
  SDL_Surface*  ImageSurface = SDL_LoadBMP(FileName);
  if(ImageSurface)
  {
    Result.Texels = ImageSurface->pixels;
    Result.Width  = ImageSurface->w;
    Result.Height = ImageSurface->h;

    assert(Result.Width > 0 && Result.Height > 0);
  }
  else
  {
    printf("Platform: bitmap load error: %s\n", SDL_GetError());
  }

  return Result;
}

void
PlatformFreeFileMemory(debug_read_file_result FileHandle)
{
  if(FileHandle.ContentsSize)
  {
    free(FileHandle.Contents);
  }
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

int
main(int argc, char* argv[])
{
  SDL_Window* Window = nullptr;
  if(!Init(&Window))
  {
    return -1;
  }

  bool AssetsHaveLoaded = false;

  game_state  GameState = {};
  game_memory GameMemory;
  {
    GameMemory.TemporaryMemory  = malloc(Mibibytes(20));
    GameMemory.PersistentMemory = malloc(Mibibytes(20));

    if(GameMemory.TemporaryMemory && GameMemory.PersistentMemory)
    {
      GameMemory.HasBeenInitialized   = true;
      GameMemory.TemporaryMemorySize  = Mibibytes(20);
      GameMemory.PersistentMemorySize = Mibibytes(20);
    }
  }

  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  SDL_Event  Event;
  game_input OldInput = {};
  game_input NewInput = {};

  // Main loop
  while(true)
  {
    ProcessInput(&OldInput, &NewInput, &Event);
    if(NewInput.Escape.EndedDown)
    {
      break;
    }

    GameUpdateAndRender(GameMemory, &GameState, &AssetsHaveLoaded, &NewInput);

    SDL_GL_SwapWindow(Window);
    SDL_Delay(16);

    OldInput = NewInput;
  }

  free(GameMemory.TemporaryMemory);
  free(GameMemory.PersistentMemory);
  SDL_DestroyWindow(Window);
  SDL_Quit();

  return 0;
}
