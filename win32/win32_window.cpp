#include <stdio.h>
#include <ShellScalingAPI.h>
#include "common.h"

void
Platform::SetHighDPIAwareness()
{
  if(SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE) != S_OK)
  {
    printf("DPI awareness was not set!\n");
  }
}
