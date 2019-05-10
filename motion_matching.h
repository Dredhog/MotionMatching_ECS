#pragma once

#include "stack_alloc.h"
#include "basic_data_structures.h"
#include "linear_math/vector.h"
#include "rid.h"
#include "anim.h"
#include "file_queries.h"

#define MM_POINT_COUNT 3
#define MM_COMPARISON_BONE_COUNT 2
#define MM_ANIM_CAPACITY 35

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
  bool  ApplyRootMotion;

  mm_info_debug_settings CurrentGoal;
  mm_info_debug_settings MatchedGoal;
};

struct mm_fixed_params
{
  fixed_stack<int32_t, MM_COMPARISON_BONE_COUNT> ComparisonBoneIndices;
  fixed_stack<int32_t, MM_COMPARISON_BONE_COUNT> MirrorBoneIndices;

  Anim::skeleton             Skeleton;
  float                      MetadataSamplingFrequency;
};

struct mm_dynamic_params
{
  float                      TrajectoryTimeHorizon;
  float                      BonePCoefficient;
  float                      BoneVCoefficient;
  float                      TrajPCoefficient;
  float                      TrajVCoefficient;
  float                      TrajAngleCoefficient;
  float                      BlendInTime;
  float                      MinTimeOffsetThreshold;
  bool                       MatchMirroredAnimations;
  Anim::skeleton_mirror_info MirrorInfo;
};

struct mm_params
{
  fixed_stack<rid, MM_ANIM_CAPACITY>  AnimRIDs;
  fixed_stack<path, MM_ANIM_CAPACITY> AnimPaths;

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

struct mm_frame_info_range
{
  float   StartTimeInAnim;
  int32_t Start;
  int32_t End;
};

struct mm_controller_data
{
  mm_params Params;

  fixed_stack<Anim::animation*, MM_ANIM_CAPACITY>    Animations;
  fixed_stack<mm_frame_info_range, MM_ANIM_CAPACITY> AnimFrameInfoRanges;
  array_handle<mm_frame_info>                        FrameInfos;
};

enum anim_endpoint_extrapolation_type
{
  EXTRAPOLATE_None,
  EXTRAPOLATE_Loop,
  EXTRAPOLATE_Stop,
  EXTRAPOLATE_Continue,
};

inline void
ResetMMParamsToDefault(mm_params* Params)
{
  memset(Params, 0, sizeof(mm_params));
  Params->DynamicParams.BonePCoefficient        = 1.0f;
  Params->DynamicParams.BoneVCoefficient        = 0.02f;
  Params->DynamicParams.TrajPCoefficient        = 0.06f;
  Params->DynamicParams.TrajVCoefficient        = 0.0f;
  Params->DynamicParams.TrajAngleCoefficient    = 0.0f;
  Params->DynamicParams.TrajectoryTimeHorizon   = 1.0f;
  Params->DynamicParams.MinTimeOffsetThreshold  = 0.2f;
  Params->DynamicParams.BlendInTime             = 0.2f;
  Params->DynamicParams.MatchMirroredAnimations = false;
  Params->FixedParams.MetadataSamplingFrequency = 30.0f;
}

// Main metadata precomputation
mm_controller_data* PrecomputeRuntimeMMData(Memory::stack_allocator*       TempAlloc,
                                            array_handle<Anim::animation*> Animations,
                                            const mm_params&               Params);
// Cost function used for search
float ComputeCost(const mm_frame_info& A, const mm_frame_info& B, float PosCoef, float VelCoef,
                  float TrajCoef, float TrajVCoef, float TrajAngleCoef);

float ComputeCostComponents(float* BonePComp, float* BoneVComp, float* TrajPComp, float* TrajVComp,
                            float* TrajAComp, const mm_frame_info& A, const mm_frame_info& B,
                            float PosCoef, float VelCoef, float TrajCoef, float TrajVCoef,
                            float TrajAngleCoef);

// Runtime API
float MotionMatch(int32_t* OutAnimIndex, float* OutLocalStartTime, mm_frame_info* OutBestMatch,
                  const mm_controller_data* MMData, mm_frame_info Goal);
float MotionMatchWithMirrors(int32_t* OutAnimIndex, float* OutLocalStartTime,
                             mm_frame_info* OutBestMatch, bool* OutMatchedMirrored,
                             const mm_controller_data* MMData, mm_frame_info Goal,
                             mm_frame_info MirroredGoal);
