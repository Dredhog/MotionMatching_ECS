#include "motion_matching.h"
#include "misc.h"
#include "profile.h"

#include <cfloat>

#define MM_MAX_FRAME_INFO_COUNT 10 * 60 * 90

const int32_t g_SkipFrameCount = 1;
// Copy the velocity for the last frame

// TODO(Lukas) make this be used by the asset pipeline
mm_controller_data*
PrecomputeRuntimeMMData(Memory::stack_allocator*       TempAlloc,
                        array_handle<Anim::animation*> Animations, const mm_params& Params)
{
  TIMED_BLOCK(BuildMotionSet);

  mm_controller_data* MMData = PushAlignedStruct(TempAlloc, mm_controller_data);
  memset(MMData, 0, sizeof(mm_controller_data));
  MMData->Params = Params;

  mm_frame_info* FrameInfoStorage =
    PushAlignedArray(TempAlloc, MM_MAX_FRAME_INFO_COUNT, mm_frame_info);

  // Alloc temp memory for transforms and matrices
  mat4*      TempMatrices = PushArray(TempAlloc, Params.FixedParams.Skeleton.BoneCount, mat4);
  transform* TempTransforms =
    PushArray(TempAlloc, Params.FixedParams.Skeleton.BoneCount, transform);

  // Initialize the frame info stack
  stack_handle<mm_frame_info> FrameInfoStack = {};
  FrameInfoStack.Init(FrameInfoStorage, 0, sizeof(mm_frame_info) * MM_MAX_FRAME_INFO_COUNT);

  // Loop over all animations in the set
  for(int a = 0; a < Params.AnimRIDs.Count; a++)
  {
    const Anim::animation* Anim = Animations[a];
    assert(Anim->ChannelCount == Params.FixedParams.Skeleton.BoneCount);
    assert(1 < Anim->KeyframeCount);
    const float AnimDuration  = Anim::GetAnimDuration(Anim);
    const float FrameDuration = AnimDuration / float(Anim->KeyframeCount);

    const float PositionSamplingPeriod =
      Params.DynamicParams.TrajectoryTimeHorizon / float(MM_POINT_COUNT);

    const float AnimStartSkipTime = Anim->SampleTimes[0] + FrameDuration * float(g_SkipFrameCount);

    const int32_t NewFrameInfoCount = int32_t(
      MaxFloat(0.0f,
               ((AnimDuration - AnimStartSkipTime - Params.DynamicParams.TrajectoryTimeHorizon) *
                Params.FixedParams.MetadataSamplingFrequency)));

    mm_frame_info_range CurrentRange = {};
    {
      CurrentRange.StartTimeInAnim = AnimStartSkipTime;
      CurrentRange.Start           = FrameInfoStack.Count;
      CurrentRange.End             = FrameInfoStack.Count + NewFrameInfoCount;
    }
    FrameInfoStack.Expand(NewFrameInfoCount);
    MMData->AnimFrameInfoRanges.Push(CurrentRange);

    for(int i = 0; i < NewFrameInfoCount; i++)
    {
      int32_t FrameInfoIndex    = CurrentRange.Start + i;
      float   CurrentSampleTime = CurrentRange.StartTimeInAnim +
                                float(i) * (1.0f / Params.FixedParams.MetadataSamplingFrequency);
      Anim::LinearAnimationSample(TempTransforms, Anim, CurrentSampleTime);

      Anim::ComputeBoneSpacePoses(TempMatrices, TempTransforms, Anim->ChannelCount);
      ComputeModelSpacePoses(TempMatrices, TempMatrices, &Params.FixedParams.Skeleton);
      ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, &Params.FixedParams.Skeleton);

      mat4    InvRootMatrix;
      mat4    RootMatrix;
      int32_t HipIndex = 0;
      mat4    HipMatrix =
        Math::MulMat4(TempMatrices[HipIndex], Params.FixedParams.Skeleton.Bones[HipIndex].BindPose);
      Anim::GetRootAndInvRootMatrices(&RootMatrix, &InvRootMatrix, HipMatrix);

      // Fill Bone Positions
      for(int b = 0; b < Params.FixedParams.ComparisonBoneIndices.Count; b++)
      {
        FrameInfoStack[FrameInfoIndex].BonePs[b] =
          Math::MulMat4(InvRootMatrix,
                        Math::MulMat4(TempMatrices[Params.FixedParams.ComparisonBoneIndices[b]],
                                      Params.FixedParams.Skeleton
                                        .Bones[Params.FixedParams.ComparisonBoneIndices[b]]
                                        .BindPose))
            .T;
      }
      // Fill Bone Trajectory Positions

      for(int p = 0; p < MM_POINT_COUNT; p++)
      {
        transform SampleHipTransform =
          Anim::LinearAnimationBoneSample(Anim, HipIndex,
                                          CurrentSampleTime + (p + 1) * PositionSamplingPeriod);
        // NOTE(Lukas) this should use the root bone if animation has a dedicated one
        const Anim::bone* Bone = &Params.FixedParams.Skeleton.Bones[HipIndex];
        mat4              CurrentHipMatrix =
          Math::MulMat4(Bone->BindPose,
                        Math::MulMat4(TransformToMat4(SampleHipTransform), Bone->InverseBindPose));
        vec3 SamplePoint      = CurrentHipMatrix.T;
        vec4 SamplePointHomog = { SamplePoint, 1 };
        FrameInfoStack[FrameInfoIndex].TrajectoryPs[p] =
          Math::MulMat4Vec4(InvRootMatrix, SamplePointHomog).XYZ;

        FrameInfoStack[FrameInfoIndex].TrajectoryPs[p].Y = 0;

        vec3 CurrentZInTrajectorySpace =
          Math::MulMat4Vec4(InvRootMatrix, { CurrentHipMatrix.Z, 0 }).XYZ;

        FrameInfoStack[FrameInfoIndex].TrajectoryAngles[p] =
          atan2f(CurrentZInTrajectorySpace.X, CurrentZInTrajectorySpace.Z);
      }
    }

    // Compute velocity for first n-1 frames
    for(int i = 0; i < NewFrameInfoCount - 1; i++)
    {
      for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
      {
        FrameInfoStack[i].BoneVs[b] =
          (FrameInfoStack[i + 1].BonePs[b] - FrameInfoStack[i].BonePs[b]) *
          MMData->Params.FixedParams.MetadataSamplingFrequency;
      }

      for(int p = 0; p < MM_POINT_COUNT; p++)
      {
        FrameInfoStack[i].TrajectoryVs[p] =
          Math::Length(FrameInfoStack[i + 1].TrajectoryPs[p] - FrameInfoStack[i].TrajectoryPs[p]) /
          PositionSamplingPeriod;
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
      /*for(int p = 0; p < MM_POINT_COUNT; p++)
      {
        FrameInfoStack[LastFrameIndex].TrajectoryVs[p] =
          (0 < LastFrameIndex) ? FrameInfoStack[LastFrameIndex - 1].TrajectoryVs[p] : 0.0f;
      }*/
    }
  }
  MMData->FrameInfos            = FrameInfoStack.GetArrayHandle();
  Memory::marker AssetEndMarker = {};
  {
      AssetEndMarker.Address = (uint8_t*)(MMData->FrameInfos.Elements + MMData->FrameInfos.Count);
  }

  // Set up the mirroring info for goal generation
  MMData->Params.FixedParams.MirrorBoneIndices.HardClear();
  if(MMData->Params.DynamicParams.MatchMirroredAnimations)
  {
    for(int i = 0; i < MMData->Params.FixedParams.ComparisonBoneIndices.Count; i++)
    {
      int BoneA = MMData->Params.FixedParams.ComparisonBoneIndices[i];
      int BoneB = -1;
      for(int p = 0; p < MMData->Params.DynamicParams.MirrorInfo.PairCount; p++)
      {
        int CandidateA = MMData->Params.DynamicParams.MirrorInfo.BoneMirrorIndices[p].a;
        int CandidateB = MMData->Params.DynamicParams.MirrorInfo.BoneMirrorIndices[p].b;
        if(BoneA == CandidateA)
        {
          BoneB = CandidateB;
					break;
        }

        if(BoneA == CandidateB)
        {
          BoneB = CandidateA;
					break;
        }
      }
      // TODO(Lukas) Make sura that the bones selected are always in the mirror info stack
      assert(BoneB != -1 && "Search bone does not have a mirror");
      MMData->Params.FixedParams.MirrorBoneIndices.Push(BoneB);
    }
  }

  TempAlloc->FreeToMarker(AssetEndMarker);
  return MMData;
}

