#pragma once

#include "movement_spline.h"
#include "motion_matching.h"
#include "goal_gen.h"
#include "rid.h"

#include "resource_manager.h"
#include "blend_stack.h"
#include "common.h"
#include "misc.h"
#include <stdint.h>

#define MM_CONTROLLER_MAX_COUNT 20

struct mm_timeline_state
{
  blend_stack SavedBlendStack;
  transform   SavedTransform;
  float       SavedAnimPlayerTime;
  uintptr_t   SavedControllerHash;

  bool Paused;
  bool Scrubbing;
  bool BreakOnMatch;
};

struct mm_input_controller
{
  float MaxSpeed;
  float PositionBias;
  float DirectionBias;
  bool  UseStrafing;
  bool  UseSmoothGoal;
};

#define FOR_ALL_NAMES(DO_FUNC)                                                                     \
  DO_FUNC(rid, MMControllerRID, MMControllerRIDs)                                                  \
  DO_FUNC(blend_stack, BlendStack, BlendStacks)                                                    \
  DO_FUNC(int32_t, EntityIndex, EntityIndices)                                                     \
  DO_FUNC(bool, FollowSpline, FollowSpline)                                                        \
  DO_FUNC(spline_follow_state, SplineState, SplineStates)                                          \
  DO_FUNC(trajectory, Trajectory, Trajectories)                                                    \
  DO_FUNC(mm_input_controller, InputController, InputControllers)                                  \
  DO_FUNC(mm_controller_data*, MMController, MMControllers)                                        \
  DO_FUNC(Anim::skeleton*, Skeleton, Skeletons)                                                    \
  DO_FUNC(float, AnimPlayerTime, AnimPlayerTimes)                                                  \
  DO_FUNC(mm_frame_info, LastMatchedGoal, LastMatchedGoals)                                        \
  DO_FUNC(transform, LastMatchedTransform, LastMatchedTransforms)                                  \
  DO_FUNC(mm_frame_info, MirroredAnimGoal, MirroredAnimGoals)                                      \
  DO_FUNC(mm_frame_info, AnimGoal, AnimGoals)                                                      \
  DO_FUNC(transform, OutDeltaRootMotion, OutDeltaRootMotions)

#define PLACE_TOKEN(X) X
#define GENERATE_MM_DATA_ARRAY_FIELDS(Type, SingularName, PluralName) Type PluralName[MM_CONTROLLER_MAX_COUNT];
#define GENERATE_MM_DATA_POINTER_FIELDS(Type, SingularName, PluralName) Type * const SingularName;
#define GENERATE_MM_DATA_SRC_TO_DEST_ASSIGNMENT(Type, SingularName, PluralName) *Dest  PLACE_TOKEN(->)  SingularName = *Src PLACE_TOKEN(->) SingularName;
#define GENERATE_MM_DATA_TEMP_COPIES(Type, SingularName, PluralName) Type Temp##SingularName = *A PLACE_TOKEN(->) SingularName;
#define GENERATE_MM_DATA_ASSIGNMENT_B_TO_A(Type, SingularName, PluralName) *A  PLACE_TOKEN(->)  SingularName = *B PLACE_TOKEN(->) SingularName;
#define GENERATE_MM_DATA_ASSIGNMENT_TEMP_TO_B(Type, SingularName, PluralName) *B  PLACE_TOKEN(->)  SingularName = Temp##SingularName;
#define GENERATE_MM_DATA_INIT_FIELDS(Type, SingularName, PluralName) PLACE_TOKEN(&) MMEntityData->PluralName[MMDataIndex],

struct mm_entity_data
{
  int32_t Count;
  FOR_ALL_NAMES(GENERATE_MM_DATA_ARRAY_FIELDS);
};

// Used in the editor to access the soa fields
struct mm_aos_entity_data
{
  FOR_ALL_NAMES(GENERATE_MM_DATA_POINTER_FIELDS);
};

inline void
CopyAOSMMEntityData(mm_aos_entity_data* Dest, const mm_aos_entity_data* Src)
{
  FOR_ALL_NAMES(GENERATE_MM_DATA_SRC_TO_DEST_ASSIGNMENT);
}

inline void
SwapMMEntityData(mm_aos_entity_data* A, mm_aos_entity_data* B)
{
  FOR_ALL_NAMES(GENERATE_MM_DATA_TEMP_COPIES);
  FOR_ALL_NAMES(GENERATE_MM_DATA_ASSIGNMENT_B_TO_A);
  FOR_ALL_NAMES(GENERATE_MM_DATA_ASSIGNMENT_TEMP_TO_B);
}

inline mm_aos_entity_data
GetAOSMMDataAtIndex(int32_t MMDataIndex, mm_entity_data* MMEntityData)
{
  assert(0 <= MMDataIndex && MMDataIndex < MMEntityData->Count);
  mm_aos_entity_data Result = { FOR_ALL_NAMES(GENERATE_MM_DATA_INIT_FIELDS) };
  return Result;
}
#undef FOR_ALL_NAMES

void SetDefaultMMControllerFileds(mm_aos_entity_data* MMEntityData);

