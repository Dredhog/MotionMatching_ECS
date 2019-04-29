#include "motion_matching.h"
#include "misc.h"
#include "profile.h"

#include <cfloat>

#define MM_MAX_FRAME_INFO_COUNT 10 * 60 * 90

const int32_t g_SkipFrameCount = 1;
// Copy the velocity for the last frame

enum anim_endpoint_extrapolation_type
{
  EXTRAPOLATE_None,
  EXTRAPOLATE_Loop,
  EXTRAPOLATE_Stop,
  EXTRAPOLATE_Continue,
};

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

    const int32_t NewFrameInfoCount =
      int32_t(MaxFloat(0.0f, ((AnimDuration - Params.DynamicParams.TrajectoryTimeHorizon) *
                              Params.FixedParams.MetadataSamplingFrequency)));

    const float AnimStartSkipTime = Anim->SampleTimes[0] + FrameDuration * float(g_SkipFrameCount);

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
        // TODO(Lukas) this should use the root bone if animation has a dedicated one
        mat4 CurrentHipMatrix = TransformToMat4(SampleHipTransform);
        vec3 SamplePoint      = SampleHipTransform.T;
        vec4 SamplePointHomog = { SamplePoint, 1 };
        FrameInfoStack[FrameInfoIndex].TrajectoryPs[p] =
          Math::MulMat4Vec4(InvRootMatrix, SamplePointHomog).XYZ;

        FrameInfoStack[FrameInfoIndex].TrajectoryPs[p].Y = 0;

        vec3  InitialXAxis = Math::Normalized({ RootMatrix.X.X, 0, RootMatrix.X.Z });
        vec3  PointXAxis   = Math::Normalized({ CurrentHipMatrix.X.X, 0, CurrentHipMatrix.X.Z });
        float CrossY       = Math::Cross(InitialXAxis, PointXAxis).Y;
        float AbsAngle     = acosf(ClampFloat(-1.0f, Math::Dot(InitialXAxis, PointXAxis), 1.0f));
        FrameInfoStack[FrameInfoIndex].TrajectoryAngles[p] = (0 <= CrossY) ? AbsAngle : -AbsAngle;
        assert(!isnan(FrameInfoStack[FrameInfoIndex].TrajectoryAngles[p]));
        assert(AbsFloat(FrameInfoStack[FrameInfoIndex].TrajectoryAngles[p]) <= 2.0f * M_PI);
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
  Memory::marker AssetEndMarker = { .Address = (uint8_t*)(MMData->FrameInfos.Elements +
                                                          MMData->FrameInfos.Count) };

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
      assert(BoneB != -1 && "Search bone does not have a mirror");
      MMData->Params.FixedParams.MirrorBoneIndices.Push(BoneB);
    }
  }

  TempAlloc->FreeToMarker(AssetEndMarker);
  return MMData;
}

void
CopyLongtermGoalFromRightToLeft(mm_frame_info* Dest, mm_frame_info Src)
{
  for(int i = 0; i < MM_POINT_COUNT; i++)
  {
    Dest->TrajectoryPs[i]     = Src.TrajectoryPs[i];
    Dest->TrajectoryVs[i]     = Src.TrajectoryVs[i];
    Dest->TrajectoryAngles[i] = Src.TrajectoryAngles[i];
  }
}

void
MirrorLongtermGoal(mm_frame_info* InOutInfo, vec3 MirrorMatDiagonal = { -1, 1, 1 })
{
  assert(AbsFloat(MirrorMatDiagonal.X) + AbsFloat(MirrorMatDiagonal.Y) +
           AbsFloat(MirrorMatDiagonal.Z) ==
         3.0f);

  mat3 MirrorMatrix = Math::Mat3Scale(MirrorMatDiagonal);
  for(int i = 0; i < MM_POINT_COUNT; i++)
  {
    InOutInfo->TrajectoryPs[i] = Math::MulMat3Vec3(MirrorMatrix, InOutInfo->TrajectoryPs[i]);
    InOutInfo->TrajectoryAngles[i] *= -1;
  }
}

void
GetMMGoal(mm_frame_info* OutGoal, mm_frame_info* OutMirroredGoal,
          Memory::stack_allocator* TempAlloc, int32_t AnimStateIndex, bool PlayingMirrored,
          const Anim::animation_controller* Controller, vec3 DesiredVelocity,
          const mm_fixed_params& Params)
{
  mm_frame_info AnimPose         = {};
  mm_frame_info MirroredAnimPose = {};

  vec3 AnimVelocity         = {};
  vec3 MirroredAnimVelocity = {};

  GetPoseGoal(&AnimPose, &MirroredAnimPose, &AnimVelocity, &MirroredAnimVelocity, TempAlloc,
              AnimStateIndex, Controller, Params);

  if(PlayingMirrored)
  {
    *OutGoal = MirroredAnimPose;
    GetLongtermGoal(OutGoal, MirroredAnimVelocity, DesiredVelocity);

    *OutMirroredGoal = AnimPose;
  }
  else
  {
    *OutGoal = AnimPose;
    GetLongtermGoal(OutGoal, AnimVelocity, DesiredVelocity);

    *OutMirroredGoal = MirroredAnimPose;
  }
  CopyLongtermGoalFromRightToLeft(OutMirroredGoal, *OutGoal);
  MirrorLongtermGoal(OutMirroredGoal);
}

