#include "profile.h"

debug_frame_cycle_counter GLOBAL_DEBUG_FRAME_CYCLE_TABLE[PROFILE_MAX_FRAME_COUNT+1];
debug_cycle_counter GLOBAL_DEBUG_CYCLE_TABLE[PROFILE_MAX_FRAME_COUNT+1][ArrayCount(DEBUG_TABLE_NAMES)];
debug_cycle_counter_event GLOBAL_DEBUG_CYCLE_EVENT_TABLE[PROFILE_MAX_FRAME_COUNT+1][PROFILE_MAX_EVENTS_PER_FRAME];
int GLOBAL_DEBUG_FRAME_EVENT_COUNT_TABLE[PROFILE_MAX_FRAME_COUNT+1];
int g_CurrentProfileBufferFrameIndex = 0;
int g_SavedCurrentFrameIndex = 0;
int g_CurrentFrameEventDepth = 0;
int g_CurrentFrameEventCount = 0;


auto_close_scope_wrapper::auto_close_scope_wrapper(int32_t BlockEnumValue)
{
	this->EnumValue = BlockEnumValue;
	this->EventIndexInFrame = g_CurrentFrameEventCount;
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].StartCycleCount = __rdtsc();
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].EventDepth = g_CurrentFrameEventDepth;
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].NameTableIndex = this->EnumValue;
	++g_CurrentFrameEventCount;
	++g_CurrentFrameEventDepth;
}

auto_close_scope_wrapper::~auto_close_scope_wrapper()
{
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][this->EventIndexInFrame].EndCycleCount = __rdtsc();
	GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][this->EnumValue].CycleCount += GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][this->EventIndexInFrame].EndCycleCount -
																																														GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].StartCycleCount;
	GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][this->EnumValue].Calls++;
	--g_CurrentFrameEventDepth;
}