void RemoveMMControllerDataAtIndex(entity* Entities, int32_t MMControllerIndex,
                                   Resource::resource_manager* Resources,
                                   mm_entity_data*             MMEntityData);

int32_t GetEntityMMDataIndex(int32_t EntityIndex, const mm_entity_data* MMEntityData);

void ClearAnimationData(blend_stack* BlendStacks, int32_t* EntityIndices, int32_t Count,
                        entity* Entities, int32_t DebugEntityCount);

void SortMMEntityDataByUsage(int32_t* OutInputControlledCount,
                             int32_t* OutTrajectoryControlledStart,
                             int32_t* OutTrajectoryControlledCount, mm_entity_data* MMEntityData,
                             const spline_system* Splines);

void FetchMMControllerDataPointers(Resource::resource_manager* Resources,
                                   mm_controller_data** OutMMControllers, rid* MMControllerRIDs,
                                   int32_t Count);

void FetchSkeletonPointers(Anim::skeleton** OutSkeletons, const int32_t* EntityIndices,
                           const entity* Entities, int32_t Count);

void FetchAnimationPointers(Resource::resource_manager* Resources,
                            mm_controller_data** MMControllers, blend_stack* BlendStacks,
                            int32_t Count);

void PlayAnimsIfBlendStacksAreEmpty(blend_stack* BSs, float* GlobalTimes,
                                    const mm_controller_data* const* MMControllers, int32_t Count);

void DrawGoalFrameInfos(const mm_frame_info* GoalInfos, const blend_stack* BlendStacks,
                        const transform* LastMatchTransforms, int32_t Count,
                        const mm_info_debug_settings* MMInfoDebug, vec3 BoneColor,
                        vec3 TrajectoryColor, vec3 DirectionColor);

void DrawGoalFrameInfos(const mm_frame_info* GoalInfos, const int32_t* EntityIndices, int32_t Count,
                        const entity* Entities, const mm_info_debug_settings* MMInfoDebug,
                        vec3 BoneColor = { 1, 0, 1 }, vec3 TrajectoryColor = { 0, 0, 1 },
                        vec3 DirectionColor = { 1, 0, 0 });

void DrawControlTrajectories(const trajectory*          Trajectories,
                             const mm_input_controller* InputControllers,
                             const int32_t* EntityIndices, int32_t Count, const entity* Entities);

void GenerateGoalsFromInput(mm_frame_info* OutGoals, mm_frame_info* OutMirroredGoals,
                            trajectory* Trajectories, Memory::stack_allocator* TempAlloc,
                            const blend_stack* BlendStacks, const float* GlobalTimes,
                            const Anim::skeleton* const*     Skeletons,
                            const mm_controller_data* const* MMControllers,
                            const mm_input_controller*       InputControllers,
                            const int32_t* EntityIndices, int32_t Count, const entity* Entities,
                            const game_input* Input, const entity_goal_input* InputOverrides,
                            int32_t InputOverrideCount, vec3 CameraForward, bool AllowWASDControls);

void AssertSplineIndicesAndClampWaypointIndices(spline_follow_state* SplineStates, int32_t Count,
                                                const movement_spline* Splines,
                                                int32_t                DebugSplineCount);

void GenerateGoalsFromSplines(Memory::stack_allocator* TempAlloc, mm_frame_info* OutGoals,
                              mm_frame_info* OutMirroredGoals, trajectory* Trajectories,
                              spline_follow_state*             SplineStates,
                              const mm_input_controller*       InputControllers,
                              const mm_controller_data* const* MMControllers,
                              const blend_stack* BlendStacks, const float* AnimPlayerTimes,
                              const Anim::skeleton* const* Skeletons, const int32_t* EntityIndices,
                              int32_t Count, const movement_spline* Splines,
                              int32_t DebugSplineCount, const entity* Entities);

void MotionMatchGoals(blend_stack* OutBlendStacks, mm_frame_info* LastMatchedGoals,
                      transform* OutLastMatchedTransforms, const mm_frame_info* AnimGoals,
                      const mm_frame_info*             MirroredAnimGoals,
                      const mm_controller_data* const* MMControllers, const float* GlobalTimes,
                      const int32_t* EntityIndices, int32_t Count, entity* Entities);
void ComputeLocalRootMotion(transform*                   OutLocalDeltaRootMotions,
                            const Anim::skeleton* const* Skeletons, const blend_stack* BlendStacks,
                            const float* GlobalTimes, int32_t Count, float dt);

void ApplyRootMotion(entity* InOutEntities, trajectory* Trajectories,
                     const transform* LocalDeltaRootMotions, int32_t* EntityIndices, int32_t Count);

void AdvanceAnimPlayerTimes(float* InOutAnimPlayerTimes, int32_t Count, float dt);

void RemoveBlendedOutAnimsFromBlendStacks(blend_stack* InOutBlendStacks,
                                          const float* GlobalPlayTimes, int32_t Count);

void CopyMMAnimDataToAnimationPlayers(entity* OutEntities, const blend_stack* BlendStacks,
                                      const float* GlobalPlayTimes, const int32_t* EntityIndices,
                                      int32_t Count);