void
GetPoseGoal(mm_frame_info* OutPose, mm_frame_info* OutMirrorPose, vec3* OutRootVelocity,
            vec3* OutMirrorRootVelocity, Memory::stack_allocator* TempAlloc, int32_t AnimStateIndex,
            const Anim::animation_controller* Controller, const mm_fixed_params& Params)
{
  Memory::marker StackMarker = TempAlloc->GetMarker();

  const Anim::animation* CurrentAnim = Controller->Animations[AnimStateIndex];
  assert(CurrentAnim);
	const bool GenerateMirrorInfo = Params.MirrorBoneIndices.Count == Params.ComparisonBoneIndices.Count;

  // Allocate temporary transforms and matrices
  transform* TempTransforms = PushArray(TempAlloc, Controller->Skeleton->BoneCount, transform);
  mat4*      TempMatrices   = PushArray(TempAlloc, Controller->Skeleton->BoneCount, mat4);

  mat4    CurrentRootMatrix;
  mat4    InvCurrentRootMatrix;
  mat4    NextRootMatrix;
  mat4    InvNextRootMatrix;
  int32_t HipIndex = 0;
  {
    // Sample the most recent animation's current frame
    {
      float LocalTime = GetLocalSampleTime(Controller, AnimStateIndex, Controller->GlobalTimeSec);
      Anim::LinearAnimationSample(TempTransforms, CurrentAnim, LocalTime);
      Anim::ComputeBoneSpacePoses(TempMatrices, TempTransforms, Controller->Skeleton->BoneCount);
      Anim::ComputeModelSpacePoses(TempMatrices, TempMatrices, Controller->Skeleton);
      Anim::ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Controller->Skeleton);
    }
    Anim::GetRootAndInvRootMatrices(&CurrentRootMatrix, &InvCurrentRootMatrix,
                                    Math::MulMat4(TempMatrices[HipIndex],
                                                  Controller->Skeleton->Bones[HipIndex].BindPose));

    // Store the current positions
    for(int b = 0; b < Params.ComparisonBoneIndices.Count; b++)
    {
      // Positions of bones used for matching
      if(OutPose)
      {
        int BoneIndex = Params.ComparisonBoneIndices[b];
        OutPose->BonePs[b] =
          Math::MulMat4(InvCurrentRootMatrix,
                        Math::MulMat4(TempMatrices[BoneIndex],
                                      Controller->Skeleton->Bones[BoneIndex].BindPose))
            .T;
      }

      // Positions of the mirroring bones used for matching
      if(OutMirrorPose && GenerateMirrorInfo)
      {
        int BoneIndex = Params.MirrorBoneIndices[b];
        OutMirrorPose->BonePs[b] =
          Math::MulMat4(InvCurrentRootMatrix,
                        Math::MulMat4(TempMatrices[BoneIndex],
                                      Controller->Skeleton->Bones[BoneIndex].BindPose))
            .T;
      }
    }
  }

  // Compute bone velocities
  vec3 RootVelocity = {};
  {
    const float Delta = 1 / 60.0f;

    // Sample the most recent animation's next frame
    {
      float LocalTimeWithDelta =
        GetLocalSampleTime(Controller, AnimStateIndex, Controller->GlobalTimeSec + Delta);
      Anim::LinearAnimationSample(TempTransforms, CurrentAnim, LocalTimeWithDelta);
      Anim::ComputeBoneSpacePoses(TempMatrices, TempTransforms, Controller->Skeleton->BoneCount);
      ComputeModelSpacePoses(TempMatrices, TempMatrices, Controller->Skeleton);
      ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Controller->Skeleton);
    }

    // Compute bone linear velocities
    {
      Anim::GetRootAndInvRootMatrices(&NextRootMatrix, &InvNextRootMatrix,
                                      Math::MulMat4(TempMatrices[HipIndex],
                                                    Controller->Skeleton->Bones[HipIndex]
                                                      .BindPose));
      for(int b = 0; b < Params.ComparisonBoneIndices.Count; b++)
      {
        if(OutPose)
        {
          int BoneIndex = Params.ComparisonBoneIndices[b];
          OutPose->BoneVs[b] =
            (Math::MulMat4(InvNextRootMatrix,
                           Math::MulMat4(TempMatrices[BoneIndex],
                                         Controller->Skeleton->Bones[BoneIndex].BindPose))
               .T -
             OutPose->BonePs[b]) /
            Delta;
        }
        if(OutMirrorPose && GenerateMirrorInfo)
        {
          int BoneIndex = Params.MirrorBoneIndices[b];
          OutMirrorPose->BoneVs[b] =
            (Math::MulMat4(InvNextRootMatrix,
                           Math::MulMat4(TempMatrices[BoneIndex],
                                         Controller->Skeleton->Bones[BoneIndex].BindPose))
               .T -
             OutMirrorPose->BonePs[b]) /
            Delta;
        }
      }
    }
    RootVelocity = Math::MulMat4(InvCurrentRootMatrix, NextRootMatrix).T / Delta;
  }

  // Flipping the mirror bone positions and velocities
  vec3 MirrorMatrixDiagonal = { -1, 1, 1 };
  if(OutMirrorPose && GenerateMirrorInfo)
  {
    mat3 MirrorMatrix = Math::Mat3Scale(MirrorMatrixDiagonal);
    for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
    {
      OutMirrorPose->BonePs[b] = Math::MulMat3Vec3(MirrorMatrix, OutMirrorPose->BonePs[b]);
      OutMirrorPose->BoneVs[b] = Math::MulMat3Vec3(MirrorMatrix, OutMirrorPose->BoneVs[b]);
    }
  }

  if(OutMirrorRootVelocity)
  {
    *OutRootVelocity = RootVelocity;
  }
  if(OutMirrorRootVelocity && GenerateMirrorInfo)
  {
    *OutMirrorRootVelocity = RootVelocity;
    OutMirrorRootVelocity->X *= MirrorMatrixDiagonal.X;
  }

	if(!GenerateMirrorInfo)
	{
    *OutMirrorPose = *OutPose;
    *OutMirrorRootVelocity = *OutRootVelocity;
  }

  TempAlloc->FreeToMarker(StackMarker);
}

