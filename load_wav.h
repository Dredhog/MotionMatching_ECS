#pragma once

#include <math.h>

#define Pi 3.14159265359f

namespace Audio
{
  struct audio_ring_buffer
  {
    int   Size;
    int   WriteCursor;
    int   PlayCursor;
    void* Data;
  };

  struct sound_output
  {
    int      SamplesPerSecond;
    int16_t  ToneVolume;
    uint32_t RunningSampleIndex;
    int      BytesPerSample;
    int      SecondaryBufferSize;
    int      LatencySampleCount;
  };

  void
  SDLAudioCallback(void* UserData, uint8_t* AudioData, int Length)
  {
    assert(UserData);
    assert(AudioData);
    audio_ring_buffer* RingBuffer = (audio_ring_buffer*)UserData;

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

  void
  LoadWAV(loaded_wav* AudioBuffer, char* FileName)
  {
    SDL_AudioSpec FileSpec;
    uint8_t*      AudioData;
    uint32_t      AudioDataLength;
    SDL_AudioCVT  ConversionSpec;

    if(SDL_LoadWAV(FileName, &FileSpec, &AudioData, &AudioDataLength) == 0)
    {
      printf("Could not open audio file %s: %s\n", FileName, SDL_GetError());
    }

    SDL_BuildAudioCVT(&ConversionSpec, FileSpec.format, FileSpec.channels, FileSpec.freq,
                      AUDIO_S16LSB, 2, 48000);
    assert(ConversionSpec.needed);

    ConversionSpec.buf = (uint8_t*)malloc(AudioDataLength * ConversionSpec.len_mult);
    ConversionSpec.len = AudioDataLength;
    memcpy(ConversionSpec.buf, AudioData, AudioDataLength);
    free(AudioData);

    SDL_ConvertAudio(&ConversionSpec);

    AudioBuffer->AudioLength      = ConversionSpec.len_cvt;
    AudioBuffer->AudioData        = (int16_t*)ConversionSpec.buf;
    AudioBuffer->AudioSampleIndex = 0;
  }

  void
  GameOutputSound(game_sound_output_buffer* SoundBuffer, loaded_wav* AudioSource)
  {

    int16_t* SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
    {
      *SampleOut++ = AudioSource->AudioData[AudioSource->AudioSampleIndex++];
      *SampleOut++ = AudioSource->AudioData[AudioSource->AudioSampleIndex++];
    }
  }
}
