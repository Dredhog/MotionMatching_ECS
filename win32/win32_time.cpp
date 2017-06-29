#include <windows.h>
#include <stdint.h>

int64_t g_PerformanceFrequency = 0;

namespace Platform
{
  float GetTimeInSeconds()
  {
    if(g_PerformanceFrequency == 0){
      LARGE_INTEGER PerformanceFrequencyResult;
      QueryPerformanceFrequency(&PerformanceFrequencyResult);
      int64_t PerformanceFrequency = PerformanceFrequencyResult.QuadPart;
    }

    LARGE_INTEGER CurrentPerformanceCounter;
    QueryPerformanceCounter(&CurrentPerformanceCounter);
    return (float)CurrentPerformanceCounter.QuadPart/(float)g_PerformanceFrequency;
  }
}
