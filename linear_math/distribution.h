#pragma once

#include <math.h>
#include <float.h>

#define MATH_PI 3.1415926535

int32_t
Abs(int32_t X)
{
    return X < 0 ? -X : X;
}

float
Abs(float X)
{
    return X < 0 ? -X : X;
}

float
GaussianDistribution1D(int32_t X, float StdDev)
{
    float Denom = (float)sqrt(2 * MATH_PI) * StdDev;
    assert(Abs(Denom) > FLT_EPSILON);

    float ExpDenom = 2 * StdDev * StdDev;
    assert(Abs(ExpDenom) > FLT_EPSILON);
    
    return expf(-((X * X) / ExpDenom)) / Denom;
}

void
GenerateGaussianBlurKernel(float* Kernel, uint32_t KernelSize, float StdDev)
{
    uint32_t Half = KernelSize / 2;
    if(KernelSize % 2 == 1)
    {
        ++Half;
    }

    for(uint32_t i = 0; i < Half; ++i)
    {
        Kernel[i] = GaussianDistribution1D(i + 1 - Half, StdDev);
        Kernel[KernelSize - 1 - i] = Kernel[i];
    }

    float Sum = 0.0f;
    for(uint32_t i = 0; i < KernelSize; ++i)
    {
        Sum += Kernel[i];
    }

    for(uint32_t i = 0; i < KernelSize; ++i)
    {
        Kernel[i] /= Sum;
    }
}
