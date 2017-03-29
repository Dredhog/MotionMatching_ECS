#pragma once

#include <stdint.h>

inline float
ClampFloat(float Min, float T, float Max)
{
  if(T < Min)
  {
    return Min;
  }
  if(T > Max)
  {
    return Max;
  }
  return T;
}

inline int32_t
ClampInt32InIn(int32_t Min, int32_t T, int32_t Max)
{
  if(T < Min)
  {
    return Min;
  }
  if(T > Max)
  {
    return Max;
  }
  return T;
}

inline float
AbsFloat(float F)
{
  return (F < 0) ? -F : F;
}

inline bool
FloatsEqualByThreshold(float A, float B, float Threshold)
{
  if(AbsFloat(A - B) <= Threshold)
  {
    return true;
  }
  return false;
}

inline bool
FloatGreaterByThreshold(float A, float B, float Threshold)
{
  if((A - Threshold) > B)
  {
    return true;
  }
  return false;
}

