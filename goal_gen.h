#pragma once

#include "motion_matching.h"
#include "misc.h"

#define HALF_TRAJECTORY_TRANSFORM_COUNT 60
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

struct entity_goal_input
{
  int32_t EntityIndex;
  vec3    WorldDir;
};

inline void
InitTrajectory(trajectory* OutTrajectory)
{
  for(int i = 0; i < 2 * HALF_TRAJECTORY_TRANSFORM_COUNT; i++)
  {
    OutTrajectory->Transforms[i].T = {};
    OutTrajectory->Transforms[i].R = Math::QuatIdent();
  }
}

inline void
GetGoalAndUpdateTrajectory(mm_frame_info* OutGoal, trajectory* Trajectory,
                           const mat4& InvEntityMatrix, const float PositionBias,
                           const float DirectionBias, vec3 DesiredLocalVelocity,
                           vec3 DesiredLocalFacing)
{
  const float SampleFrequency = HALF_TRAJECTORY_TRANSFORM_COUNT;

  mat3 EntityMatrix    = Math::Mat4ToMat3(Math::InvMat4(InvEntityMatrix));
  vec3 DesiredFacing   = Math::MulMat3Vec3(EntityMatrix, DesiredLocalFacing);
  vec3 DesiredVelocity = Math::MulMat3Vec3(EntityMatrix, DesiredLocalVelocity);

  // Update the trajectory transform array
  vec2 DesiredLinearDisplacement = vec2{ DesiredVelocity.X, DesiredVelocity.Z } / SampleFrequency;

  if(Math::Length(DesiredVelocity) > 0.01f)
  {
    Trajectory->TargetAngle = atan2f(DesiredFacing.X, DesiredFacing.Z);
  }

  quat TargetRotation = Math::QuatAxisAngle({ 0, 1, 0 }, Trajectory->TargetAngle);

  vec2 TrajectoryPositions[HALF_TRAJECTORY_TRANSFORM_COUNT];
  TrajectoryPositions[0] = Trajectory->Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT].T;

  for(int i = 1; i < HALF_TRAJECTORY_TRANSFORM_COUNT; i++)
  {
    float Fraction         = float(i) / float(HALF_TRAJECTORY_TRANSFORM_COUNT - 1);
    float OneMinusFraction = 1.0f - Fraction;
    float TranslationBlend = 1.0f - powf(OneMinusFraction, PositionBias);
    float RotationBlend    = 1.0f - powf(OneMinusFraction, DirectionBias);

    vec2 TrajectoryPointDisplacement =
      Trajectory->Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT + i].T -
      Trajectory->Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT + i - 1].T;
    vec2 AdjustedPointDisplacement = (1 - TranslationBlend) * TrajectoryPointDisplacement +
                                     TranslationBlend * DesiredLinearDisplacement;

    TrajectoryPositions[i] = TrajectoryPositions[i - 1] + AdjustedPointDisplacement;

    Trajectory->Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT + i].R =
      Math::QuatLerp(Trajectory->Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT].R, TargetRotation,
                     RotationBlend);
  }

  for(int i = 1; i < HALF_TRAJECTORY_TRANSFORM_COUNT; i++)
  {
    Trajectory->Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT + i].T = TrajectoryPositions[i];
  }

  // vec2 OriginalNext = Trajectory->Transforms[1].T;
  for(int i = 0; i < HALF_TRAJECTORY_TRANSFORM_COUNT; i++)
  {
    Trajectory->Transforms[i] = Trajectory->Transforms[i + 1];
  }

  //
  {
    // Generate a goal from this array
    for(int i = 0; i < MM_POINT_COUNT; i++)
    {
      int TrajectoryPointIndex =
        int(float(i + 1) * float(HALF_TRAJECTORY_TRANSFORM_COUNT - 1) / float(MM_POINT_COUNT));

      trajectory_transform PointTransform =
        Trajectory->Transforms[HALF_TRAJECTORY_TRANSFORM_COUNT + TrajectoryPointIndex];

      OutGoal->TrajectoryPs[i] =
        Math::MulMat4Vec4(InvEntityMatrix, { PointTransform.T.X, 0, PointTransform.T.Y, 1 }).XYZ;

      vec3 LocalDirection =
        Math::MulMat3Vec3(Math::Mat4ToMat3(InvEntityMatrix),
                          Math::MulMat3Vec3(Math::QuatToMat3(PointTransform.R), { 0, 0, 1 }));
      OutGoal->TrajectoryAngles[i] = atan2f(LocalDirection.X, LocalDirection.Z);
    }
  }
}

