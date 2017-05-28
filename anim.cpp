#include "anim.h"
#include "resource_manager.h"

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
  if(State->Loop && AnimDuration < SampleTime)
  {
    SampleTime = SampleTime - AnimDuration * (float)((int)(SampleTime / AnimDuration));
  }
  else if(AnimDuration < SampleTime)
  {
    SampleTime = AnimDuration;
  }
  LinearAnimationSample(Controller, AnimationIndex, SampleTime, OutputBlockIndex);
}

void
Anim::UpdateController(Anim::animation_controller* Controller, float dt,
                       void BlendFunc(animation_controller*))
{
  Controller->GlobalTimeSec += dt;
  if(0 < Controller->AnimStateCount)
  {
    if(BlendFunc == NULL)
    {
      SampleAtGlobalTime(Controller, 0, 0);
    }
    else
    {
      BlendFunc(Controller);
    }
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
Anim::AddAnimation(Anim::animation_controller* Controller, rid AnimationID)
{
  assert(0 <= Controller->AnimStateCount &&
         Controller->AnimStateCount < ANIM_CONTROLLER_MAX_ANIM_COUNT);
  assert(AnimationID.Value > 0);
  Controller->States[Controller->AnimStateCount]         = {};
  Controller->AnimationIDs[Controller->AnimStateCount++] = AnimationID;
}

void
Anim::SetAnimation(Anim::animation_controller* Controller, rid AnimationID, int32_t ControllerIndex)
{
  assert(0 <= ControllerIndex && ControllerIndex < ANIM_CONTROLLER_MAX_ANIM_COUNT);
  assert(AnimationID.Value > 0);
  Controller->States[ControllerIndex] = {};

  if(Controller->AnimationIDs[ControllerIndex].Value == 0)
  {
    Controller->AnimationIDs[ControllerIndex] = AnimationID;
    ++Controller->AnimStateCount;
  }
  else
  {
    Controller->AnimationIDs[ControllerIndex] = AnimationID;
  }
}

void
Anim::StartAnimationAtGlobalTime(Anim::animation_controller* Controller, int AnimationIndex,
                                 bool Loop)
{
  assert(0 <= AnimationIndex && AnimationIndex <= ANIM_CONTROLLER_MAX_ANIM_COUNT);
  Controller->States[AnimationIndex]                 = {};
  Controller->States[AnimationIndex].StartTimeSec    = Controller->GlobalTimeSec;
  Controller->States[AnimationIndex].IsPlaying       = true;
  Controller->States[AnimationIndex].PlaybackRateSec = 1.0f;
  Controller->States[AnimationIndex].Loop            = Loop;
}

void
Anim::StopAnimation(Anim::animation_controller* Controller, int AnimationIndex)
{
  assert(0 <= AnimationIndex && AnimationIndex <= ANIM_CONTROLLER_MAX_ANIM_COUNT);
  Controller->States[AnimationIndex] = {};
  Controller->AnimStateCount         = 0;
}

void
Anim::SetPlaybackRate(Anim::animation_controller* Controller, int32_t Index, float Rate)
{
  assert(Rate >= 0);
  assert(0 <= Index && Index <= Controller->AnimStateCount);
  Controller->States[Index].PlaybackRateSec = Rate;
}

void
Anim::LerpTransforms(const Anim::transform* InA, const Anim::transform* InB, int TransformCount,
                     float T, Anim::transform* Out)
{
  float KoefA = (1.0f - T);
  float KoefB = T;
  for(int i = 0; i < TransformCount; i++)
  {
    Out[i].Translation = KoefA * InA[i].Translation + KoefB * InB[i].Translation;
    Out[i].Rotation    = KoefA * InA[i].Rotation + KoefB * InB[i].Rotation;
    Out[i].Scale       = KoefA * InA[i].Scale + KoefB * InB[i].Scale;
  }
}

void
Anim::AddTransforms(const Anim::transform* InA, const Anim::transform* InB, int TransformCount,
                    float T, Anim::transform* Out)
{
  for(int i = 0; i < TransformCount; i++)
  {
    Out[i].Translation = InA[i].Translation + InB[i].Translation * T;
    Out[i].Rotation    = InA[i].Rotation + InB[i].Rotation * T;
    // Out[i].Scale       = InA[i].Scale + InB[i].Scale * T;
  }
}

void
Anim::LinearBlend(animation_controller* Controller, int AnimAInd, int AnimBInd, float t,
                  int ResultIndex)
{
  assert(0 <= AnimAInd && AnimAInd < ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT);
  assert(0 <= AnimBInd && AnimBInd < ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT);
  assert(0 <= ResultIndex && ResultIndex < ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT);
  const int ChannelCount = Controller->Skeleton->BoneCount;
  LerpTransforms(&Controller->OutputTransforms[AnimAInd * ChannelCount],
                 &Controller->OutputTransforms[AnimBInd * ChannelCount], ChannelCount, t,
                 &Controller->OutputTransforms[ResultIndex * ChannelCount]);
}

void
Anim::AdditiveBlend(animation_controller* Controller, int AnimAInd, int AnimBInd, float t,
                    int ResultIndex)
{
  assert(0 <= AnimAInd && AnimAInd < ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT);
  assert(0 <= AnimBInd && AnimBInd < ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT);
  assert(0 <= ResultIndex && ResultIndex < ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT);
  const int ChannelCount = Controller->Skeleton->BoneCount;
  AddTransforms(&Controller->OutputTransforms[AnimAInd * ChannelCount],
                &Controller->OutputTransforms[AnimBInd * ChannelCount], ChannelCount, t,
                &Controller->OutputTransforms[ResultIndex * ChannelCount]);
}

void
Anim::LinearAnimationSample(animation_controller* Controller, int AnimIndex, float Time,
                            int ResultIndex)
{
  assert(0 <= AnimIndex && AnimIndex < Controller->AnimStateCount);
  assert(0 <= ResultIndex && ResultIndex < ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT);
  animation* Animation = Controller->Animations[AnimIndex];
  for(int k = 0; k < Animation->KeyframeCount - 1; k++)
  {
    if(Time <= Animation->SampleTimes[k + 1])
    {
      LerpTransforms(&Animation->Transforms[k * Animation->ChannelCount],
                     &Animation->Transforms[(k + 1) * Animation->ChannelCount],
                     Animation->ChannelCount,
                     (Time - Animation->SampleTimes[k]) /
                       (Animation->SampleTimes[k + 1] - Animation->SampleTimes[k]),
                     &Controller->OutputTransforms[Animation->ChannelCount * ResultIndex]);
      return;
    }
  }
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
