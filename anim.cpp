#include "anim.h"

void
Anim::LerpTransforms(const Anim::transform* InA, const Anim::transform* InB, int TransformCount,
                     float T, Anim::transform* Out)
{
  assert(0.0f <= T && T <= 1);
  float KoefA = (1 - T);
  float KoefB = T;
  for(int i = 0; i < TransformCount; i++)
  {
    Out[i].Translation = KoefA * InA[i].Translation + KoefB * InB[i].Translation;
    Out[i].Rotation    = KoefA * InA[i].Rotation + KoefB * InB[i].Rotation;
    Out[i].Scale       = KoefA * InA[i].Scale + KoefB * InB[i].Scale;
  }
}

void
Anim::LinearAnimationSample(const Anim::animation* Animation, float Time,
                            Anim::transform* OutputTransforms)
{
  for(int k = 0; k < Animation->KeyframeCount - 1; k++)
  {
    if(Time <= Animation->SampleTimes[k + 1])
    {
      LerpTransforms(&Animation->Transforms[k * Animation->ChannelCount],
                     &Animation->Transforms[(k + 1) * Animation->ChannelCount],
                     Animation->ChannelCount,
                     (Time - Animation->SampleTimes[k]) /
                       (Animation->SampleTimes[k + 1] - Animation->SampleTimes[k]),
                     OutputTransforms);
      return;
    }
  }
}

void
Anim::SampleAtGlobalTime(Anim::animation_controller* Controller, int AnimationIndex,
                         int OutputBlockIndex)
{
  assert(0 <= OutputBlockIndex && OutputBlockIndex < ANIM_CONTROLLER_MAX_ANIM_COUNT);

  const animation_state* State     = &Controller->States[AnimationIndex];
  const animation*       Animation = Controller->Animations[AnimationIndex];
  const float            AnimDuration =
    (Animation->SampleTimes[Animation->KeyframeCount - 1] - Animation->SampleTimes[0]);
  float SampleTime = State->PlaybackRateSec * (Controller->GlobalTimeSec - State->StartTimeSec);
  if(State->Loop)
  {
    SampleTime = SampleTime - AnimDuration * (float)((int)(SampleTime / AnimDuration));
  }
  else if(AnimDuration < SampleTime)
  {
    SampleTime = AnimDuration;
  }
  LinearAnimationSample(Animation, SampleTime,
                        &Controller->OutputTransforms[OutputBlockIndex * Animation->ChannelCount]);
}

void
Anim::UpdateController(Anim::animation_controller* Controller, float dt)
{
  Controller->GlobalTimeSec += dt;
  if(0 < Controller->AnimStateCount)
  {
    SampleAtGlobalTime(Controller, 0, 0);
  }
  else
  {
    for(int i = 0; i < Controller->Skeleton->BoneCount; i++)
    {
      Controller->OutputTransforms[i]       = {};
      Controller->OutputTransforms[i].Scale = { 1, 1, 1 };
    }
  }
  ComputeBoneSpacePoses(Controller->BoneSpaceMatrices, Controller->OutputTransforms,
                        Controller->Skeleton->BoneCount);
  ComputeModelSpacePoses(Controller->ModelSpaceMatrices, Controller->BoneSpaceMatrices,
                         Controller->Skeleton);
  ComputeFinalHierarchicalPoses(Controller->HierarchicalModelSpaceMatrices,
                                Controller->ModelSpaceMatrices, Controller->Skeleton);
}

void
Anim::AddAnimation(Anim::animation_controller* AnimController, Anim::animation* Animation)
{
  if(0 <= AnimController->AnimStateCount &&
     AnimController->AnimStateCount < ANIM_CONTROLLER_MAX_ANIM_COUNT)
  {
    if(AnimController->Skeleton->BoneCount == Animation->ChannelCount)
    {
      AnimController->States[AnimController->AnimStateCount]       = {};
      AnimController->Animations[AnimController->AnimStateCount++] = Animation;
    }
    else
    {
      printf("anim error: wrong number of animation channels for skeleton\n");
    }
  }
  else
  {
    assert(false && "assert: overflowed animation array in animation_controller");
  }
}

void
Anim::SetAnimation(Anim::animation_controller* AnimController, Anim::animation* Animation,
                   int32_t ControllerIndex)
{
  if(0 <= ControllerIndex && ControllerIndex < AnimController->AnimStateCount)
  {
    if(AnimController->Skeleton->BoneCount == Animation->ChannelCount)
    {
      AnimController->States[ControllerIndex]     = {};
      AnimController->Animations[ControllerIndex] = Animation;
    }
    else
    {
      printf("anim error: wrong number of animation channels for skeleton\n");
    }
  }
  else
  {
    assert(false && "assert: overflowed animation array in animation_controller");
  }
}

void
Anim::StartAnimationAtGlobalTime(Anim::animation_controller* AnimController, int AnimationIndex,
                                 bool Loop)
{
  assert(0 <= AnimationIndex && AnimationIndex <= ANIM_CONTROLLER_MAX_ANIM_COUNT);
  AnimController->States[AnimationIndex]                 = {};
  AnimController->States[AnimationIndex].StartTimeSec    = AnimController->GlobalTimeSec;
  AnimController->States[AnimationIndex].IsPlaying       = true;
  AnimController->States[AnimationIndex].PlaybackRateSec = 1.0f;
  AnimController->States[AnimationIndex].Loop            = Loop;
}

void
Anim::ComputeBoneSpacePoses(mat4* BoneSpaceMatrices, const Anim::transform* Transforms, int Count)
{
  for(int i = 0; i < Count; i++)
  {
    BoneSpaceMatrices[i] =
      Math::MulMat4(Math::Mat4Translate(Transforms[i].Translation),
                    Math::MulMat4(Math::Mat4Rotate(Transforms[i].Rotation), Math::Mat4Scale(1)));
  }
}

void
Anim::ComputeModelSpacePoses(mat4* ModelSpaceMatrices, const mat4* BoneSpaceMatrices,
                             const Anim::skeleton* Skeleton)
{
  for(int i = 0; i < Skeleton->BoneCount; i++)
  {
    const Anim::bone* Bone = Skeleton->Bones + i;
    ModelSpaceMatrices[i] =
      Math::MulMat4(Bone->BindPose, Math::MulMat4(BoneSpaceMatrices[i], Bone->InverseBindPose));
  }
}

void
Anim::ComputeFinalHierarchicalPoses(mat4* FinalPoseMatrices, const mat4* ModelSpaceMatrices,
                                    const Anim::skeleton* Skeleton)
{
  // Assume LocalPoses are ordered from parent to child
  FinalPoseMatrices[0] = ModelSpaceMatrices[0];
  for(int i = 1; i < Skeleton->BoneCount; i++)
  {
    FinalPoseMatrices[i] =
      Math::MulMat4(FinalPoseMatrices[Skeleton->Bones[i].ParentIndex], ModelSpaceMatrices[i]);
  }
}
