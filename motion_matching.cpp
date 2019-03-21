#include "motion_matching.h"
#include "misc.h"

#define MM_MAX_FRAME_INFO_COUNT 10 * 60 * 120

// TODO(Lukas): move this to a propper part of memory, likely a seperate mm resource heap
mm_frame_info g_MMStorageArray[MM_MAX_FRAME_INFO_COUNT];

const int32_t g_FirstMatchedFrameIndex = 1;
//Copy the velocity for the last frame

mm_controller_data
PrecomputeRuntimeMMData(Memory::stack_allocator* TempAlloc, Resource::resource_manager* Resources,
                        mm_matching_params Params, const Anim::skeleton* Skeleton)
{
  Memory::marker FuncStartMemoryMarker = TempAlloc->GetMarker();
  mat4*          TempMatrices = PushArray(TempAlloc, Skeleton->BoneCount, mat4);

  mm_controller_data MMData = {}; //ZII
  MMData.Params             = Params;

  stack_handle<mm_frame_info> FrameInfoStack = {};
  FrameInfoStack.Init(g_MMStorageArray, 0, sizeof(g_MMStorageArray));

  for(int a = 0; a < Params.AnimRIDs.Count; a++)
  {
    const Anim::animation* Anim = Resources->GetAnimation(Params.AnimRIDs[a]);
    assert(Anim->ChannelCount == Skeleton->BoneCount);
    const float AnimDuration  = Anim::GetAnimDuration(Anim);
    const float FrameDuration = AnimDuration / (float)Anim->KeyframeCount;

    const float PosSamplePeriod =
      Params.DynamicParams.TrajectoryTimeHorizon / float(MM_POINT_COUNT);
    const int32_t PosSamplingInterval =
      (int32_t)((PosSamplePeriod / AnimDuration) * Anim->KeyframeCount);
    const int32_t LookaheadKeyframeCount = PosSamplingInterval * MM_POINT_COUNT;

    const int32_t NewFrameInfoCount =
      MaxInt32(0, (Anim->KeyframeCount - g_FirstMatchedFrameIndex) - LookaheadKeyframeCount);

    int32_range CurrentRange = { FrameInfoStack.Count, FrameInfoStack.Count + NewFrameInfoCount };
    FrameInfoStack.Expand(NewFrameInfoCount);
    MMData.AnimFrameRanges.Push(CurrentRange);

    for(int i = 0; i < NewFrameInfoCount; i++)
    {
      int32_t FrameInfoIndex = CurrentRange.Start + i;
      int32_t FrameIndex     = i + g_FirstMatchedFrameIndex;
      // TODO(Lukas) MAKE THIS USE A SAMPLING AND ADD A SAMPLING FREQUENCY TO MM_FORMAT_INFO SO THAT
      // ARBITRARILY SAMPLED ANIMATIONS COULD WORK
      const Anim::transform* LocalTransforms = &Anim->Transforms[FrameIndex * Anim->ChannelCount];

      ComputeBoneSpacePoses(TempMatrices, LocalTransforms, Anim->ChannelCount);
      ComputeModelSpacePoses(TempMatrices, TempMatrices, Skeleton);
      ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Skeleton);

      mat4    InvRootMatrix;
      int32_t HipIndex  = 0;
      mat4    HipMatrix = TempMatrices[HipIndex];
      Anim::GetRootAndInvRootMatrices(NULL, &InvRootMatrix, HipMatrix);

      // Fill Bone Positions
      for(int b = 0; b < Params.FixedParams.ComparisonBoneIndices.Count; b++)
      {
        FrameInfoStack[FrameInfoIndex].BonePs[b] =
          Math::MulMat4(InvRootMatrix, TempMatrices[Params.FixedParams.ComparisonBoneIndices[b]]).T;
      }
      // Fill Bone Trajectory Positions

      // TODO(Lukas) MAKE THIS USE SAMPING AT DESIRED TIME TIMES SO THAT ARBITRARILY SAMPLED
      // ANIMATIONS COULD WORK
      for(int p = 0; p < MM_POINT_COUNT; p++)
      {
        vec3 SamplePoint =
          Anim
            ->Transforms[(FrameIndex + (1 + p) * PosSamplingInterval) * Anim->ChannelCount +
                         HipIndex]
            .Translation;
        vec4 SamplePointHomog = { SamplePoint, 1 };
        FrameInfoStack[FrameInfoIndex].TrajectoryPs[p] =
          Math::MulMat4Vec4(InvRootMatrix, SamplePointHomog).XYZ;
        FrameInfoStack[FrameInfoIndex].TrajectoryPs[p].Y = 0;
      }
    }

    // Compute velocity for first n-1 frames
    for(int i = 0; i < NewFrameInfoCount - 1; i++)
    {
      for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
      {
        FrameInfoStack[i].BoneVs[b] =
          (FrameInfoStack[i + 1].BonePs[b] - FrameInfoStack[i].BonePs[b]) / FrameDuration;
      }
    }

		if(0 < NewFrameInfoCount)
		{
      int32_t LastFrameIndex = NewFrameInfoCount - 1;
      // Copy the velocity for the last frame
      // Last frames data
      for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
      {
        FrameInfoStack[LastFrameIndex].BoneVs[b] =
          (0 < LastFrameIndex) ? FrameInfoStack[LastFrameIndex - 1].BoneVs[b] : vec3{ 0, 0, 0 };
      }
    }
  }
  MMData.FrameInfos = FrameInfoStack.GetArrayHandle();

  TempAlloc->FreeToMarker(FuncStartMemoryMarker);
  return MMData;
}