float
ComputeCost(const mm_frame_info& A, const mm_frame_info& B, float PosCoef, float VelCoef,
            float TrajCoef, float TrajVCoef, float TrajAngleCoef, const float* TrajectoryWeights)
{
  float PosDiffSum = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 Diff = A.BonePs[b] - B.BonePs[b];
    PosDiffSum += Math::Length(Diff);
  }

  float VelDiffSum = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 VelDiff = A.BoneVs[b] - B.BoneVs[b];
    VelDiffSum += Math::Length(VelDiff);
  }

  float TrajDiffSum  = 0.0f;
  //float TrajVDiffSum = 0.0f;
  for(int p = 0; p < MM_POINT_COUNT; p++)
  {
    vec3 Diff = A.TrajectoryPs[p] - B.TrajectoryPs[p];
    TrajDiffSum += TrajectoryWeights[p] * Math::Length(Diff);
    /*float VDiff = TrajectoryWeights[p] * fabs(A.TrajectoryVs[p] - B.TrajectoryVs[p]);
    TrajVDiffSum += VDiff;*/
  }

  float TrajDirDiffSum = 0.0f;
  for(int p = 0; p < MM_POINT_COUNT; p++)
  {
    vec2 DirA = { sinf(A.TrajectoryAngles[p]), cosf(A.TrajectoryAngles[p]) };
    vec2 DirB = { sinf(B.TrajectoryAngles[p]), cosf(B.TrajectoryAngles[p]) };
    vec2 Diff = DirA - DirB;
    TrajDirDiffSum += TrajectoryWeights[p] * Math::Length(Diff);
  }

  float Cost = PosCoef * PosDiffSum + VelCoef * VelDiffSum + TrajCoef * TrajDiffSum +
               /*TrajVCoef * TrajVDiffSum +*/ TrajAngleCoef * TrajDirDiffSum;

  return Cost;
}