struct trajectory_update_args
{
  float PositionBias;
  float DirectionBias;
  mat4  InvEntityMatrix;
};

inline void
GetLongtermGoal(mm_frame_info* OutGoal, trajectory* Trajectory, vec3 StartVelocity,
                vec3 DesiredVelocity, vec3 DesiredFacing, float TimeHorizon,
                const trajectory_update_args* TrajectoryArgs)
{
  if(TrajectoryArgs)
  {
    GetGoalAndUpdateTrajectory(OutGoal, Trajectory, TrajectoryArgs->InvEntityMatrix,
                               TrajectoryArgs->PositionBias, TrajectoryArgs->DirectionBias,
                               DesiredVelocity, DesiredFacing);
  }
  else
  {
    const float Step       = 1 / 60.0f;
    float       PointDelta = TimeHorizon / MM_POINT_COUNT;

    assert(Math::Length(DesiredFacing) > 0.5f);
    float GoalAngle = atan2f(DesiredFacing.X, DesiredFacing.Z);

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
        float t = (p + 1) / float(MM_POINT_COUNT);

        OutGoal->TrajectoryAngles[p] = t * GoalAngle;
      }
    }
  }
}

inline void
GetPoseGoal(mm_frame_info* OutPose, mm_frame_info* OutMirrorPose, vec3* OutRootVelocity,
            vec3* OutMirrorRootVelocity, Memory::stack_allocator* TempAlloc,
            const Anim::skeleton* Skeleton, const Anim::animation* Animation, float LocalAnimTime,
            const mm_fixed_params& Params, const float Delta = 1 / 60.0f)
{
  Memory::marker StackMarker = TempAlloc->GetMarker();

  const bool GenerateMirrorInfo =
    Params.MirrorBoneIndices.Count == Params.ComparisonBoneIndices.Count;

  // Allocate temporary transforms and matrices
  transform* TempTransforms = PushArray(TempAlloc, Skeleton->BoneCount, transform);
  mat4*      TempMatrices   = PushArray(TempAlloc, Skeleton->BoneCount, mat4);

  mat4    CurrentRootMatrix;
  mat4    InvCurrentRootMatrix;
  mat4    NextRootMatrix;
  mat4    InvNextRootMatrix;
  int32_t HipIndex = 0;
  {
    // Sample the most recent animation's current frame
    {
      Anim::LinearAnimationSample(TempTransforms, Animation, LocalAnimTime);
      Anim::ComputeBoneSpacePoses(TempMatrices, TempTransforms, Skeleton->BoneCount);
      Anim::ComputeModelSpacePoses(TempMatrices, TempMatrices, Skeleton);
      Anim::ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Skeleton);
    }
    Anim::GetRootAndInvRootMatrices(&CurrentRootMatrix, &InvCurrentRootMatrix,
                                    Math::MulMat4(TempMatrices[HipIndex],
                                                  Skeleton->Bones[HipIndex].BindPose));

    // Store the current positions
    for(int b = 0; b < Params.ComparisonBoneIndices.Count; b++)
    {
      // Positions of bones used for matching
      if(OutPose)
      {
        int BoneIndex = Params.ComparisonBoneIndices[b];
        OutPose->BonePs[b] =
          Math::MulMat4(InvCurrentRootMatrix,
                        Math::MulMat4(TempMatrices[BoneIndex], Skeleton->Bones[BoneIndex].BindPose))
            .T;
      }

      // Positions of the mirroring bones used for matching
      if(OutMirrorPose && GenerateMirrorInfo)
      {
        int BoneIndex = Params.MirrorBoneIndices[b];
        OutMirrorPose->BonePs[b] =
          Math::MulMat4(InvCurrentRootMatrix,
                        Math::MulMat4(TempMatrices[BoneIndex], Skeleton->Bones[BoneIndex].BindPose))
            .T;
      }
    }
  }

  // Compute bone velocities
  vec3 RootVelocity = {};
  {

    // Sample the most recent animation's next frame
    {
      Anim::LinearAnimationSample(TempTransforms, Animation, LocalAnimTime + Delta);
      Anim::ComputeBoneSpacePoses(TempMatrices, TempTransforms, Skeleton->BoneCount);
      ComputeModelSpacePoses(TempMatrices, TempMatrices, Skeleton);
      ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Skeleton);
    }

    // Compute bone linear velocities
    {
      Anim::GetRootAndInvRootMatrices(&NextRootMatrix, &InvNextRootMatrix,
                                      Math::MulMat4(TempMatrices[HipIndex],
                                                    Skeleton->Bones[HipIndex].BindPose));
      for(int b = 0; b < Params.ComparisonBoneIndices.Count; b++)
      {
        if(OutPose)
        {
          int BoneIndex = Params.ComparisonBoneIndices[b];
          OutPose->BoneVs[b] =
            (Math::MulMat4(InvNextRootMatrix, Math::MulMat4(TempMatrices[BoneIndex],
                                                            Skeleton->Bones[BoneIndex].BindPose))
               .T -
             OutPose->BonePs[b]) /
            Delta;
        }
        if(OutMirrorPose && GenerateMirrorInfo)
        {
          int BoneIndex = Params.MirrorBoneIndices[b];
          OutMirrorPose->BoneVs[b] =
            (Math::MulMat4(InvNextRootMatrix, Math::MulMat4(TempMatrices[BoneIndex],
                                                            Skeleton->Bones[BoneIndex].BindPose))
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
    *OutMirrorPose         = *OutPose;
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
          Memory::stack_allocator* TempAlloc, const Anim::skeleton* Skeleton,
          const Anim::animation* Animation, bool PlayingMirrored, float LocalAnimTime,
          vec3 DesiredVelocity, vec3 DesiredFacing, float TimeHorizon,
          const mm_fixed_params& Params, const trajectory_update_args* TrajectoryArgs)
{
  mm_frame_info AnimPose         = {};
  mm_frame_info MirroredAnimPose = {};

  vec3 AnimVelocity         = {};
  vec3 MirroredAnimVelocity = {};

  GetPoseGoal(&AnimPose, &MirroredAnimPose, &AnimVelocity, &MirroredAnimVelocity, TempAlloc,
              Skeleton, Animation, LocalAnimTime, Params);

  if(PlayingMirrored)
  {
    *OutGoal = MirroredAnimPose;
    GetLongtermGoal(OutGoal, ControlTrajectory, MirroredAnimVelocity, DesiredVelocity,
                    DesiredFacing, TimeHorizon, TrajectoryArgs);

    *OutMirroredGoal = AnimPose;
  }
  else
  {
    *OutGoal = AnimPose;
    GetLongtermGoal(OutGoal, ControlTrajectory, AnimVelocity, DesiredVelocity, DesiredFacing,
                    TimeHorizon, TrajectoryArgs);

    *OutMirroredGoal = MirroredAnimPose;
  }
  CopyLongtermGoalFromRightToLeft(OutMirroredGoal, *OutGoal);
  MirrorLongtermGoal(OutMirroredGoal);
}

