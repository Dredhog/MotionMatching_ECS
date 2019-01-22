#pragma once

#define DEBUG_PROFILING 1

#if DEBUG_PROFILING  
#define PROFILE_MAX_FRAME_COUNT 500
#define PROFILE_MAX_EVENTS_PER_FRAME 1000


#include "common.h"
#include "linear_math/vector.h"

#if defined( __WIN32__ ) || defined( _WIN32 ) || defined( __WIN64__ ) || defined( _WIN64 ) || defined( WIN32 )
#include <intrin.h>
#define _rdtsc __rdtsc
#elif defined(__linux__) || defined( LINUX )
#include <x86intrin.h>
#else
#error
#endif
enum{
	DEBUG_FirstInit,
	DEBUG_FilesystemUpdate,
	DEBUG_Update,
	DEBUG_Editor,
	DEBUG_GUI,
	DEBUG_SelectionDrawing,
	DEBUG_EntityCreation,
	DEBUG_Physics,
	DEBUG_Render,
	DEBUG_PostProcessing,
	DEBUG_DebugDrawingSubmission,
	DEBUG_LoadTextTexture,
	DEBUG_ImportScene,
	DEBUG_LoadTexture,
	DEBUG_LoadModel,
	DEBUG_LoadShader,
	DEBUG_LoadMaterial,
};

struct debug_frame_cycle_counter {
	uint64_t FrameStart;
	uint64_t FrameEnd;
};

struct debug_cycle_counter {
	uint64_t CycleCount;
	uint64_t Calls;
};

struct debug_cycle_counter_event {
	uint64_t StartCycleCount;
	uint64_t EndCycleCount;
	int EventDepth;
	int NameTableIndex;
};

const char DEBUG_TABLE_NAMES[][40] = {
	"FirstInit",
	"FilesystemUpdate",
	"Update",
	"Editor",
	"GUI",
	"SelectionDrawing",
	"EntityCreation",
	"Physics",
	"Render",
	"PostProcessing",
	"DebugDrawingSubmission",
	"LoadTextTexture",
	"ImportScene",
	"LoadTexture",
	"LoadModel",
	"LoadShader",
	"LoadMaterial",
};

const vec3 DEBUG_ENTRY_COLORS[] = {
	vec3{1, 0, 0},
	vec3{1, 0.4f, 0.4f},
	vec3{0.1f, 0.8f, 0.2f},
	vec3{0.5f, 1, 0.5f},
	vec3{0.2f, 0.7f, 0.5f},
	vec3{0,0,1},
	vec3{1,1,0},
	vec3{0.2f, 0.2f, 0.2f},
	vec3{0, 1, 1},
	vec3{1, 1, 0},
	vec3{0.7f, 0.7f, 0.7f},
	vec3{1, 0, 1},
	vec3{1, 0, 0},
	vec3{0, 0.5, 1},
	vec3{0.5f, 0.5f, 0},
	vec3{1, 0.6f, 0.6f},
	vec3{1, 0.6f, 0},
};

extern debug_frame_cycle_counter GLOBAL_DEBUG_FRAME_CYCLE_TABLE[PROFILE_MAX_FRAME_COUNT+1];
extern debug_cycle_counter GLOBAL_DEBUG_CYCLE_TABLE[PROFILE_MAX_FRAME_COUNT+1][ArrayCount(DEBUG_TABLE_NAMES)];
extern debug_cycle_counter_event GLOBAL_DEBUG_CYCLE_EVENT_TABLE[PROFILE_MAX_FRAME_COUNT+1][PROFILE_MAX_EVENTS_PER_FRAME];
extern int GLOBAL_DEBUG_FRAME_EVENT_COUNT_TABLE[PROFILE_MAX_FRAME_COUNT+1];
extern int g_CurrentProfileBufferFrameIndex;
extern int g_SavedCurrentFrameIndex;
extern int g_CurrentFrameEventDepth;
extern int g_CurrentFrameEventCount;

#define BEGIN_FRAME() \
	for(int i = 0; i < ArrayCount(DEBUG_TABLE_NAMES); i++)\
	{\
		GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][i] = {};\
	}\
	GLOBAL_DEBUG_FRAME_EVENT_COUNT_TABLE[g_CurrentProfileBufferFrameIndex] = 0;\
	g_CurrentFrameEventCount = 0;\
	GLOBAL_DEBUG_FRAME_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex].FrameStart = _rdtsc();

#define END_FRAME()\
	assert(g_CurrentFrameEventCount <= PROFILE_MAX_EVENTS_PER_FRAME);\
	assert(g_CurrentFrameEventDepth == 0);\
	GLOBAL_DEBUG_FRAME_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex].FrameEnd = _rdtsc();\
	GLOBAL_DEBUG_FRAME_EVENT_COUNT_TABLE[g_CurrentProfileBufferFrameIndex] = g_CurrentFrameEventCount;\
	if(g_CurrentProfileBufferFrameIndex != PROFILE_MAX_FRAME_COUNT)\
	{\
		g_CurrentProfileBufferFrameIndex = (g_CurrentProfileBufferFrameIndex + 1)%PROFILE_MAX_FRAME_COUNT;\
	}\

#define BEGIN_TIMED_BLOCK(ID) \
	uint64_t StartCycleCount##ID = _rdtsc();\
	int      FrameEventIndex##ID = g_CurrentFrameEventCount;\
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].StartCycleCount = StartCycleCount##ID;\
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].EventDepth = g_CurrentFrameEventDepth;\
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].NameTableIndex = DEBUG_##ID;\
	++g_CurrentFrameEventCount;\
  ++g_CurrentFrameEventDepth;\
  if(g_CurrentProfileBufferFrameIndex == PROFILE_MAX_FRAME_COUNT) { g_CurrentFrameEventCount--; }

#define END_TIMED_BLOCK(ID)\
	uint64_t EndCycleCount##ID = _rdtsc();\
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][FrameEventIndex##ID].EndCycleCount = EndCycleCount##ID;\
	GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][DEBUG_##ID].CycleCount += EndCycleCount##ID - StartCycleCount##ID;\
	GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][DEBUG_##ID].Calls++;\
  --g_CurrentFrameEventDepth;

#define PAUSE_PROFILE()\
	g_SavedCurrentFrameIndex = g_CurrentProfileBufferFrameIndex;\
	g_CurrentProfileBufferFrameIndex = PROFILE_MAX_FRAME_COUNT;

#define RESUME_PROFILE()\
	g_CurrentProfileBufferFrameIndex = g_SavedCurrentFrameIndex;

#else

#define BEGIN_FRAME()
#define END_FRAME()
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)
#define PAUSE_PROFILE()
#define RESUME_PROFILE()

#endif //DEGUB_PROFILING