float
ComputeCostComponents(float* BonePCost, float* BoneVCost, float* TrajPCost, float* TrajVCost,
                      float* TrajACost, const mm_frame_info& A, const mm_frame_info& B,
                      float PosCoef, float VelCoef, float TrajCoef, float TrajVCoef,
                      float TrajAngleCoef, const float* TrajectoryWeights)
{
  float PosDiffSum = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 Diff = A.BonePs[b] - B.BonePs[b];
    PosDiffSum += Math::Length(Diff);
  }

  float VelDiffSum = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 VelDiff = A.BoneVs[b] - B.BoneVs[b];
    VelDiffSum += Math::Length(VelDiff);
  }

  float TrajDiffSum  = 0.0f;
  //float TrajVDiffSum = 0.0f;
  for(int p = 0; p < MM_POINT_COUNT; p++)
  {
    vec3 Diff = A.TrajectoryPs[p] - B.TrajectoryPs[p];
    TrajDiffSum += TrajectoryWeights[p] * Math::Length(Diff);
    /*float VDiff = TrajectoryWeights[p] * fabs(A.TrajectoryVs[p] - B.TrajectoryVs[p]);
    TrajVDiffSum += VDiff;*/
  }

  float TrajDirDiffSum = 0.0f;
  for(int p = 0; p < MM_POINT_COUNT; p++)
  {
    vec2 DirA = { sinf(A.TrajectoryAngles[p]), cosf(A.TrajectoryAngles[p]) };
    vec2 DirB = { sinf(B.TrajectoryAngles[p]), cosf(B.TrajectoryAngles[p]) };
    vec2 Diff = DirA - DirB;
    TrajDirDiffSum += TrajectoryWeights[p] * Math::Length(Diff);
  }
  *BonePCost = PosDiffSum * PosCoef;
  *BoneVCost = VelDiffSum * VelCoef;
  *TrajPCost = TrajDiffSum * TrajCoef;
  *TrajVCost = 0;//TrajVDiffSum * TrajVCoef;
  *TrajACost = TrajDirDiffSum * TrajAngleCoef;

  float Cost = PosCoef * PosDiffSum + VelCoef * VelDiffSum + TrajCoef * TrajDiffSum +
               /*TrajVCoef * TrajVDiffSum +*/ TrajAngleCoef * TrajDirDiffSum;

  return Cost;
}