mm_frame_info
GetCurrentFrameGoal(Memory::stack_allocator* TempAlloc, int32_t CurrentAnimIndex,
                    const Anim::animation_controller* Controller, vec3 StartVelocity,
                    vec3 EndVelocity, mm_matching_params Params)
{
  mm_frame_info  ResultInfo  = {};
  Memory::marker StackMarker = TempAlloc->GetMarker();

  const Anim::animation* CurrentAnim = Controller->Animations[CurrentAnimIndex];
  assert(Controller->Animations[CurrentAnimIndex]);

  // Allocate temporary transforms and matrices
  Anim::transform* TempTransforms =
    PushArray(TempAlloc, Controller->Skeleton->BoneCount, Anim::transform);
  mat4* TempMatrices = PushArray(TempAlloc, Controller->Skeleton->BoneCount, mat4);

  {
    // Sample the most recent animation's current frame
    {
      float LocalTime =
        GetLoopedSampleTime(Controller, CurrentAnimIndex, Controller->GlobalTimeSec);
      Anim::LinearAnimationSample(TempTransforms, CurrentAnim, LocalTime);
      ComputeBoneSpacePoses(TempMatrices, TempTransforms, Controller->Skeleton->BoneCount);
      ComputeModelSpacePoses(TempMatrices, TempMatrices, Controller->Skeleton);
      ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Controller->Skeleton);
    }
    mat4    RootMatrix;
    mat4    InvRootMatrix;
    int32_t HipIndex = 0;
    Anim::GetRootAndInvRootMatrices(&RootMatrix, &InvRootMatrix, TempMatrices[HipIndex]);

    // Store the current positions
    {
      for(int b = 0; b < Params.FixedParams.ComparisonBoneIndices.Count; b++)
      {
        ResultInfo.BonePs[b] =
          Math::MulMat4(InvRootMatrix, TempMatrices[Params.FixedParams.ComparisonBoneIndices[b]])
            .T;
      }
    }
  }

  // Compute bone velocities
  {
    const float Delta = 1 / 60.0f;

    // Sample the most recent animation's next frame
    {
      float LocalTimeWithDelta =
        GetLoopedSampleTime(Controller, CurrentAnimIndex, Controller->GlobalTimeSec + Delta);
      Anim::LinearAnimationSample(TempTransforms, CurrentAnim, LocalTimeWithDelta);
      ComputeBoneSpacePoses(TempMatrices, TempTransforms, Controller->Skeleton->BoneCount);
      ComputeModelSpacePoses(TempMatrices, TempMatrices, Controller->Skeleton);
      ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Controller->Skeleton);
    }

    // Compute bone linear velocities
    {
      mat4    RootMatrix;
      mat4    InvRootMatrix;
      int32_t HipIndex = 0;
      Anim::GetRootAndInvRootMatrices(&RootMatrix, &InvRootMatrix, TempMatrices[HipIndex]);
      for(int b = 0; b < Params.FixedParams.ComparisonBoneIndices.Count; b++)
      {
        ResultInfo.BoneVs[b] =
          (Math::MulMat4(InvRootMatrix, TempMatrices[Params.FixedParams.ComparisonBoneIndices[b]])
             .T -
           ResultInfo.BonePs[b]) /
          Delta;
      }
    }
  }

  // Extract desired sampled trajectory
  {
    const float TimeHorizon = 1.0f;
    const float Step        = 1 / 60.0f;
    float       PointDelta  = TimeHorizon / MM_POINT_COUNT;

    vec3  CurrentPoint    = {};
    vec3  CurrentVelocity = StartVelocity;
    float Elapsed         = 0.0f;
    for(int p = 0; p < MM_POINT_COUNT; p++)
    {
      float PointTimeHorizon = (p + 1) * PointDelta;
      for(; Elapsed <= PointTimeHorizon; Elapsed += Step)
      {
        CurrentPoint += CurrentVelocity * Step;

        float t = Elapsed / TimeHorizon;

        CurrentVelocity = (1.0f - t) * StartVelocity + t * EndVelocity;
      }

      ResultInfo.TrajectoryPs[p] = CurrentPoint;
    }
  }

  TempAlloc->FreeToMarker(StackMarker);
  return ResultInfo;
}

