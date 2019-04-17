#pragma once

#include <stdint.h>

inline float
MaxFloat(float A, float B)
{
  if(A > B)
  {
    return A;
  }
  return B;
}

inline float
MinFloat(float A, float B)
{
  if(A < B)
  {
    return A;
  }
  return B;
}

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
MinInt32(int32_t A, int32_t B)
{
  if(A < B)
  {
    return A;
  }
  return B;
}

inline int32_t
ArgMin(float a, float b, float c)
{
  if(a < b)
  {
    return (a < c) ? 0 : 2;
  }
  else
  {
    return (b < c) ? 1 : 2;
  }
}

inline int32_t
MaxInt32(int32_t A, int32_t B)
{
  if(A > B)
  {
    return A;
  }
  return B;
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

inline int32_t
ClampMinInt32(int32_t Min, int32_t T)
{
  if(T < Min)
  {
    return Min;
  }
  return T;
}

/*inline float
CeilFloat(float F)
{
	if((F - (float)((int64_t)F)) > 0.0f)
	{
		++F;
	}
  return F;
}*/

inline float
AbsFloat(float F)
{
  return (F < 0) ? -F : F;
}

inline float
SignFloat(float F)
{
	return (F < 0) ? -1.0f : 1.0f;
}

inline int32_t
AbsInt32(int32_t T)
{
  return (T < 0) ? -T : T;
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

#define ARRAY_SIZE(Array) sizeof(Array) / sizeof((Array)[0])