float
MotionMatch(int32_t* OutAnimIndex, float* OutLocalStartTime, mm_frame_info* OutBestMatch,
            const mm_controller_data* MMData, mm_frame_info Goal)
{
  TIMED_BLOCK(MotionMatch);
  assert(OutAnimIndex && OutLocalStartTime);
  assert(MMData);
  assert(MMData->FrameInfos.IsValid());

  float   SmallestCost       = FLT_MAX;
  int32_t BestFrameInfoIndex = -1;
  for(int i = 0; i < MMData->FrameInfos.Count; i++)
  {
    {
      float CurrentCost =
        ComputeCost(Goal, MMData->FrameInfos[i], MMData->Params.DynamicParams.BonePCoefficient,
                    MMData->Params.DynamicParams.BoneVCoefficient,
                    MMData->Params.DynamicParams.TrajPCoefficient,
                    MMData->Params.DynamicParams.TrajVCoefficient,
                    MMData->Params.DynamicParams.TrajAngleCoefficient,
                    MMData->Params.DynamicParams.TrajectoryWeights);
      if(CurrentCost < SmallestCost)
      {
        SmallestCost       = CurrentCost;
        BestFrameInfoIndex = i;
      }
    }
  }

  assert(BestFrameInfoIndex != -1);

  for(int a = 0; a < MMData->AnimFrameInfoRanges.Count; a++)
  {
    mm_frame_info_range CurrentRange = MMData->AnimFrameInfoRanges[a];
    if(CurrentRange.Start <= BestFrameInfoIndex && BestFrameInfoIndex < CurrentRange.End)
    {
      *OutAnimIndex = a;
      *OutLocalStartTime =
        CurrentRange.StartTimeInAnim + (BestFrameInfoIndex - CurrentRange.Start) /
                                         MMData->Params.FixedParams.MetadataSamplingFrequency;
      *OutBestMatch = MMData->FrameInfos[BestFrameInfoIndex];
    }
  }

  return SmallestCost;
}

float
MotionMatchWithMirrors(int32_t* OutAnimIndex, float* OutLocalStartTime, mm_frame_info* OutBestMatch,
                       bool* OutMatchedMirrored, const mm_controller_data* MMData,
                       mm_frame_info Goal, mm_frame_info MirroredGoal)
{
  TIMED_BLOCK(MotionMatch);
  assert(MMData->FrameInfos.IsValid());

  float   SmallestCost       = FLT_MAX;
  int32_t BestFrameInfoIndex = -1;
  bool    MatchIsMirrored    = false;
  for(int i = 0; i < MMData->FrameInfos.Count; i++)
  {
    {
      float CurrentCost =
        ComputeCost(Goal, MMData->FrameInfos[i], MMData->Params.DynamicParams.BonePCoefficient,
                    MMData->Params.DynamicParams.BoneVCoefficient,
                    MMData->Params.DynamicParams.TrajPCoefficient,
                    MMData->Params.DynamicParams.TrajVCoefficient,
                    MMData->Params.DynamicParams.TrajAngleCoefficient,
                    MMData->Params.DynamicParams.TrajectoryWeights);
      if(CurrentCost < SmallestCost)
      {
        SmallestCost       = CurrentCost;
        BestFrameInfoIndex = i;
        MatchIsMirrored    = false;
      }
    }

    {
      float MirroredCost = ComputeCost(MirroredGoal, MMData->FrameInfos[i],
                                       MMData->Params.DynamicParams.BonePCoefficient,
                                       MMData->Params.DynamicParams.BoneVCoefficient,
                                       MMData->Params.DynamicParams.TrajPCoefficient,
                                       MMData->Params.DynamicParams.TrajVCoefficient,
                                       MMData->Params.DynamicParams.TrajAngleCoefficient,
                                       MMData->Params.DynamicParams.TrajectoryWeights);

      if(MirroredCost < SmallestCost)
      {
        SmallestCost       = MirroredCost;
        BestFrameInfoIndex = i;
        MatchIsMirrored    = true;
      }
    }
  }

  assert(BestFrameInfoIndex != -1);

  for(int a = 0; a < MMData->AnimFrameInfoRanges.Count; a++)
  {
    mm_frame_info_range CurrentRange = MMData->AnimFrameInfoRanges[a];
    if(CurrentRange.Start <= BestFrameInfoIndex && BestFrameInfoIndex < CurrentRange.End)
    {
      *OutAnimIndex = a;
      *OutLocalStartTime =
        CurrentRange.StartTimeInAnim + (BestFrameInfoIndex - CurrentRange.Start) /
                                         MMData->Params.FixedParams.MetadataSamplingFrequency;
      *OutBestMatch = MMData->FrameInfos[BestFrameInfoIndex];
    }
  }
  *OutMatchedMirrored = MatchIsMirrored;

  return SmallestCost;
}
