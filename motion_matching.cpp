#include "motion_matching.h"

void
FillFrameInfoBonePositionFields(mm_frame_info* OutInfo, const mat4* HierarchicalModelSpacePoses,
                                const mat4& InvRootMatrix, const mm_format_info& FormatInfo)
{
  for(int i = 0; i < FormatInfo.ComparisonBoneIndices.Count; i++)
  {
    OutInfo->BonePs[i] =
      Math::MulMat4(InvRootMatrix, HierarchicalModelSpacePoses[FormatInfo.ComparisonBoneIndices[i]])
        .T;
    OutInfo->BonePs[i].Y = 0;
  }
}

// TODO(Lukas): move this to a propper part of memory, likely to the resource manager
mm_frame_info g_MMStorageArray[MM_MAX_FRAME_INFO_COUNT];

void
PrecomputeRuntimeMMData(Memory::stack_allocator* TempAlloc, mm_animation_set* MMSet,
                        Resource::resource_manager* Resources, const Anim::skeleton* Skeleton)
{
  Memory::marker LoadStart    = TempAlloc->GetMarker();
  mat4*          TempMatrices = PushArray(TempAlloc, Skeleton->BoneCount, mat4);
  MMSet->FrameInfos           = &g_MMStorageArray[0];
  MMSet->FrameInfoCount       = 0;
  MMSet->AnimFrameRanges.Clear();

  for(int a = 0; a < MMSet->AnimRIDs.Count; a++)
  {
    const Anim::animation* Anim = Resources->GetAnimation(MMSet->AnimRIDs[a]);
    assert(Anim->ChannelCount == Skeleton->BoneCount);

    const float   PosSamplePeriod = MMSet->FormatInfo.TrajectoryTimeHorizon / float(MM_POINT_COUNT);
    const int32_t PosSamplingInterval =
      (int32_t)((PosSamplePeriod / Anim::GetAnimDuration(Anim)) * Anim->KeyframeCount);
    const int32_t LookaheadKeyframeCount = PosSamplingInterval * MM_POINT_COUNT;

    int32_range CurrentRange = {};
    CurrentRange.Start       = MMSet->FrameInfoCount;
    MMSet->FrameInfoCount += Anim->KeyframeCount - LookaheadKeyframeCount;
    CurrentRange.End = MMSet->FrameInfoCount;
    assert(MMSet->FrameInfoCount < MM_MAX_FRAME_INFO_COUNT);
    MMSet->AnimFrameRanges.Push(CurrentRange);

    for(int i = 0; i < Anim->KeyframeCount - PosSamplingInterval; i++)
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

      FillFrameInfoBonePositionFields(&MMSet->FrameInfos[FrameInfoIndex], TempMatrices, InvRootMatrix,
                                      MMSet->FormatInfo);
      {
        // TODO(Lukas) MAKE THIS USE A SAMPLING AND ADD A SAMPLING FREQUENCY TO MM_FORMAT_INFO SO
        // THAT ARBITRARILY SAMPLED ANIMATIONS COULD WORK
        for(int p = 0; p < MM_POINT_COUNT; p++)
        {
          vec3 SamplePoint =
            Anim->Transforms[(i + p * PosSamplingInterval) * Anim->ChannelCount + HipIndex]
              .Translation;
          vec4 SamplePointHomog = { SamplePoint.X, 0, SamplePoint.Z, 1 };
          MMSet->FrameInfos[FrameInfoIndex].TrajectoryPs[p] =
            Math::MulMat4Vec4(InvRootMatrix, SamplePointHomog).XYZ;
        }
      }
    }
    // Compute the velocities from the frame_info positions
    {
    }
  }
  MMSet->IsBuilt = true;
  TempAlloc->FreeToMarker(LoadStart);
}

mm_frame_info
GetCurrentFrameGoal(const Anim::animation_controller* Controller, vec3 LocalVelocity,
                    mm_format_info FormatInfo)
{
  mm_frame_info ResultInfo = {};

  mat4    RootMatrix;
  mat4    InvRootMatrix;
  int32_t HipIndex = 0;
  Anim::GetRootAndInvRootMatrices(&RootMatrix, &InvRootMatrix,
                                  Controller->HierarchicalModelSpaceMatrices[HipIndex]);

  // Extract current bone positions
  {
    for(int b = 0; b < FormatInfo.ComparisonBoneIndices.Count; b++)
    {
      ResultInfo.BonePs[b] =
        Math::MulMat4(InvRootMatrix,
                      Controller
                        ->HierarchicalModelSpaceMatrices[FormatInfo.ComparisonBoneIndices[b]])
          .T;
      ResultInfo.BonePs[b].Y = 0;
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
ComputeCost(const mm_frame_info& A, const mm_frame_info& B)
{
  float Cost = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 Temp = A.BonePs[b] - B.BonePs[b];
    Cost += Math::Dot(Temp, Temp);
  }
  for(int p = 0; p < MM_POINT_COUNT; p++)
  {
    vec3 Temp = A.TrajectoryPs[p] - B.TrajectoryPs[p];
    Cost += Math::Dot(Temp, Temp);
  }
  return Cost;
}

float
MotionMatch(int32_t* OutAnimIndex, int32_t* OutStartFrameIndex, const mm_animation_set* MMSet,
            mm_frame_info Goal)
{
  assert(OutAnimIndex && OutStartFrameIndex);
  assert(MMSet);
  assert(MMSet->FrameInfos);

  // TODO(Lukas) Change this number to float infinity
  float   SmallestCost       = 100000000;
  int32_t BestFrameInfoIndex = -1;
  for(int i = 0; i < MMSet->FrameInfoCount; i++)
  {
    float CurrentCost = ComputeCost(Goal, MMSet->FrameInfos[i]);
    if(CurrentCost < SmallestCost)
    {
      SmallestCost       = CurrentCost;
      BestFrameInfoIndex = i;
    }
  }

  assert(BestFrameInfoIndex != -1);
  *OutStartFrameIndex = BestFrameInfoIndex;

  for(int a = 0; a < MMSet->AnimFrameRanges.Count; a++)
  {
    int32_range CurrentRange = MMSet->AnimFrameRanges[a];
    if(CurrentRange.Start <= BestFrameInfoIndex && BestFrameInfoIndex < CurrentRange.End)
    {
      *OutAnimIndex = a;
    }
  }

  return SmallestCost;
}
