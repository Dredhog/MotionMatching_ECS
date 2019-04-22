#pragma once

#include "stack_alloc.h"
#include "resource_manager.h"
#include "basic_data_structures.h"

#define MM_POINT_COUNT 3
#define MM_COMPARISON_BONE_COUNT 2
#define MM_ANIM_CAPACITY 35

struct mm_fixed_params
{
  fixed_stack<int32_t, MM_COMPARISON_BONE_COUNT> ComparisonBoneIndices;
  fixed_stack<int32_t, MM_COMPARISON_BONE_COUNT> MirroredComparisonIndexIndices;

  float MetadataSamplingFrequency;
};

struct mm_dynamic_params
{
  float TrajectoryTimeHorizon;
  float BonePCoefficient;
  float BoneVCoefficient;
  float TrajPCoefficient;
  float TrajVCoefficient;
  float TrajAngleCoefficient;
  float BelndInTime;
  float MinTimeOffsetThreshold;
  bool  MatchMirroredAnimations;
};

struct mm_matching_params
{
  fixed_stack<rid, MM_ANIM_CAPACITY> AnimRIDs;

  mm_fixed_params   FixedParams;
  mm_dynamic_params DynamicParams;
};

struct mm_frame_info
{
  vec3  BonePs[MM_COMPARISON_BONE_COUNT];
  vec3  BoneVs[MM_COMPARISON_BONE_COUNT];
  vec3  TrajectoryPs[MM_POINT_COUNT];
  float TrajectoryVs[MM_POINT_COUNT];
  float TrajectoryAngles[MM_POINT_COUNT];
};

struct mm_info_debug_settings
{
  bool ShowTrajectory;
  bool ShowTrajectoryAngles;
  bool ShowBonePositions;
  bool ShowBoneVelocities;
};

struct mm_debug_settings
{
  float TrajectoryDuration;
  int   TrajectorySampleCount;
  bool  ShowHipTrajectories;
  bool  ShowRootTrajectories;
  bool  PreviewInRootSpace;

  mm_info_debug_settings CurrentGoal;
  mm_info_debug_settings MatchedGoal;
};

struct mm_frame_info_range
{
  float   StartTimeInAnim;
  int32_t Start;
  int32_t End;
};

struct mm_controller_data
{
  mm_matching_params Params;

  fixed_stack<mm_frame_info_range, MM_ANIM_CAPACITY> AnimFrameInfoRanges;
  array_handle<mm_frame_info>                        FrameInfos;
};

mm_frame_info GetMMGoal(Memory::stack_allocator* TempAlloc, int32_t CurrentAnimIndex, bool Mirror,
                        const Anim::animation_controller* Controller, vec3 DesiredVelocity,
                        mm_matching_params Params);

void GetPoseGoal(mm_frame_info* OutPose, vec3* OutStartVelocity, Memory::stack_allocator* TempAlloc,
                 int32_t CurrentAnimIndex, bool Mirror,
                 const Anim::animation_controller* Controller, mm_matching_params Params);
void GetLongtermGoal(mm_frame_info* OutTrajectory, vec3 StartVelocity, vec3 EndVelocity);
void MirrorGoalJoints(mm_frame_info* InOutInfo, vec3 MirrorMatDiagonal,
                      const mm_fixed_params& Params);

mm_frame_info      GetMirroredFrameGoal(mm_frame_info OriginalInfo, vec3 MirrorMatDiagonal,
                                        const mm_fixed_params& Params);
mm_controller_data PrecomputeRuntimeMMData(Memory::stack_allocator*    TempAlloc,
                                           Resource::resource_manager* Resources,
                                           mm_matching_params          Params,
                                           const Anim::skeleton*       Skeleton);
float MotionMatch(int32_t* OutAnimIndex, float* OutLocalStartTime, mm_frame_info* OutBestMatch,
                  const mm_controller_data* MMData, mm_frame_info Goal);
float MotionMatchWithMirrors(int32_t* OutAnimIndex, float* OutLocalStartTime,
                             mm_frame_info* OutBestMatch, bool* OutMatchedMirrored,
                             const mm_controller_data* MMData, mm_frame_info Goal);