float
ComputeCost(const mm_frame_info& A, const mm_frame_info& B, float PosCoef, float VelCoef,
            float TrajCoef)
{
  float PosDiffSum = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 Diff = A.BonePs[b] - B.BonePs[b];
    PosDiffSum +=  Math::Dot(Diff, Diff);
  }

  float VelDiffSum = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 VelDiff = A.BoneVs[b] - B.BoneVs[b];
    VelDiffSum += Math::Dot(VelDiff, VelDiff);
  }

	float TrajDiffSum = 0.0f;
  for(int p = 0; p < MM_POINT_COUNT; p++)
  {
    vec3 Diff = A.TrajectoryPs[p] - B.TrajectoryPs[p];
    TrajDiffSum += Math::Dot(Diff, Diff);
  }

  float Cost = PosCoef * PosDiffSum + VelCoef * VelDiffSum + TrajDiffSum * TrajDiffSum;
  return Cost;
}

float
MotionMatch(int32_t* OutAnimIndex, int32_t* OutStartFrameIndex, const mm_controller_data* MMData,
            mm_frame_info Goal)
{
  assert(OutAnimIndex && OutStartFrameIndex);
  assert(MMData);
  assert(MMData->FrameInfos.IsValid());

  // TODO(Lukas) Change this number to float infinity
  float   SmallestCost       = 100000000;
  int32_t BestFrameInfoIndex = -1;
  for(int i = 0; i < MMData->FrameInfos.Count; i++)
  {
    float CurrentCost =
      ComputeCost(Goal, MMData->FrameInfos[i], MMData->Params.DynamicParams.PosCoefficient,
                  MMData->Params.DynamicParams.VelCoefficient,
                  MMData->Params.DynamicParams.TrajCoefficient);
    if(CurrentCost < SmallestCost)
    {
      SmallestCost       = CurrentCost;
      BestFrameInfoIndex = i;
    }
  }

  assert(BestFrameInfoIndex != -1);

  for(int a = 0; a < MMData->AnimFrameRanges.Count; a++)
  {
    int32_range CurrentRange = MMData->AnimFrameRanges[a];
    if(CurrentRange.Start <= BestFrameInfoIndex && BestFrameInfoIndex < CurrentRange.End)
    {
      *OutAnimIndex = a;
      *OutStartFrameIndex = BestFrameInfoIndex - CurrentRange.Start + g_FirstMatchedFrameIndex;
    }
  }

  return SmallestCost;
}
