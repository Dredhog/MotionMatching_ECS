#pragma once

#define DEBUG_PROFILING 1

#if DEBUG_PROFILING  
#define PROFILE_MAX_FRAME_COUNT 500

#include <x86intrin.h>
#include "common.h"
enum{
	DEBUG_FirstInit,
	DEBUG_FilesystemUpdate,
	DEBUG_Update,
	DEBUG_Editor,
	DEBUG_GUI,
	DEBUG_SelectionDrawing,
	DEBUG_EntityCreation,
	DEBUG_CameraUpdate,
	DEBUG_Physics,
	DEBUG_Render,
	DEBUG_PostProcessing,
	DEBUG_DebugDrawingSubmission,
};
struct debug_cycle_counter {
	uint64_t CycleCount;
	uint64_t Calls;
};
const char DEBUG_TABLE_NAMES[][40] = {
	"FirstInit             ",
	"FilesystemUpdate      ",
	"Update                ",
	"Editor                ",
	"GUI                   ",
	"SelectionDrawing      ",
	"EntityCreation        ",
	"CameraUpdate          ",
	"Physics               ",
	"Render                ",
	"PostProcessing        ",
	"DebugDrawingSubmission",
};
extern debug_cycle_counter GLOBAL_DEBUG_CYCLE_TABLE[PROFILE_MAX_FRAME_COUNT+1][ArrayCount(DEBUG_TABLE_NAMES)];
extern int g_CurrentProfileBufferFrameIndex;
extern int g_SavedCurrentFrameIndex;

#define CLEAR_DEBUG_CYCLE_TABLE_() for(int i = 0; i < ArrayCount(DEBUG_TABLE_NAMES); i++){ GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][i] = {};}
#define BEGIN_FRAME() CLEAR_DEBUG_CYCLE_TABLE_()
#define PAUSE_PROFILE() g_SavedCurrentFrameIndex = g_CurrentProfileBufferFrameIndex; g_CurrentProfileBufferFrameIndex = PROFILE_MAX_FRAME_COUNT;
#define RESUME_PROFILE() g_CurrentProfileBufferFrameIndex = g_SavedCurrentFrameIndex;
#define END_FRAME() if(g_CurrentProfileBufferFrameIndex != PROFILE_MAX_FRAME_COUNT){ g_CurrentProfileBufferFrameIndex = (g_CurrentProfileBufferFrameIndex + 1)%PROFILE_MAX_FRAME_COUNT;}
#define BEGIN_TIMED_BLOCK(ID) uint64_t StartCycleCount##ID = _rdtsc();
#define END_TIMED_BLOCK(ID) GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][DEBUG_##ID].CycleCount += _rdtsc() - StartCycleCount##ID; \
							GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][DEBUG_##ID].Calls++;

#else

#define BEGIN_FRAME()
#define END_FRAME()
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)

#endif //DEGUB_PROFILING