void
GetLongtermGoal(mm_frame_info* OutTrajectory, vec3 StartVelocity, vec3 DesiredVelocity)
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

      CurrentVelocity = (1.0f - t) * StartVelocity + t * DesiredVelocity;
    }

    OutTrajectory->TrajectoryPs[p] = CurrentPoint;
    OutTrajectory->TrajectoryVs[p] = Math::Length(CurrentVelocity);
    {
      vec3  InitialXAxis = Math::Normalized(StartVelocity);
      vec3  PointXAxis   = Math::Normalized(CurrentVelocity);
      float CrossY       = Math::Cross(InitialXAxis, PointXAxis).Y;
      float AbsAngle     = acosf(ClampFloat(-1.0f, Math::Dot(InitialXAxis, PointXAxis), 1.0f));
      OutTrajectory->TrajectoryAngles[p] = (0 <= CrossY) ? AbsAngle : -AbsAngle;
    }
  }
}

float
ComputeCost(const mm_frame_info& A, const mm_frame_info& B, float PosCoef, float VelCoef,
            float TrajCoef, float TrajVCoef, float TrajAngleCoef)
{
  float PosDiffSum = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 Diff = A.BonePs[b] - B.BonePs[b];
    PosDiffSum += Math::Dot(Diff, Diff);
  }

  float VelDiffSum = 0.0f;
  for(int b = 0; b < MM_COMPARISON_BONE_COUNT; b++)
  {
    vec3 VelDiff = A.BoneVs[b] - B.BoneVs[b];
    VelDiffSum += Math::Dot(VelDiff, VelDiff);
  }

  float TrajDiffSum  = 0.0f;
  float TrajVDiffSum = 0.0f;
  for(int p = 0; p < MM_POINT_COUNT; p++)
  {
    vec3 Diff = A.TrajectoryPs[p] - B.TrajectoryPs[p];
    TrajDiffSum += Math::Dot(Diff, Diff);
    float VDiff = A.TrajectoryVs[p] - B.TrajectoryVs[p];
    TrajVDiffSum += VDiff * VDiff;
  }

  float TrajDirDiffSum = 0.0f;
  for(int p = 0; p < MM_POINT_COUNT; p++)
  {
    vec2 DirA = { sinf(A.TrajectoryAngles[p]), cosf(A.TrajectoryAngles[p]) };
    vec2 DirB = { sinf(B.TrajectoryAngles[p]), cosf(B.TrajectoryAngles[p]) };
    vec2 Diff = DirA - DirB;
    TrajDirDiffSum += Math::Dot(Diff, Diff);
  }

  float Cost = PosCoef * PosDiffSum + VelCoef * VelDiffSum + TrajCoef * TrajDiffSum +
               TrajVCoef * TrajVDiffSum + TrajAngleCoef * TrajDirDiffSum;

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
                    MMData->Params.DynamicParams.TrajAngleCoefficient);
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
                    MMData->Params.DynamicParams.TrajAngleCoefficient);
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
                                       MMData->Params.DynamicParams.TrajAngleCoefficient);

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
