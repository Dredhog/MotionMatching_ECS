#include "motion_matching.h"

#define MM_MAX_FRAME_INFO_COUNT 10 * 60 * 120

// TODO(Lukas): move this to a propper part of memory, likely to the resource manager
mm_frame_info g_MMStorageArray[MM_MAX_FRAME_INFO_COUNT];

// TODO(Lukas) Should remove first frame on export instead of skipping it at runtime
const int32_t g_FirstUsedKeyframeIndex = 1;

mm_controller_data
PrecomputeRuntimeMMData(Memory::stack_allocator* TempAlloc, Resource::resource_manager* Resources,
                        mm_matching_params Params, const Anim::skeleton* Skeleton)
{
  Memory::marker LoadStart    = TempAlloc->GetMarker();
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
      Anim->KeyframeCount - LookaheadKeyframeCount - g_FirstUsedKeyframeIndex;

    FrameInfoStack.Expand(NewFrameInfoCount);
    int32_range CurrentRange = { FrameInfoStack.Count, FrameInfoStack.Count + NewFrameInfoCount };
    MMData.AnimFrameRanges.Push(CurrentRange);

    for(int i = g_FirstUsedKeyframeIndex; i < Anim->KeyframeCount - LookaheadKeyframeCount; i++)
    {
      int32_t FrameInfoIndex = CurrentRange.Start + i;
      // TODO(Lukas) MAKE THIS USE A SAMPLING AND ADD A SAMPLING FREQUENCY TO MM_FORMAT_INFO SO THAT
      // ARBITRARILY SAMPLED ANIMATIONS COULD WORK
      const Anim::transform* LocalPoseTransforms = &Anim->Transforms[i * Anim->ChannelCount];

      ComputeBoneSpacePoses(TempMatrices, LocalPoseTransforms, Anim->ChannelCount);
      ComputeModelSpacePoses(TempMatrices, TempMatrices, Skeleton);
      ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Skeleton);

      mat4    InvRootMatrix;
      int32_t HipIndex  = 0;
      mat4    HipMatrix = TempMatrices[HipIndex];
      Anim::GetRootAndInvRootMatrices(NULL, &InvRootMatrix, HipMatrix);

      {
        // Fill Bone Positions
        for(int b = 0; b < Params.FixedParams.ComparisonBoneIndices.Count; b++)
        {
          MMData.FrameInfos[FrameInfoIndex].BonePs[b] =
            Math::MulMat4(InvRootMatrix, TempMatrices[Params.FixedParams.ComparisonBoneIndices[b]])
              .T;
        }
      }
      // Fill Bone Trajectory Positions
      {
        // TODO(Lukas) MAKE THIS USE A SAMPLING AND ADD A SAMPLING FREQUENCY TO MM_FORMAT_INFO SO
        // THAT ARBITRARILY SAMPLED ANIMATIONS COULD WORK
        for(int p = 0; p < MM_POINT_COUNT; p++)
        {
          vec3 SamplePoint =
            Anim->Transforms[(i + (1 + p) * PosSamplingInterval) * Anim->ChannelCount + HipIndex]
              .Translation;
          vec4 SamplePointHomog = { SamplePoint, 1 };
          FrameInfoStack[FrameInfoIndex].TrajectoryPs[p] =
            Math::MulMat4Vec4(InvRootMatrix, SamplePointHomog).XYZ;
          FrameInfoStack[FrameInfoIndex].TrajectoryPs[p].Y = 0;
        }
      }
    }

    // TODO(Lukas) There is an off-by-one error here and the last frame info BoneVs is not computed
    // Compute the velocities from the frame_info position
    for(int i = g_FirstUsedKeyframeIndex; i < Anim->KeyframeCount - LookaheadKeyframeCount - 1; i++)
    {
      for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
      {
        FrameInfoStack[i].BoneVs[b] =
          (FrameInfoStack[i + 1].BonePs[b] - FrameInfoStack[i].BonePs[b]) / FrameDuration;
      }
    }
  }
  MMData.FrameInfos = FrameInfoStack.GetArrayHandle();

  TempAlloc->FreeToMarker(LoadStart);
  return MMData;
}

mm_frame_info
GetCurrentFrameGoal(const Anim::animation_controller* Controller, vec3 LocalVelocity,
                    mm_matching_params Params)
{
  mm_frame_info ResultInfo = {};

  mat4    RootMatrix;
  mat4    InvRootMatrix;
  int32_t HipIndex = 0;
  Anim::GetRootAndInvRootMatrices(&RootMatrix, &InvRootMatrix,
                                  Controller->HierarchicalModelSpaceMatrices[HipIndex]);

  // Extract current bone positions
  {
    for(int b = 0; b < Params.FixedParams.ComparisonBoneIndices.Count; b++)
    {
      ResultInfo.BonePs[b] =
        Math::MulMat4(InvRootMatrix,
                      Controller->HierarchicalModelSpaceMatrices[Params.FixedParams
                                                                   .ComparisonBoneIndices[b]])
          .T;
    }
  }
  // TODO(Lukas) add computation for velocities
  {

  } // Extract desired sampled trajectory
  {
    vec3 PointDelta = LocalVelocity / MM_POINT_COUNT;
    for(int p = 0; p < MM_POINT_COUNT; p++)
    {
      ResultInfo.TrajectoryPs[p] = PointDelta * (p + 1);
    }
  }

  return ResultInfo;
}

float
ComputeCost(const mm_frame_info& A, const mm_frame_info& B, float Responsiveness)
{
  float ShortTermCost = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 Diff = A.BonePs[b] - B.BonePs[b];
    ShortTermCost += Math::Dot(Diff, Diff);
  }
	float LongTermCost = 0.0f;
  for(int p = 0; p < MM_POINT_COUNT; p++)
  {
    vec3 Diff = A.TrajectoryPs[p] - B.TrajectoryPs[p];
    LongTermCost += Math::Dot(Diff, Diff);
  }

	float Cost = ShortTermCost + Responsiveness * LongTermCost;
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
      ComputeCost(Goal, MMData->FrameInfos[i], MMData->Params.DynamicParams.Responsiveness);
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
      *OutStartFrameIndex = (BestFrameInfoIndex - CurrentRange.Start) + g_FirstUsedKeyframeIndex;
    }
  }

  return SmallestCost;
}
