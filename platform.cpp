#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdio.h>

// Memory
#include <sys/mman.h>

#include "common.h"

#define FRAME_TIME_MS 16
#define SLOW_MOTION_COEFFICIENT 0.2f

#include "update_render.h"
#include "sound_samples.h"

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
        if(Event->key.keysym.sym == SDLK_TAB)
        {
          NewInput->Tab.EndedDown = true;
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
        if(Event->key.keysym.sym == SDLK_c)
        {
          NewInput->c.EndedDown = true;
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
        if(Event->key.keysym.sym == SDLK_v)
        {
          NewInput->v.EndedDown = true;
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
        if(Event->key.keysym.sym == SDLK_TAB)
        {
          NewInput->Tab.EndedDown = false;
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
        if(Event->key.keysym.sym == SDLK_c)
        {
          NewInput->c.EndedDown = false;
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
        if(Event->key.keysym.sym == SDLK_v)
        {
          NewInput->v.EndedDown = false;
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
        if(Event->button.button == SDL_BUTTON_MIDDLE)
        {
          NewInput->MouseMiddle.EndedDown = true;
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
        if(Event->button.button == SDL_BUTTON_MIDDLE)
        {
          NewInput->MouseMiddle.EndedDown = false;
        }
        break;
      }
    }
  }

  SDL_GetMouseState(&NewInput->MouseX, &NewInput->MouseY);
  NewInput->MouseY = SCREEN_HEIGHT - NewInput->MouseY;
  if(!NewInput->IsMouseInEditorMode)
  {
    NewInput->dMouseX = NewInput->MouseX - SCREEN_WIDTH / 2;
    NewInput->dMouseY = NewInput->MouseY - SCREEN_HEIGHT / 2;
  }
  else
  {
    NewInput->dMouseX = NewInput->MouseX - OldInput->MouseX;
    NewInput->dMouseY = NewInput->MouseY - OldInput->MouseY;
  }

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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#if 1
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif

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

bool
InitAudio(SDL_AudioDeviceID* AudioDevice, Audio::audio_ring_buffer* AudioRingBuffer,
          int SamplesPerSecond, int BufferSize)
{
  AudioRingBuffer->Size        = BufferSize;
  AudioRingBuffer->Data        = malloc(AudioRingBuffer->Size);
  AudioRingBuffer->PlayCursor  = 0;
  AudioRingBuffer->WriteCursor = 0;

  SDL_AudioSpec AudioSettings = { 0 };

  AudioSettings.freq     = SamplesPerSecond;
  AudioSettings.format   = AUDIO_S16LSB;
  AudioSettings.channels = 2;
  AudioSettings.samples  = 1024;
  AudioSettings.callback = &Audio::SDLAudioCallback;
  AudioSettings.userdata = AudioRingBuffer;

  *AudioDevice = SDL_OpenAudioDevice(NULL, 0, &AudioSettings, 0, 0);

  if(*AudioDevice == 0)
  {
    printf("Failed to get an audio device: %s\n", SDL_GetError());
    return false;
  }
  return true;
}

void
ClearSoundBuffer(Audio::audio_ring_buffer* AudioRingBuffer, int ByteToLock, int BytesToWrite)
{
  void* Region1     = (uint8_t*)AudioRingBuffer->Data + ByteToLock;
  int   Region1Size = BytesToWrite;
  if(Region1Size + ByteToLock > AudioRingBuffer->Size)
  {
    Region1Size = AudioRingBuffer->Size - ByteToLock;
  }
  void* Region2     = AudioRingBuffer->Data;
  int   Region2Size = BytesToWrite - Region1Size;

  uint8_t* DestSample = (uint8_t*)Region1;
  for(int ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++)
  {
    *DestSample++ = 0;
  }

  DestSample = (uint8_t*)Region2;
  for(int ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++)
  {
    *DestSample++ = 0;
  }
}

void
FillSoundBuffer(Audio::audio_ring_buffer* AudioRingBuffer, Audio::sound_output* SoundOutput,
                int ByteToLock, int BytesToWrite, game_sound_output_buffer* SourceBuffer)
{
  void* Region1     = (uint8_t*)AudioRingBuffer->Data + ByteToLock;
  int   Region1Size = BytesToWrite;
  if(Region1Size + ByteToLock > AudioRingBuffer->Size)
  {
    Region1Size = AudioRingBuffer->Size - ByteToLock;
  }
  void* Region2     = AudioRingBuffer->Data;
  int   Region2Size = BytesToWrite - Region1Size;

  int16_t* DestSample         = (int16_t*)Region1;
  int16_t* SourceSample       = SourceBuffer->Samples;
  int      Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;

  for(int i = 0; i < Region1SampleCount; i++)
  {
    *DestSample++ = *SourceSample++;
    *DestSample++ = *SourceSample++;
    SoundOutput->RunningSampleIndex++;
  }

  DestSample             = (int16_t*)Region2;
  int Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;

  for(int i = 0; i < Region2SampleCount; i++)
  {
    *DestSample++ = *SourceSample++;
    *DestSample++ = *SourceSample++;
    SoundOutput->RunningSampleIndex++;
  }
}

int
main(int argc, char* argv[])
{
  SDL_Window* Window = nullptr;
  if(!Init(&Window))
  {
    return -1;
  }

  if(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF) !=
     (IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF))
  {
    printf("Image loading could not be initialized!\nError: %s\n", SDL_GetError());
  }

  // Init TrueType Font API
  if(TTF_Init() == -1)
  {
    printf("TrueType Fonts could not be initialized!\n");
    return -1;
  }

  game_memory GameMemory;
  {
    GameMemory.TemporaryMemorySize  = Mibibytes(50);
    GameMemory.PersistentMemorySize = Mibibytes(300);

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
  if(!NewInput.IsMouseInEditorMode)
  {
    SDL_WarpMouseInWindow(Window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
  }

  ProcessInput(&OldInput, &NewInput, &Event);
  OldInput = NewInput;

  // Audio preparation

  Audio::sound_output SoundOutput = {};

  SoundOutput.SamplesPerSecond    = 48000;
  SoundOutput.ToneVolume          = 0;
  SoundOutput.BytesPerSample      = sizeof(int16_t) * 2;
  SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
  SoundOutput.LatencySampleCount  = SoundOutput.SamplesPerSecond / 30;

  SDL_AudioDeviceID        AudioDevice;
  Audio::audio_ring_buffer AudioRingBuffer;
  if(!InitAudio(&AudioDevice, &AudioRingBuffer, SoundOutput.SamplesPerSecond,
                SoundOutput.SecondaryBufferSize))
  {
    return -1;
  }
  ClearSoundBuffer(&AudioRingBuffer, 0, SoundOutput.SecondaryBufferSize);
  SDL_PauseAudioDevice(AudioDevice, 0);
  SoundOutput.RunningSampleIndex = AudioRingBuffer.WriteCursor / SoundOutput.BytesPerSample;

  // Main loop
  // TODO(Rytis): Clean up audio preparation and computation code
  while(true)
  {
    //---------INPUT MANAGEMENT
    ProcessInput(&OldInput, &NewInput, &Event);
    if(NewInput.Escape.EndedDown)
    {
      break;
    }
    if(NewInput.Tab.EndedDown && NewInput.Tab.Changed)
    {
      NewInput.IsMouseInEditorMode = !NewInput.IsMouseInEditorMode;
      if(NewInput.IsMouseInEditorMode)
      {
        SDL_ShowCursor(SDL_ENABLE);
      }
      else
      {
        SDL_ShowCursor(SDL_DISABLE);
      }
    }
    // Should go after mouse mode switch to avoid jerking the camera
    if(!NewInput.IsMouseInEditorMode)
    {
      SDL_WarpMouseInWindow(Window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    }
    // Set noramlised mouse after all mouse editing is done
    {
      // 0.0f-1.0f mouse coords
      NewInput.NormMouseX  = (float)NewInput.MouseX / (float)(SCREEN_WIDTH);
      NewInput.NormMouseY  = (float)NewInput.MouseY / (float)(SCREEN_HEIGHT);
      NewInput.NormdMouseX = (float)NewInput.dMouseX / (float)(SCREEN_WIDTH);
      NewInput.NormdMouseY = (float)NewInput.dMouseY / (float)(SCREEN_HEIGHT);
    }
    //---------END INPUT MANAGEMENT

    // Audio computation
    int ByteToLock =
      (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % AudioRingBuffer.Size;
    int TargetCursor = ((AudioRingBuffer.PlayCursor +
                         (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
                        AudioRingBuffer.Size);
    int BytesToWrite;

    if(ByteToLock > TargetCursor)
    {
      BytesToWrite = AudioRingBuffer.Size - ByteToLock;
      BytesToWrite += TargetCursor;
    }
    else
    {
      BytesToWrite = TargetCursor - ByteToLock;
    }

    int16_t                  Samples[48000 * 2];
    game_sound_output_buffer SoundBuffer = {};

    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
    SoundBuffer.SampleCount      = BytesToWrite / SoundOutput.BytesPerSample;
    SoundBuffer.Samples          = Samples;
    // Audio computation done

    NewInput.dt = 0.001f * (float)FRAME_TIME_MS;
    if(NewInput.LeftCtrl.EndedDown)
    {
      NewInput.dt *= SLOW_MOTION_COEFFICIENT;
    }

    GameUpdateAndRender(GameMemory, &NewInput);
    GameGetSoundSamples(GameMemory, &SoundBuffer);

    SDL_LockAudioDevice(AudioDevice);
    FillSoundBuffer(&AudioRingBuffer, &SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
    SDL_UnlockAudioDevice(AudioDevice);

    SDL_GL_SwapWindow(Window);
    SDL_Delay(FRAME_TIME_MS);

    OldInput = NewInput;
  }

  SDL_CloseAudioDevice(AudioDevice);
  free(GameMemory.TemporaryMemory);
  free(GameMemory.PersistentMemory);
  IMG_Quit();
  TTF_Quit();
  SDL_DestroyWindow(Window);
  SDL_Quit();

  return 0;
}
