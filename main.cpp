#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#include "load_obj.h"
#include "load_shader.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"

struct sdl_audio_ring_buffer
{
  int   Size;
  int   WriteCursor;
  int   PlayCursor;
  void* Data;
};

static sdl_audio_ring_buffer AudioRingBuffer;

void
SDLAudioCallback(void* UserData, uint8_t* AudioData, int Length)
{
  assert(UserData);
  assert(AudioData);
  sdl_audio_ring_buffer* RingBuffer = (sdl_audio_ring_buffer*)UserData;

  int Region1Size = Length;
  int Region2Size = 0;
  if(RingBuffer->PlayCursor + Length > RingBuffer->Size)
  {
    Region1Size = RingBuffer->Size - RingBuffer->PlayCursor;
    Region2Size = Length - Region1Size;
  }
  memcpy(AudioData, (uint8_t*)(RingBuffer->Data) + RingBuffer->PlayCursor, Region1Size);
  memcpy(&AudioData[Region1Size], RingBuffer->Data, Region2Size);
  RingBuffer->PlayCursor  = (RingBuffer->PlayCursor + Length) % RingBuffer->Size;
  RingBuffer->WriteCursor = (RingBuffer->PlayCursor + 2048) % RingBuffer->Size;
}

bool
InitAudio(SDL_AudioDeviceID* AudioDevice, int SamplesPerSecond, int BufferSize)
{
  AudioRingBuffer.Size        = BufferSize;
  AudioRingBuffer.Data        = malloc(AudioRingBuffer.Size);
  AudioRingBuffer.PlayCursor  = 0;
  AudioRingBuffer.WriteCursor = 0;

  SDL_AudioSpec AudioSettings = { 0 };

  AudioSettings.freq     = SamplesPerSecond;
  AudioSettings.format   = AUDIO_S16LSB; // Check actual format type.
  AudioSettings.channels = 2;
  AudioSettings.samples  = 1024;
  AudioSettings.callback = &SDLAudioCallback;
  AudioSettings.userdata = &AudioRingBuffer;

  *AudioDevice = SDL_OpenAudioDevice(NULL, 0, &AudioSettings, 0, 0);

  if(*AudioDevice == 0)
  {
    printf("Failed to get an audio device: %s\n", SDL_GetError());
    return false;
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

  int      SamplesPerSecond     = 48000;
  int      ToneHz               = 256;
  int16_t  ToneVolume           = 10000;
  uint32_t RunningSampleIndex   = 0;
  int      SquareWavePeriod     = SamplesPerSecond / ToneHz;
  int      HalfSquareWavePeriod = SquareWavePeriod / 2;
  int      BytesPerSample       = sizeof(int16_t) * 2;
  int      SecondaryBufferSize  = SamplesPerSecond * BytesPerSample;
  bool     SoundIsPlaying       = true;

  SDL_AudioDeviceID AudioDevice;
  if(!InitAudio(&AudioDevice, SamplesPerSecond, SecondaryBufferSize))
  {
    return -1;
  }
  SDL_PauseAudioDevice(AudioDevice, 0);

  // Asset loading
  Mesh::mesh Mesh = Mesh::LoadOBJMesh("./data/armadillo.obj\0", false, true, false);
  if(!Mesh.VerticeCount)
  {
    printf("ReadOBJ error: no vertices read\n");
    return -1;
  }
  else
  {
    SetUpMesh(&Mesh);
  }

  int ShaderVertexColor = Shader::LoadShader("./shaders/color");
  if(ShaderVertexColor < 0)
  {
    printf("Shader loading failed!\n");
  }

  int ShaderWireframe = Shader::LoadShader("./shaders/wireframe");
  if(ShaderVertexColor < 0)
  {
    printf("Shader loading failed!\n");
  }

  // \Asset loading

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

    SDL_LockAudioDevice(AudioDevice);

    int ByteToLock = RunningSampleIndex * BytesPerSample % SecondaryBufferSize;
    int BytesToWrite;
    if(ByteToLock > AudioRingBuffer.PlayCursor)
    {
      BytesToWrite = (SecondaryBufferSize - ByteToLock);
      BytesToWrite += AudioRingBuffer.PlayCursor;
    }
    else
    {
      BytesToWrite = AudioRingBuffer.PlayCursor - ByteToLock;
    }

    void* Region1     = (uint8_t*)AudioRingBuffer.Data + ByteToLock;
    int   Region1Size = BytesToWrite;
    if(Region1Size + ByteToLock > SecondaryBufferSize)
    {
      Region1Size = SecondaryBufferSize - ByteToLock;
    }
    void* Region2     = AudioRingBuffer.Data;
    int   Region2Size = BytesToWrite - Region1Size;

    int16_t* SampleOut          = (int16_t*)Region1;
    int      Region1SampleCount = Region1Size / BytesPerSample;

    for(int i = 0; i < Region1SampleCount; i++)
    {
      int16_t SampleValue =
        ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
    }

    SampleOut              = (int16_t*)Region2;
    int Region2SampleCount = Region2Size / BytesPerSample;

    for(int i = 0; i < Region2SampleCount; i++)
    {
      int16_t SampleValue =
        ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
    }

    SDL_UnlockAudioDevice(AudioDevice);

    if(!SoundIsPlaying)
    {
      SDL_PauseAudioDevice(AudioDevice, 0);
      SoundIsPlaying = true;
    }

    // Update
    mat4 ModelMatrix =
      Math::MulMat4(Math::Mat4RotateY(180 + AngleDeg * 0.2f), Math::Mat4Scale(0.25f));
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

#if 0
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

  SDL_CloseAudioDevice(AudioDevice);
  SDL_DestroyWindow(Window);
  SDL_Quit();
  return 0;
}
