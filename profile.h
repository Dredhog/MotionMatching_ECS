#pragma once

#define DEBUG_PROFILING 1

#if DEBUG_PROFILING

// Windows perf counter header
#if defined(__WIN32__) || defined(_WIN32) || defined(__WIN64__) || defined(_WIN64) || defined(WIN32)
#include <intrin.h>
#define _rdtsc __rdtsc

// Linux perf counter header
#elif defined(__linux__) || defined(LINUX)
#include <x86intrin.h>

#else
#error "compilation error: OS must be either linux or windows"
#endif

#include <stdint.h>

#define ARRAY_COUNT(Array) sizeof((Array)) / sizeof(Array[0])
#define PROFILE_MAX_FRAME_COUNT 500
#define PROFILE_MAX_TIMER_EVENTS_PER_FRAME 500

enum
{
  TIMER_NAME_FirstInit,
  TIMER_NAME_FilesystemUpdate,
  TIMER_NAME_Update,
  TIMER_NAME_Editor,
  TIMER_NAME_GUI,
  TIMER_NAME_SelectionDrawing,
  TIMER_NAME_EntityCreation,
  TIMER_NAME_Physics,
  TIMER_NAME_Render,
  TIMER_NAME_PostProcessing,
  TIMER_NAME_DebugDrawingSubmission,
  TIMER_NAME_LoadTextTexture,
  TIMER_NAME_ImportScene,
  TIMER_NAME_LoadTexture,
  TIMER_NAME_LoadModel,
  TIMER_NAME_LoadShader,
  TIMER_NAME_LoadMaterial,
  TIMER_NAME_PartitionMemory,
  TIMER_NAME_LoadInitialResources,
  TIMER_NAME_GBufferPass,
  TIMER_NAME_SSAOPass,
  TIMER_NAME_ShadowmapPass,
  TIMER_NAME_VolumetricScatteringPass,
  TIMER_NAME_Cubemap,
  TIMER_NAME_RenderScene,
  TIMER_NAME_RenderSelection,
  TIMER_NAME_RenderPreview,
  TIMER_NAME_SimulateDynamics,
  TIMER_NAME_CopyDataToPhysicsWorld,
  TIMER_NAME_CopyDataFromPhysicsWorld,
  TIMER_NAME_AnimationSystem,
  TIMER_NAME_SAT,
  TIMER_NAME_ODE,
  TIMER_NAME_LoadSizedFont,
  TIMER_NAME_LoadFont,
  TIMER_NAME_SearchForDesiredTexture,
  TIMER_NAME_FindCacheLineToOccupy,
  TIMER_NAME_GetBestMatchingFont,
  TIMER_NAME_GetTextSize,
  TIMER_NAME_GetTextTextureID,
  TIMER_NAME_ResetCache,
  TIMER_NAME_SetMaterial,
  TIMER_NAME_UpdateAssetPathLists,
  TIMER_NAME_DeleteUnused,
  TIMER_NAME_ReloadModified,
  TIMER_NAME_ReadEntireFile,
  TIMER_NAME_WriteEntireFile,
  TIMER_NAME_Count,
};

