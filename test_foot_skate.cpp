#include "test_foot_skate.h"
#include "debug_drawing.h"

foot_skate_data_row
MeasureFootSkate(foot_skate_test* Test, Anim::animation_controller* AnimPlayer,
                 const mm_controller_data*         MMController,
                 const Anim::skeleton_mirror_info* MirrorInfo, const blend_stack* BlendStack,
                 const mat4& EntityModelMatrix, transform LocalDeltaRoot, float t, float dt)
{
  foot_skate_data_row Result = {};

  // NOTE(Lukas) this is stale by one dt
  mat4 LocalDeltaRootMatrix = TransformToMat4(LocalDeltaRoot);

  vec3 CurrentPs[MAX_TEST_BONE_COUNT];
  vec3 FuturePs[MAX_TEST_BONE_COUNT];
  vec3 Velocities[MAX_TEST_BONE_COUNT];

  playback_info PlaybackInfo = { .BlendStack = BlendStack, .MirrorInfo = MirrorInfo };

  const float OriginalAnimPlayerTime = AnimPlayer->GlobalTimeSec;
  const int   HipBoneIndex           = 0;

  // Undo the update so that the root motion which was already applied would be valid
  AnimPlayer->GlobalTimeSec -= dt;
  // FirstPoseSample
  {
    AnimPlayer->BlendFunc(AnimPlayer, &PlaybackInfo);

    Anim::ComputeBoneSpacePoses(AnimPlayer->BoneSpaceMatrices, AnimPlayer->OutputTransforms,
                                AnimPlayer->Skeleton->BoneCount);
    Anim::ComputeModelSpacePoses(AnimPlayer->ModelSpaceMatrices, AnimPlayer->BoneSpaceMatrices,
                                 AnimPlayer->Skeleton);
    Anim::ComputeFinalHierarchicalPoses(AnimPlayer->HierarchicalModelSpaceMatrices,
                                        AnimPlayer->ModelSpaceMatrices, AnimPlayer->Skeleton);
    mat4 InvRootMatrix;
    Anim::GetRootAndInvRootMatrices(NULL, &InvRootMatrix,
                                    AnimPlayer->HierarchicalModelSpaceMatrices[HipBoneIndex]);

    for(int i = 0; i < Test->TestBoneIndices.Count; i++)
    {
      int BoneIndex = Test->TestBoneIndices[i];
      CurrentPs[i] =
        Math::MulMat4(EntityModelMatrix,
                      Math::MulMat4(InvRootMatrix,
                                    Math::MulMat4(AnimPlayer
                                                    ->HierarchicalModelSpaceMatrices[BoneIndex],
                                                  AnimPlayer->Skeleton->Bones[BoneIndex].BindPose)))
          .T;
    }
  }

  AnimPlayer->GlobalTimeSec += dt;
  // Sample CurrentPose Pose
  {
    AnimPlayer->BlendFunc(AnimPlayer, &PlaybackInfo);

    Anim::ComputeBoneSpacePoses(AnimPlayer->BoneSpaceMatrices, AnimPlayer->OutputTransforms,
                                AnimPlayer->Skeleton->BoneCount);
    Anim::ComputeModelSpacePoses(AnimPlayer->ModelSpaceMatrices, AnimPlayer->BoneSpaceMatrices,
                                 AnimPlayer->Skeleton);
    Anim::ComputeFinalHierarchicalPoses(AnimPlayer->HierarchicalModelSpaceMatrices,
                                        AnimPlayer->ModelSpaceMatrices, AnimPlayer->Skeleton);
    mat4 InvRootMatrix;
    Anim::GetRootAndInvRootMatrices(NULL, &InvRootMatrix,
                                    AnimPlayer->HierarchicalModelSpaceMatrices[HipBoneIndex]);

    for(int i = 0; i < Test->TestBoneIndices.Count; i++)
    {
      int BoneIndex = Test->TestBoneIndices[i];
      FuturePs[i] =
        Math::MulMat4(EntityModelMatrix,
                      Math::MulMat4(LocalDeltaRootMatrix,
                                    Math::MulMat4(InvRootMatrix,
                                                  Math::MulMat4(AnimPlayer
                                                                  ->HierarchicalModelSpaceMatrices
                                                                    [BoneIndex],
                                                                AnimPlayer->Skeleton
                                                                  ->Bones[BoneIndex]
                                                                  .BindPose))))
          .T;
    }
  }

  // Compute Local Velocities
  for(int i = 0; i < Test->TestBoneIndices.Count; i++)
  {
    Debug::PushWireframeSphere(CurrentPs[i], 0.02f, { 0, 0, 0, 1 });

    Velocities[i] = (FuturePs[i] - CurrentPs[i]) / dt;

    // Start the visualization from the future as that is already where the model will be drawn
    Debug::PushWireframeSphere(FuturePs[i] + Velocities[i], 0.02f, { 1, 1, 0, 1 });
    float t =
      (ClampFloat(Test->BottomMargin, CurrentPs[i].Y, Test->TopMargin) - Test->BottomMargin) /
      MaxFloat(Test->TopMargin - Test->BottomMargin, 0.001f);

    vec4 Color = (1 - t) * vec4{ 1, 0, 0, 1 } + t * vec4{ 1, 1, 0, 1 };

    Debug::PushLine(FuturePs[i], FuturePs[i] + Velocities[i], Color);
  }

  // Compute Global Velocity

  // Visualize Velocity Colored By Height

  // Debug visualize velocities
  AnimPlayer->GlobalTimeSec = OriginalAnimPlayerTime;

  Result.t               = t;
  Result.dt              = dt;
  Result.LeftFootHeight  = CurrentPs[0].Y;
  Result.RightFootHeight = CurrentPs[1].Y;

  Result.LeftFootXVel  = Velocities[0].X;
  Result.LeftFootYVel  = Velocities[0].Y;
  Result.LeftFootZVel  = Velocities[0].Z;
  Result.RightFootXVel = Velocities[1].X;
  Result.RightFootYVel = Velocities[1].Y;
  Result.RightFootZVel = Velocities[1].Z;
  Result.AnimCount     = BlendStack->Count;
  assert(BlendStack->Count == AnimPlayer->AnimStateCount);
  Result.LocalAnimTime =
    Anim::GetLocalSampleTime(AnimPlayer, AnimPlayer->AnimStateCount - 1, AnimPlayer->GlobalTimeSec);
  Result.AnimIsMirrored = BlendStack->Peek().Mirror;
  for(int i = 0; i < MMController->Animations.Count; i++)
  {
    if(BlendStack->Peek().Animation == MMController->Animations[i])
    {
      Result.AnimIndex = i;
      break;
    }
  }
  assert(Result.AnimIndex != -1);

  return Result;
}

