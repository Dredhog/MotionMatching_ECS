#pragma once

#include "trajectory.h"
#include "motion_matching.h"
#include "goal_gen.h"
#include "rid.h"

#include "resource_manager.h"
#include "blend_stack.h"
#include "common.h"
#include "misc.h"
#include <stdint.h>

#define MM_CONTROLLER_MAX_COUNT 20

enum spline_loop_type
{
  SPLINE_LoopToStart,
  SPLINE_ReverseWhenEnded,
};

struct spline_follow_state
{
  int32_t SplineIndex;

  int32_t  NextWaypointIndex;
  uint32_t LoopType;
  bool     MovingInPositive;
};

struct mm_input_controller
{
  float MaxSpeed;
  float PositionBias;
  float DirectionBias;
  bool  UseStrafing;
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
  DO_FUNC(Anim::animation_controller*, AnimController, AnimControllers)                            \
  DO_FUNC(mm_frame_info, LastMatchedGoal, LastMatchedGoals)                                        \
  DO_FUNC(mm_frame_info, MirroredAnimGoal, MirroredAnimGoals)                                      \
  DO_FUNC(mm_frame_info, AnimGoal, AnimGoals)                                                      \
  DO_FUNC(transform, OutDeltaRootMotion, OutDeltaRootMotions)

#define PLACE_TOKEN(X) X
#define GENERATE_MM_DATA_ARRAY_FIELDS(Type, SingularName, PluralName) Type PluralName[MM_CONTROLLER_MAX_COUNT];
#define GENERATE_MM_DATA_POINTER_FIELDS(Type, SingularName, PluralName) Type * const SingularName;
#define GENERATE_MM_DATA_SRC_TO_DEST_ASSIGNMENT(Type, SingularName, PluralName) *Dest  PLACE_TOKEN(->)  SingularName = *Src PLACE_TOKEN(->) SingularName;
#define GENERATE_MM_DATA_TEMP_COPIES(Type, SingularName, PluralName) Type Temp##SingularName = *A PLACE_TOKEN(->) SingularName;
#define GENERATE_MM_DATA_ASSIGNMENT_A_TO_B(Type, SingularName, PluralName) *B  PLACE_TOKEN(->)  SingularName = *A PLACE_TOKEN(->) SingularName;
#define GENERATE_MM_DATA_ASSIGNMENT_B_TO_A(Type, SingularName, PluralName) *A  PLACE_TOKEN(->)  SingularName = *B PLACE_TOKEN(->) SingularName;
#define GENERATE_MM_DATA_INIT_FIELDS(Type, SingularName, PluralName) PLACE_TOKEN(.) SingularName = PLACE_TOKEN(&) MMEntityData->PluralName[MMDataIndex],

struct mm_entity_data
{
  // Serializable data
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
  FOR_ALL_NAMES(GENERATE_MM_DATA_ASSIGNMENT_A_TO_B);
}

inline mm_aos_entity_data
GetEntityAOSMMData(int32_t MMDataIndex, mm_entity_data* MMEntityData)
{
  assert(0 <= MMDataIndex && MMDataIndex < MMEntityData->Count);
  mm_aos_entity_data Result = { FOR_ALL_NAMES(GENERATE_MM_DATA_INIT_FIELDS) };
  return Result;
}
#undef FOR_ALL_NAMES

inline void
SetDefaultMMControllerFileds(mm_aos_entity_data* MMEntityData)
{
  *MMEntityData->MMControllerRID = {};
  *MMEntityData->BlendStack      = {};
  *MMEntityData->EntityIndex     = -1;
  *MMEntityData->FollowSpline    = false;
  *MMEntityData->SplineState     = { .SplineIndex       = -1,
                                 .NextWaypointIndex = 0,
                                 .LoopType          = 0,
                                 .MovingInPositive  = true };
  InitTrajectory(MMEntityData->Trajectory);
  *MMEntityData->InputController = { .MaxSpeed      = 1.0f,
                                     .PositionBias  = 0.08f,
                                     .DirectionBias = 0.1f,
                                     .UseStrafing   = false };

  *MMEntityData->MMController     = NULL;
  *MMEntityData->AnimController   = NULL;
  *MMEntityData->LastMatchedGoal  = {};
  *MMEntityData->MirroredAnimGoal = {};
  *MMEntityData->AnimGoal         = {};
}

inline void
CopyMMEntityData(int32_t DestIndex, int32_t SourceIndex, mm_entity_data* MMEntityData)
{
  mm_aos_entity_data Dest   = GetEntityAOSMMData(DestIndex, MMEntityData);
  mm_aos_entity_data Source = GetEntityAOSMMData(SourceIndex, MMEntityData);
  CopyAOSMMEntityData(&Dest, &Source);
}

inline void
RemoveMMControllerDataAtIndex(int32_t MMControllerIndex, Resource::resource_manager* Resources,
                              mm_entity_data* MMEntityData)
{
  assert(0 <= MMControllerIndex && MMControllerIndex < MMEntityData->Count);

  mm_aos_entity_data RemovedController = GetEntityAOSMMData(MMControllerIndex, MMEntityData);
  if(RemovedController.MMControllerRID->Value > 0)
  {
    Resources->MMControllers.RemoveReference(*RemovedController.MMControllerRID);
  }

  if(MMEntityData->AnimControllers[MMControllerIndex])
  {
    for(int i = 0; i < ANIM_CONTROLLER_MAX_ANIM_COUNT; i++)
    {
      MMEntityData->AnimControllers[MMControllerIndex]->AnimationIDs[i] = {};
      MMEntityData->AnimControllers[MMControllerIndex]->Animations[i]   = {};
    }
    MMEntityData->AnimControllers[MMControllerIndex]->AnimStateCount = 0;
  }

  mm_aos_entity_data LastController = GetEntityAOSMMData(MMEntityData->Count - 1, MMEntityData);
  CopyAOSMMEntityData(&RemovedController, &LastController);
  MMEntityData->Count--;
}

inline int32_t
GetEntityMMDataIndex(int32_t EntityIndex, const mm_entity_data* MMEntityData)
{
  int32_t MMDataIndex = -1;
  for(int i = 0; i < MMEntityData->Count; i++)
  {
    if(MMEntityData->EntityIndices[i] == EntityIndex)
    {
      MMDataIndex = i;
      break;
    }
  }
  return MMDataIndex;
}

transform GetAnimRootMotionDelta(Anim::animation*                  RootMotionAnim,
                                 const Anim::animation_controller* C, bool MirrorRootMotionInX,
                                 float LocalSampleTime, float dt);

void SortMMEntityDataByUsage(int32_t* OutInputControlledCount,
                             int32_t* OutTrajectoryControlledStart,
                             int32_t* OutTrajectoryControlledCount, mm_entity_data* MMEntityData);

void FetchMMControllerDataPointers(Resource::resource_manager* Resources,
                                   mm_controller_data** OutMMControllers, rid* MMControllerRIDs,
                                   int32_t Count);

void FetchAnimControllerPointers(Anim::animation_controller** OutAnimControllers,
                                 const int32_t* EntityIndices, const entity* Entities,
                                 int32_t Count);

void FetchAnimationPointers(Resource::resource_manager* Resources,
                            mm_controller_data** MMControllers, int32_t Count);

void PlayAnimsIfBlendStacksAreEmpty(blend_stack* BSs, Anim::animation_controller** ACs,
                                    const mm_controller_data* const* MMControllers, int32_t Count);

void DrawGoalFrameInfos(const mm_frame_info* const GoalInfos, const blend_stack* const BlendStacks,
                        int32_t Count, const mm_info_debug_settings* MMInfoDebug, vec3 BoneColor,
                        vec3 TrajectoryColor, vec3 DirectionColor);

void DrawGoalFrameInfos(const mm_frame_info* GoalInfos, const int32_t* EntityIndices, int32_t Count,
                        const entity* Entities, const mm_info_debug_settings* MMInfoDebug,
                        vec3 BoneColor = { 1, 0, 1 }, vec3 TrajectoryColor = { 0, 0, 1 },
                        vec3 DirectionColor = { 1, 0, 0 });

void DrawControlTrajectories(const trajectory* Trajectories, const int32_t* EntityIndices,
                             int32_t Count, const entity* Entities);

void GenerateGoalsFromInput(mm_frame_info* OutGoals, mm_frame_info* OutMirroredGoals,
                            trajectory* Trajectories, Memory::stack_allocator* TempAlloc,
                            const blend_stack*                       BlendStacks,
                            const Anim::animation_controller* const* AnimControllers,
                            const mm_controller_data* const*         MMControllers,
                            const mm_input_controller*               InputController,
                            const int32_t* EntityIndices, int32_t Count, const entity* Entities,
                            const game_input* Input, vec3 CameraForward);

void GenerateGoalsFromSplines(mm_frame_info* OutGoals, const spline_follow_state* SplineStates,
                              const Anim::animation_controller* const* ACs,
                              const blend_stack* BlendStacks, int32_t Count,
                              const movement_spline* Splines);

void MotionMatchGoals(blend_stack*                       OutBlendStacks,
                      Anim::animation_controller* const* AnimControllers,
                      mm_frame_info* LastMatchedGoals, const mm_frame_info* AnimGoals,
                      const mm_frame_info*             MirroredAnimGoals,
                      const mm_controller_data* const* MMControllers, const int32_t* EntityIndices,
                      int32_t Count, entity* Entities);

void ComputeLocalRootMotion(transform*                               OutLocalDeltaRootMotions,
                            const Anim::animation_controller* const* AnimControllers,
                            const blend_stack* BlendStacks, int32_t Count, float dt);

void ApplyRootMotion(entity* InOutEntities, trajectory* Trajectories,
                     const transform* LocalDeltaRootMotions, int32_t* EntityIndices, int32_t Count);
