#include "../common.h"
#include <time.h>

float
Platform::GetTimeInSeconds()
{
  struct timespec CurrentTime;
  clock_gettime(CLOCK_MONOTONIC_RAW, &CurrentTime);
  float Result = (float)CurrentTime.tv_sec + (float)CurrentTime.tv_nsec / 1e9f;
  return Result;
}