foot_skate_data_row
MeasureFootSkate(Memory::stack_allocator* TempAlloc, foot_skate_test* Test,
                 const Anim::skeleton* Skeleton, const Anim::animation* Anim, float t,
                 float dt = 1 / 60.0f)
{
  Memory::marker      StackMarker = TempAlloc->GetMarker();
  foot_skate_data_row Result      = {};
  assert(dt != 0);
  float LocalAnimTime = t;

  transform* TempTransforms = PushArray(TempAlloc, Skeleton->BoneCount, transform);
  mat4*      TempMatrices   = PushArray(TempAlloc, Skeleton->BoneCount, mat4);

  vec3 CurrentPs[MAX_TEST_BONE_COUNT];
  vec3 FuturePs[MAX_TEST_BONE_COUNT];

  int32_t HipIndex = 0;
  {
    Anim::LinearAnimationSample(TempTransforms, Anim, LocalAnimTime);
    Anim::ComputeBoneSpacePoses(TempMatrices, TempTransforms, Skeleton->BoneCount);
    Anim::ComputeModelSpacePoses(TempMatrices, TempMatrices, Skeleton);
    Anim::ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Skeleton);

    for(int i = 0; i < Test->TestBoneIndices.Count; i++)
    {
      int BoneIndex = Test->TestBoneIndices[i];
      CurrentPs[i]  = Math::MulMat4(TempMatrices[BoneIndex], Skeleton->Bones[BoneIndex].BindPose).T;
    }
  }
  {
    Anim::LinearAnimationSample(TempTransforms, Anim, LocalAnimTime + dt);
    Anim::ComputeBoneSpacePoses(TempMatrices, TempTransforms, Skeleton->BoneCount);
    Anim::ComputeModelSpacePoses(TempMatrices, TempMatrices, Skeleton);
    Anim::ComputeFinalHierarchicalPoses(TempMatrices, TempMatrices, Skeleton);

    for(int i = 0; i < Test->TestBoneIndices.Count; i++)
    {
      int BoneIndex = Test->TestBoneIndices[i];
      FuturePs[i]   = Math::MulMat4(TempMatrices[BoneIndex], Skeleton->Bones[BoneIndex].BindPose).T;
    }
  }

  vec3 Velocities[MAX_TEST_BONE_COUNT];
  for(int i = 0; i < Test->TestBoneIndices.Count; i++)
  {
    Debug::PushWireframeSphere(CurrentPs[i], 0.02f, { 0, 0, 0, 1 });

    Velocities[i] = (FuturePs[i] - CurrentPs[i]) / dt;

    Debug::PushWireframeSphere(CurrentPs[i] + Velocities[i], 0.02f, { 1, 1, 0, 1 });
    float t =
      (ClampFloat(Test->BottomMargin, CurrentPs[i].Y, Test->TopMargin) - Test->BottomMargin) /
      MaxFloat(Test->TopMargin - Test->BottomMargin, 0.001f);

    vec4 Color = (1 - t) * vec4{ 1, 0, 0, 1 } + t * vec4{ 1, 1, 0, 1 };

    Debug::PushLine(CurrentPs[i], CurrentPs[i] + Velocities[i], Color);
  }

  Result.t               = LocalAnimTime;
  Result.dt              = dt;
  Result.LeftFootHeight  = CurrentPs[0].Y;
  Result.RightFootHeight = CurrentPs[1].Y;

  Result.LeftFootXVel   = Velocities[0].X;
  Result.LeftFootYVel   = Velocities[0].Y;
  Result.LeftFootZVel   = Velocities[0].Z;
  Result.RightFootXVel  = Velocities[1].X;
  Result.RightFootYVel  = Velocities[1].Y;
  Result.RightFootZVel  = Velocities[1].Z;
  Result.AnimCount      = 1;
  Result.AnimIndex      = 0;
  Result.LocalAnimTime  = LocalAnimTime;
  Result.AnimIsMirrored = 0;

  TempAlloc->FreeToMarker(StackMarker);
  return Result;
}
