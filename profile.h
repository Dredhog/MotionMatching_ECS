#pragma once

#define DEBUG_PROFILING 1
#define ArrayCount(Array) sizeof((Array)) / sizeof(Array[0])

#if DEBUG_PROFILING  
#define PROFILE_MAX_FRAME_COUNT 500
#define PROFILE_MAX_EVENTS_PER_FRAME 200

#include <stdint.h>

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
	DEBUG_PartitionMemory,
	DEBUG_LoadInitialResources,
	DEBUG_GBufferPass,
	DEBUG_SSAOPass,
	DEBUG_ShadowmapPass,
	DEBUG_VolumetricScatteringPass,
	DEBUG_Cubemap,
	DEBUG_RenderScene,
	DEBUG_RenderSelection,
	DEBUG_RenderPreview,
	DEBUG_SimulateDynamics,
	DEBUG_CopyDataToPhysicsWorld,
	DEBUG_CopyDataFromPhysicsWorld,
	DEBUG_AnimationSystem,
	DEBUG_SAT,
	DEBUG_ODE,
  DEBUG_LoadSizedFont,
  DEBUG_LoadFont,
  DEBUG_SearchForDesiredTexture,
	DEBUG_FindCacheLineToOccupy,
  DEBUG_GetBestMatchingFont,
  DEBUG_GetTextSize,
  DEBUG_GetTextTextureID,
  DEBUG_ResetCache,
  DEBUG_SetMaterial,
  DEBUG_UpdateAssetPathLists,
  DEBUG_DeleteUnused,
  DEBUG_ReloadModified,
	DEBUG_ReadEntireFile,
	DEBUG_WriteEntireFile,
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
	"PartitionMemory",
	"LoadInitialResources",
	"GBufferPass",
	"SSAOPass",
	"ShadowmapPass",
	"VolumetricScatteringPass",
	"Cubemap",
	"RenderScene",
	"RenderSelection",
	"RenderPreview",
	"SimulateDynamics",
	"CopyDataToPhysicsWorld",
	"CopyDataFromPhysicsWorld",
	"AnimationSystem",
	"SAT",
	"ODE",
  "LoadSizedFont",
  "LoadFont",
  "SearchForDesiredTexture",
  "FindCacheLineToOccupy",
  "GetBestMatchingFont",
  "GetTextSize",
  "GetTextTextureID",
  "ResetCache",
  "SetMaterial",
  "UpdateAssetPathLists",
  "DeleteUnused",
  "ReloadModified",
	"ReadEntireFile",
	"WriteEntireFile",
};

const float DEBUG_ENTRY_COLORS[][3]  = {
	{1, 0, 0},
	{1, 0.4f, 0.4f},
	{0.1f, 0.8f, 0.2f},
	{0.5f, 1, 0.5f},
	{0.2f, 0.7f, 0.5f},
	{0,0,1},
	{1,1,0},
	{0.2f, 0.2f, 0.2f},
	{0, 1, 1},
	{1, 1, 0},
	{0.7f, 0.7f, 0.7f},
	{1, 0, 1},
	{1, 0, 0},
	{0, 0.5, 1},
	{0.5f, 0.5f, 0},
	{1, 0.6f, 0.6f},
	{1, 0.6f, 0},
	{0.5f, 1, 0.5f},
	{0.2f, 0.7f, 0.5f},
	{0,0,1},
	{1,1,0},
	{0.2f, 0.2f, 0.2f},
	{0, 1, 1},
	{1, 1, 0},
	{0.7f, 0.7f, 0.7f},
	{1, 0, 1},
	{1, 0.4f, 0.4f},
	{0.1f, 0.8f, 0.2f},
	{0.5f, 1, 0.5f},
	{0.2f, 0.7f, 0.5f},
	{0,0,1},
	{0.6f, 0.2f, 0.2f},
	{0, 1, 1},
	{1, 0, 0},
	{1, 0.5, 1},
	{0.5f, 0.5f, 0},
	{1, 0.6f, 0.6f},
	{1, 0.6f, 0},
	{0.5f, 1, 0.5f},
	{0.2f, 0.7f, 0.5f},
	{0,0,1},
	{0.1f,0.8f,0.2f},
	{0,0,1},
	{1,1,0},
	{0.5f,0.2f,0.5f},
	{0.6f, 0.5f,0.3f},
	{1, 0.2f,0.3f},
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
	int      FrameEventIndex##ID = g_CurrentFrameEventCount;\
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].StartCycleCount = _rdtsc();\
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].EventDepth = g_CurrentFrameEventDepth;\
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].NameTableIndex = DEBUG_##ID;\
	++g_CurrentFrameEventCount;\
  ++g_CurrentFrameEventDepth;\
  if(g_CurrentProfileBufferFrameIndex == PROFILE_MAX_FRAME_COUNT) { g_CurrentFrameEventCount--; }

#define END_TIMED_BLOCK(ID)\
	uint64_t EndCycleCount##ID = _rdtsc();\
	GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][FrameEventIndex##ID].EndCycleCount = EndCycleCount##ID;\
	GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][DEBUG_##ID].CycleCount += EndCycleCount##ID - GLOBAL_DEBUG_CYCLE_EVENT_TABLE[g_CurrentProfileBufferFrameIndex][g_CurrentFrameEventCount].StartCycleCount;\
	GLOBAL_DEBUG_CYCLE_TABLE[g_CurrentProfileBufferFrameIndex][DEBUG_##ID].Calls++;\
  --g_CurrentFrameEventDepth;

struct auto_close_scope_wrapper
{
	int32_t EnumValue;
	int32_t EventIndexInFrame;

	auto_close_scope_wrapper(int32_t BlockEnumValue);
	~auto_close_scope_wrapper();
};

#define TIMED_BLOCK_(ID, Line) auto_close_scope_wrapper ID##Line(DEBUG_##ID)
#define TIMED_BLOCK(ID) auto_close_scope_wrapper ID##__LINE__(DEBUG_##ID)
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
