#pragma once

#include "motion_matching.h"
#include "misc.h"

#define HALF_TRAJECTORY_TRANSFORM_COUNT 30
#define TRAJECTORY_TRANSFORM_COUNT (2 * HALF_TRAJECTORY_TRANSFORM_COUNT)

struct trajectory_transform
{
  vec2 T;
  quat R;
};

struct trajectory
{
  trajectory_transform Transforms[2 * HALF_TRAJECTORY_TRANSFORM_COUNT];
  float                TargetAngle;
};

inline void
InitTrajectory(trajectory* OutTrajectory)
{
	for(int i = 0; i < 2*HALF_TRAJECTORY_TRANSFORM_COUNT; i++)
	{
    OutTrajectory->Transforms[i].T = {};
    OutTrajectory->Transforms[i].R = Math::QuatIdent();
  }
}

inline void
GetLongtermGoal(mm_frame_info* OutGoal, trajectory* Trajectory, vec3 StartVelocity,
                vec3 DesiredVelocity, float dt)
{
  const float TimeHorizon = 1.0f;
  const float Step       = 1 / 60.0f;
  float       PointDelta = TimeHorizon / MM_POINT_COUNT;

#define START_ANGLE_IS_0

#ifdef START_ANGLE_IS_0
  quat StartRotation = Math::QuatIdent();
  assert(acosf(StartRotation.S) == 0);
  float GoalAngle       = atan2f(DesiredVelocity.X, DesiredVelocity.Z);
  quat  GoalRotation    = Math::QuatAxisAngle({ 0, 1, 0 }, GoalAngle);
#endif

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

    OutGoal->TrajectoryPs[p] = CurrentPoint;
    OutGoal->TrajectoryVs[p] = Math::Length(CurrentVelocity);
    {
      vec3  InitialXAxis = Math::Normalized(StartVelocity);
      vec3  PointXAxis   = Math::Normalized(CurrentVelocity);
      float CrossY       = Math::Cross(InitialXAxis, PointXAxis).Y;
      float AbsAngle     = acosf(ClampFloat(-1.0f, Math::Dot(InitialXAxis, PointXAxis), 1.0f));
#ifdef START_ANGLE_IS_0
      OutGoal->TrajectoryAngles[p] =
        acosf(Math::QuatLerp(StartRotation, GoalRotation, (p + 1) / float(MM_POINT_COUNT)).S);
#else
      OutGoal->TrajectoryAngles[p] = (0 <= CrossY) ? AbsAngle : -AbsAngle;
#endif
    }
  }
}

inline void
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

inline void
CopyLongtermGoalFromRightToLeft(mm_frame_info* Dest, mm_frame_info Src)
{
  for(int i = 0; i < MM_POINT_COUNT; i++)
  {
    Dest->TrajectoryPs[i]     = Src.TrajectoryPs[i];
    Dest->TrajectoryVs[i]     = Src.TrajectoryVs[i];
    Dest->TrajectoryAngles[i] = Src.TrajectoryAngles[i];
  }
}

inline void
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

inline void
GetMMGoal(mm_frame_info* OutGoal, mm_frame_info* OutMirroredGoal, trajectory* ControlTrajectory,
          Memory::stack_allocator* TempAlloc, int32_t AnimStateIndex, bool PlayingMirrored,
          const Anim::animation_controller* Controller, vec3 DesiredVelocity,
          const mm_fixed_params& Params, float dt)
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
    GetLongtermGoal(OutGoal, ControlTrajectory, MirroredAnimVelocity, DesiredVelocity, dt);

    *OutMirroredGoal = AnimPose;
  }
  else
  {
    *OutGoal = AnimPose;
    GetLongtermGoal(OutGoal, ControlTrajectory, AnimVelocity, DesiredVelocity, dt);

    *OutMirroredGoal = MirroredAnimPose;
  }
  CopyLongtermGoalFromRightToLeft(OutMirroredGoal, *OutGoal);
  MirrorLongtermGoal(OutMirroredGoal);
}

