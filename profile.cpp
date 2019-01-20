#include "profile.h"

debug_cycle_counter GLOBAL_DEBUG_CYCLE_TABLE[PROFILE_MAX_FRAME_COUNT+1][ArrayCount(DEBUG_TABLE_NAMES)];
int g_CurrentProfileBufferFrameIndex = 0;
int g_SavedCurrentFrameIndex = 0;