const char TIMER_NAME_TABLE[][TIMER_NAME_Count] = {
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

const float TIMER_UI_COLOR_TABLE[TIMER_NAME_Count][3] = {
  { 1, 0, 0 },          { 1, 0.4f, 0.4f },    { 0.1f, 0.8f, 0.2f }, { 0.5f, 1, 0.5f },
  { 0.2f, 0.7f, 0.5f }, { 0, 0, 1 },          { 1, 1, 0 },          { 0.2f, 0.2f, 0.2f },
  { 0, 1, 1 },          { 1, 1, 0 },          { 0.7f, 0.7f, 0.7f }, { 1, 0, 1 },
  { 1, 0, 0 },          { 0, 0.5, 1 },        { 0.5f, 0.5f, 0 },    { 1, 0.6f, 0.6f },
  { 1, 0.6f, 0 },       { 0.5f, 1, 0.5f },    { 0.2f, 0.7f, 0.5f }, { 0, 0, 1 },
  { 1, 1, 0 },          { 0.2f, 0.2f, 0.2f }, { 0, 1, 1 },          { 1, 1, 0 },
  { 0.7f, 0.7f, 0.7f }, { 1, 0, 1 },          { 1, 0.4f, 0.4f },    { 0.1f, 0.8f, 0.2f },
  { 0.5f, 1, 0.5f },    { 0.2f, 0.7f, 0.5f }, { 0, 0, 1 },          { 0.6f, 0.2f, 0.2f },
  { 0, 1, 1 },          { 1, 0, 0 },          { 1, 0.5, 1 },        { 0.5f, 0.5f, 0 },
  { 1, 0.6f, 0.6f },    { 1, 0.6f, 0 },       { 0.5f, 1, 0.5f },    { 0.2f, 0.7f, 0.5f },
  { 0, 0, 1 },          { 0.1f, 0.8f, 0.2f }, { 0, 0, 1 },          { 1, 1, 0 },
  { 0.5f, 0.2f, 0.5f }, { 0.6f, 0.5f, 0.3f }, { 1, 0.2f, 0.3f },
};

struct frame_endpoints
{
  uint64_t FrameStart;
  uint64_t FrameEnd;
};

struct timer_frame_summary
{
  uint64_t CycleCount;
  uint64_t Calls;
};

struct timer_event
{
  uint64_t StartCycleCount;
  uint64_t EndCycleCount;
  int      EventDepth;
  int      NameTableIndex;
};

extern frame_endpoints     GLOBAL_FRAME_ENDPOINT_TABLE[PROFILE_MAX_FRAME_COUNT + 1];
extern timer_frame_summary GLOBAL_TIMER_FRAME_SUMMARY_TABLE[PROFILE_MAX_FRAME_COUNT + 1]
                                                           [ARRAY_COUNT(TIMER_NAME_TABLE)];
extern timer_event GLOBAL_FRAME_TIMER_EVENT_TABLE[PROFILE_MAX_FRAME_COUNT + 1]
                                                 [PROFILE_MAX_TIMER_EVENTS_PER_FRAME];
extern int GLOBAL_TIMER_FRAME_EVENT_COUNT_TABLE[PROFILE_MAX_FRAME_COUNT + 1];
extern int g_CurrentProfilerFrameIndex;
extern int g_SavedCurrentFrameIndex;
extern int g_CurrentTimerEventDepth;
extern int g_CurrentTimerEventCount;

struct timer_event_autoclose_wrapper
{
  int32_t NameTableIndex;
  int32_t IndexInFrame;

  timer_event_autoclose_wrapper(int32_t NameTableIndex);
  ~timer_event_autoclose_wrapper();
};

#define FOR_ALL_NAMES(DO_FUNC)                                                                     \
  DO_FUNC(GeomPrePass)                                                                             \
  DO_FUNC(Shadowmapping) DO_FUNC(VolumetricLighting) DO_FUNC(SSAO) DO_FUNC(RenderScene)            \
    DO_FUNC(FXAA) DO_FUNC(PostProcessing) DO_FUNC(Bloom) DO_FUNC(MotionBlur) DO_FUNC(DepthOfField) \
      DO_FUNC(FlipScreenbuffer)

#define GENERATE_ENUM_NO_COMMA(Name) GPU_TIMER_##Name
#define GENERATE_ENUM(Name) GENERATE_ENUM_NO_COMMA(Name),
#define GENERATE_STRING(Name) #Name,
enum gpu_timer_type
{
  FOR_ALL_NAMES(GENERATE_ENUM) GENERATE_ENUM(EnumCount)
};
static const char* GPU_TIMER_NAME_TABLE[GENERATE_ENUM_NO_COMMA(EnumCount)] = { FOR_ALL_NAMES(
  GENERATE_STRING) };
#undef FOR_ALL_NAMES
#undef GENERATE_ENUM
#undef GENERATE_STRING
struct gpu_timer_event
{
  uint32_t ElapsedTime;
  bool     WasRunThisFrame;
};

extern gpu_timer_event GPU_TIMER_EVENT_TABLE[PROFILE_MAX_FRAME_COUNT + 1][GPU_TIMER_EnumCount];
extern uint32_t        GPU_QUERY_OBJECT_TABLE[GPU_TIMER_EnumCount];

#define TIME_GPU_EXECUTION 0

#if TIME_GPU_EXECUTION

#if 0
struct gpu_timer_event_autoclose_wrapper
{
	inline gpu_timer_event_autoclose_wrapper(int32_t TableIndex)
	{
		glBeginQuery(GL_TIME_ELAPSED, &GPU_QUERY_OBJECT_TABLE[TableIndex]);
	}

	inline ~gpu_timer_event_autoclose_wrapper()
	{
		glEndQuery(GL_TIME_ELAPSED);
	}
};
#endif
#define BEGIN_GPU_TIMED_BLOCK(ID)                                                                  \
  GPU_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][GPU_TIMER_##ID].WasRunThisFrame = true;       \
  glBeginQuery(GL_TIME_ELAPSED, GPU_QUERY_OBJECT_TABLE[GPU_TIMER_##ID]);

#define END_GPU_TIMED_BLOCK(ID) glEndQuery(GL_TIME_ELAPSED);

#define INIT_GPU_TIMERS() glGenQueries(GPU_TIMER_EnumCount, GPU_QUERY_OBJECT_TABLE)
#define READ_GPU_QUERY_TIMERS()                                                                    \
  for(int q_ind = 0; q_ind < GPU_TIMER_EnumCount; q_ind++)                                         \
  {                                                                                                \
    int PrevInd =                                                                                  \
      (g_CurrentProfilerFrameIndex - 1 + PROFILE_MAX_FRAME_COUNT) % PROFILE_MAX_FRAME_COUNT;       \
    if(GPU_TIMER_EVENT_TABLE[PrevInd][q_ind].WasRunThisFrame)                                      \
      glGetQueryObjectuiv(GPU_QUERY_OBJECT_TABLE[q_ind], GL_QUERY_RESULT,                          \
                          &GPU_TIMER_EVENT_TABLE[PrevInd][q_ind].ElapsedTime);                     \
  }
#define GPU_TIMED_BLOCK(ID) gpu_timer_event_autoclose_wrapper ID##__LINE__(GPU_TIMER_NAME_##ID)

#else

#define BEGIN_GPU_TIMED_BLOCK(ID)
#define END_GPU_TIMED_BLOCK(ID)
#define INIT_GPU_TIMERS()
#define READ_GPU_QUERY_TIMERS()
#define GPU_TIMED_BLOCK(ID)

#endif

#define TIMED_BLOCK(ID) timer_event_autoclose_wrapper ID##__LINE__(TIMER_NAME_##ID)

#define BEGIN_FRAME()                                                                              \
  for(int i = 0; i < ARRAY_COUNT(GPU_TIMER_NAME_TABLE); i++)                                       \
  {                                                                                                \
    GPU_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][i] = {};                                    \
  }                                                                                                \
  for(int i = 0; i < ARRAY_COUNT(TIMER_NAME_TABLE); i++)                                           \
  {                                                                                                \
    GLOBAL_TIMER_FRAME_SUMMARY_TABLE[g_CurrentProfilerFrameIndex][i] = {};                         \
  }                                                                                                \
  GLOBAL_TIMER_FRAME_EVENT_COUNT_TABLE[g_CurrentProfilerFrameIndex]   = 0;                         \
  g_CurrentTimerEventCount                                            = 0;                         \
  GLOBAL_FRAME_ENDPOINT_TABLE[g_CurrentProfilerFrameIndex].FrameStart = _rdtsc();

#define END_FRAME()                                                                                \
  assert(g_CurrentTimerEventCount <= PROFILE_MAX_TIMER_EVENTS_PER_FRAME);                          \
  assert(g_CurrentTimerEventDepth == 0);                                                           \
  GLOBAL_FRAME_ENDPOINT_TABLE[g_CurrentProfilerFrameIndex].FrameEnd = _rdtsc();                    \
  GLOBAL_TIMER_FRAME_EVENT_COUNT_TABLE[g_CurrentProfilerFrameIndex] = g_CurrentTimerEventCount;    \
  if(g_CurrentProfilerFrameIndex != PROFILE_MAX_FRAME_COUNT)                                       \
  {                                                                                                \
    g_CurrentProfilerFrameIndex = (g_CurrentProfilerFrameIndex + 1) % PROFILE_MAX_FRAME_COUNT;     \
  }

#define BEGIN_TIMED_BLOCK(ID)                                                                      \
  int FrameEventIndex##ID = g_CurrentTimerEventCount;                                              \
  GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][g_CurrentTimerEventCount]            \
    .StartCycleCount = _rdtsc();                                                                   \
  GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][g_CurrentTimerEventCount]            \
    .EventDepth = g_CurrentTimerEventDepth;                                                        \
  GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][g_CurrentTimerEventCount]            \
    .NameTableIndex = TIMER_NAME_##ID;                                                             \
  ++g_CurrentTimerEventCount;                                                                      \
  ++g_CurrentTimerEventDepth;                                                                      \
  if(g_CurrentProfilerFrameIndex == PROFILE_MAX_FRAME_COUNT)                                       \
  {                                                                                                \
    g_CurrentTimerEventCount--;                                                                    \
  }

