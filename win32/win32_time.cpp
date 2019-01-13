#include <windows.h>
#include <stdint.h>
#include <SDL2/SDL.h>

// TODO(rytis): A temporary solution for an oddly working Windows native tick counter.
// This is still not good enough, for sure, but will be used for a while.
#define USE_SDL_TIMER 1

int64_t g_PerformanceFrequency = 0;

namespace Platform
{
  float GetTimeInSeconds()
  {
#if USE_SDL_TIMER
    return SDL_GetTicks() / 1000.0f;
#else
    if(g_PerformanceFrequency == 0){
      LARGE_INTEGER PerformanceFrequencyResult;
      QueryPerformanceFrequency(&PerformanceFrequencyResult);
      g_PerformanceFrequency = PerformanceFrequencyResult.QuadPart;
    }

    LARGE_INTEGER CurrentPerformanceCounter;
    QueryPerformanceCounter(&CurrentPerformanceCounter);
    return (float)CurrentPerformanceCounter.QuadPart/(float)g_PerformanceFrequency;
#endif
  }
}
