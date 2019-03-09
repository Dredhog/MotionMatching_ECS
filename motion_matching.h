#pragma once

#include "stack_alloc.h"
#include "resource_manager.h"
#include "basic_data_structures.h"

#define MM_POINT_COUNT 3
#define MM_COMPARISON_BONE_COUNT 2
#define MM_MAX_ANIM_COUNT 20

struct mm_format_info
{
  fixed_stack<int32_t, MM_COMPARISON_BONE_COUNT> ComparisonBoneIndices;
  float                                          TrajectoryTimeHorizon;
  float                                          Responsiveness;
  float                                          BelndInTime;
};

struct mm_frame_info
{
  vec3 TrajectoryPs[MM_POINT_COUNT];
  vec3 BonePs[MM_COMPARISON_BONE_COUNT];
  vec3 BoneVs[MM_COMPARISON_BONE_COUNT];
  // vec2 Directions[MM_POINT_COUNT];
};

struct int32_range
{
  int32_t Start;
  int32_t End;
};

struct mm_animation_set
{
  // Set through GUI
  fixed_stack<rid, MM_MAX_ANIM_COUNT> AnimRIDs;
  mm_format_info                      FormatInfo;

  // Precomputed data
  bool                                        IsBuilt;
  fixed_stack<int32_range, MM_MAX_ANIM_COUNT> AnimFrameRanges;
  mm_frame_info*                              FrameInfos;
  int32_t                                     FrameInfoCount;
};

mm_frame_info GetCurrentFrameGoal(const Anim::animation_controller* Controller, vec3 Velocity,
                                  mm_format_info FormatInfo);
void  PrecomputeRuntimeMMData(Memory::stack_allocator* TempAlloc, mm_animation_set* MMSet,
                              Resource::resource_manager* Resources, const Anim::skeleton* Skeleton);
float MotionMatch(int32_t* OutAnimIndex, int32_t* OutStartFrameIndex, const mm_animation_set* MMSet,
                  mm_frame_info Goal);