#define END_TIMED_BLOCK(ID)                                                                        \
  uint64_t EndCycleCount##ID = _rdtsc();                                                           \
  GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][FrameEventIndex##ID].EndCycleCount = \
    EndCycleCount##ID;                                                                             \
  GLOBAL_TIMER_FRAME_SUMMARY_TABLE[g_CurrentProfilerFrameIndex][TIMER_NAME_##ID].CycleCount +=     \
    EndCycleCount##ID -                                                                            \
    GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][g_CurrentTimerEventCount]          \
      .StartCycleCount;                                                                            \
  GLOBAL_TIMER_FRAME_SUMMARY_TABLE[g_CurrentProfilerFrameIndex][TIMER_NAME_##ID].Calls++;          \
  --g_CurrentTimerEventDepth;

#define BEGIN_GPU_BLOCK(ID) glBeginQuery(GL_TIME_ELAPSED, PROFILER_)
#define PAUSE_PROFILE()                                                                            \
  g_SavedCurrentFrameIndex    = g_CurrentProfilerFrameIndex;                                       \
  g_CurrentProfilerFrameIndex = PROFILE_MAX_FRAME_COUNT;

#define RESUME_PROFILE() g_CurrentProfilerFrameIndex = g_SavedCurrentFrameIndex;

#else

#define BEGIN_FRAME()
#define END_FRAME()
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)
#define PAUSE_PROFILE()
#define RESUME_PROFILE()

#endif // DEGUB_PROFILING
