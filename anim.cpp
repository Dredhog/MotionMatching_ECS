#include "anim.h"
#include "misc.h"
#include "basic_data_structures.h"

float
Anim::GetLocalSampleTime(const Anim::animation_controller* Controller, int AnimationIndex,
                         float GlobalTimeSec)
{
  const animation_state* State     = &Controller->States[AnimationIndex];
  const animation*       Animation = Controller->Animations[AnimationIndex];
  return Anim::GetLocalSampleTime(Animation, State, GlobalTimeSec);
}

float
Anim::GetLocalSampleTime(const Anim::animation* Animation, const Anim::animation_state* AnimState,
                         float GlobalSampleTime)
{
  return Anim::GetLocalSampleTime(Animation, GlobalSampleTime, AnimState->StartTimeSec,
                                  AnimState->Loop, AnimState->PlaybackRateSec);
}

float
Anim::GetLocalSampleTime(const Anim::animation* Animation, float GlobalSampleTime,
                         float GlobalStartTime, bool Loop, float PlaybackRate)
{
	assert(Animation->SampleTimes[0] == 0);
  const float AnimDuration = GetAnimDuration(Animation);

  float LocalSampleTime = PlaybackRate * (GlobalSampleTime - GlobalStartTime);
  if(Loop && AnimDuration < LocalSampleTime)
  {
    LocalSampleTime =
      LocalSampleTime - AnimDuration * (float)((int)(LocalSampleTime / AnimDuration));
  }
  else if(AnimDuration < LocalSampleTime)
  {
    LocalSampleTime = AnimDuration;
  }
  return LocalSampleTime;
}

void
Anim::SampleAtGlobalTime(Anim::animation_controller* Controller, int AnimationIndex,
                         int OutputBlockIndex, const Anim::skeleton_mirror_info* MirrorInfo)
{
  assert(0 <= OutputBlockIndex && OutputBlockIndex < ANIM_CONTROLLER_MAX_ANIM_COUNT);
  float SampleTime =
    Anim::GetLocalSampleTime(Controller, AnimationIndex, Controller->GlobalTimeSec);
  if(MirrorInfo)
  {
    LinearMirroredAnimationSample(Controller, AnimationIndex, SampleTime, OutputBlockIndex,
                                  MirrorInfo);
  }
  else
  {
    LinearAnimationSample(Controller, AnimationIndex, SampleTime, OutputBlockIndex);
  }
}

void
Anim::LinearMirroredAnimationSample(Anim::animation_controller* Controller, int AnimIndex,
                                    float Time, int ResultIndex,
                                    const skeleton_mirror_info* MirrorInfo)
{
  assert(0 <= AnimIndex && AnimIndex < Controller->AnimStateCount);
  assert(0 <= ResultIndex && ResultIndex < ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT);
  const Anim::animation* Animation = Controller->Animations[AnimIndex];
  LinearMirroredAnimationSample(&Controller
                                   ->OutputTransforms[Animation->ChannelCount * ResultIndex],
                                Controller->ModelSpaceMatrices, Controller->Skeleton, Animation,
                                Time, MirrorInfo);
}

void
Anim::LinearMirroredAnimationSample(transform* OutputTransforms, mat4* TempMatrices,
                                    const skeleton* Skeleton, const Anim::animation* Animation,
                                    float Time, const Anim::skeleton_mirror_info* MirrorInfo)
{
  assert(OutputTransforms);
  assert(TempMatrices);
  assert(Skeleton);
  Anim::LinearAnimationSample(OutputTransforms, Animation, Time);

  // Compute the skeleton skinning matrices
  ComputeBoneSpacePoses(TempMatrices, OutputTransforms, Skeleton->BoneCount);
  ComputeModelSpacePoses(TempMatrices, TempMatrices, Skeleton);

#if 1
  assert(MirrorInfo);
  assert(AbsFloat(MirrorInfo->MirrorBasisScales.X) + AbsFloat(MirrorInfo->MirrorBasisScales.Y) +
           AbsFloat(MirrorInfo->MirrorBasisScales.Z) ==
         3.0f);

  mat4 MirrorMatrix = Math::Mat4Scale(MirrorInfo->MirrorBasisScales);
  // Mirror the matrices and put them into their appropriate places
  for(int i = 0; i < MirrorInfo->PairCount; i++)
  {
    int a = MirrorInfo->BoneMirrorIndices[i].a;
    int b = MirrorInfo->BoneMirrorIndices[i].b;

    mat4 MatA = TempMatrices[a];

    mat4 MatB = TempMatrices[b];

    // Mirror a
    MatA = Math::MulMat4(MirrorMatrix, MatA);
    // Mirror b
    MatB = Math::MulMat4(MirrorMatrix, MatB);

    // Undo the handedness change
    MatA.X *= MirrorInfo->MirrorBasisScales.X;
    MatA.Y *= MirrorInfo->MirrorBasisScales.Y;
    MatA.Z *= MirrorInfo->MirrorBasisScales.Z;

    MatB.X *= MirrorInfo->MirrorBasisScales.X;
    MatB.Y *= MirrorInfo->MirrorBasisScales.Y;
    MatB.Z *= MirrorInfo->MirrorBasisScales.Z;

    // Store back to the matrix array
    TempMatrices[a] = MatB;
    TempMatrices[b] = MatA;
  }
#endif

  InverseComputeModelSpacePoses(TempMatrices, TempMatrices, Skeleton);
  InverseComputeBoneSpacePoses(OutputTransforms, TempMatrices, Animation->ChannelCount);
}

void
Anim::UpdateController(Anim::animation_controller* Controller, float dt,
                       void BlendFunc(animation_controller*, void*), void* UserData)
{
  if(0 < Controller->AnimStateCount)
  {
    if(BlendFunc == NULL)
    {
      Controller->GlobalTimeSec += dt;
      SampleAtGlobalTime(Controller, 0, 0);
    }
    else
    {
      BlendFunc(Controller, UserData);
    }
  }
  else
  {
    for(int i = 0; i < Controller->Skeleton->BoneCount; i++)
    {
      Controller->OutputTransforms[i]   = {};
      Controller->OutputTransforms[i].R = Math::QuatIdent();
      Controller->OutputTransforms[i].S = { 1, 1, 1 };
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
Anim::AppendAnimation(Anim::animation_controller* Controller, rid AnimationID)
{
  assert(0 <= Controller->AnimStateCount &&
         Controller->AnimStateCount < ANIM_CONTROLLER_MAX_ANIM_COUNT);
  SetAnimation(Controller, AnimationID, Controller->AnimStateCount);
  Controller->AnimStateCount++;
}

void
Anim::SetAnimation(Anim::animation_controller* Controller, rid AnimationID, int32_t AnimationIndex)
{
  assert(0 <= AnimationIndex && AnimationIndex < ANIM_CONTROLLER_MAX_ANIM_COUNT);
  assert(0 < AnimationID.Value);

  Controller->States[AnimationIndex]       = {};
  Controller->AnimationIDs[AnimationIndex] = AnimationID;
}

void
Anim::StartAnimationAtGlobalTime(Anim::animation_controller* Controller, int AnimationIndex,
                                 bool Loop, float LocalStartTime)
{
  assert(0 <= AnimationIndex && AnimationIndex <= ANIM_CONTROLLER_MAX_ANIM_COUNT);
  Controller->States[AnimationIndex]                 = {};
  Controller->States[AnimationIndex].StartTimeSec    = Controller->GlobalTimeSec - LocalStartTime;
  Controller->States[AnimationIndex].PlaybackRateSec = 1.0f;
  Controller->States[AnimationIndex].Loop            = Loop;
}

void
Anim::StopAnimation(Anim::animation_controller* Controller, int AnimationIndex)
{
  assert(0 <= AnimationIndex && AnimationIndex <= ANIM_CONTROLLER_MAX_ANIM_COUNT);
  Controller->States[AnimationIndex] = {};
  // Controller->AnimStateCount         = 0;
  assert(0 && "Invalid Code Path");
}

void
Anim::LerpTransforms(const transform* InA, const transform* InB, int TransformCount, float T,
                     transform* Out)
{
  float KoefA = (1.0f - T);
  float KoefB = T;
  for(int i = 0; i < TransformCount; i++)
  {
    Out[i].T = KoefA * InA[i].T + KoefB * InB[i].T;
    Out[i].R = Math::QuatLerp(InA[i].R, InB[i].R, T);
    Out[i].S = KoefA * InA[i].S + KoefB * InB[i].S;
  }
}

void
Anim::AddTransforms(const transform* InA, const transform* InB, int TransformCount, float T,
                    transform* Out)
{
  for(int i = 0; i < TransformCount; i++)
  {
    Out[i].T = InA[i].T + InB[i].T * T;
    Out[i].R = InA[i].R + InB[i].R * T;
    // Out[i].S       = InA[i].S + InB[i].S * T;
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
GetKeyframeIndexAndInterpolant(int* K, float* T, const float* SampleTimes, int SampleCount,
                               float Time)
{
  Time = ClampFloat(SampleTimes[0], Time, SampleTimes[SampleCount - 1]);
  for(int k = 0; k < SampleCount - 1; k++)
  {
    if(Time <= SampleTimes[k + 1])
    {
      *K = k;
      *T = (Time - SampleTimes[k]) / (SampleTimes[k + 1] - SampleTimes[k]);
      return;
    }
  }
}

void
Anim::LinearAnimationSample(transform* OutputTransforms, const Anim::animation* Animation,
                            float Time)
{
  int   k;
  float t;
  GetKeyframeIndexAndInterpolant(&k, &t, Animation->SampleTimes, Animation->KeyframeCount, Time);
  LerpTransforms(&Animation->Transforms[k * Animation->ChannelCount],
                 &Animation->Transforms[(k + 1) * Animation->ChannelCount], Animation->ChannelCount,
                 t, OutputTransforms);
}

void
Anim::LinearAnimationSample(Anim::animation_controller* Controller, int AnimIndex, float Time,
                            int ResultIndex)
{
  assert(0 <= AnimIndex && AnimIndex < Controller->AnimStateCount);
  assert(0 <= ResultIndex && ResultIndex < ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT);
  const Anim::animation* Animation = Controller->Animations[AnimIndex];
  LinearAnimationSample(&Controller->OutputTransforms[Animation->ChannelCount * ResultIndex],
                        Animation, Time);
}

transform
Anim::LinearAnimationBoneSample(const Anim::animation* Animation, int BoneIndex, float Time)
{
  transform Result;
  int       k;
  float     t;
  int       ChannelCount = 1;
  GetKeyframeIndexAndInterpolant(&k, &t, Animation->SampleTimes, Animation->KeyframeCount, Time);
  LerpTransforms(&Animation->Transforms[k * Animation->ChannelCount + BoneIndex],
                 &Animation->Transforms[(k + 1) * Animation->ChannelCount + BoneIndex],
                 ChannelCount, t, &Result);
  return Result;
}

float
Anim::GetAnimDuration(const Anim::animation* Animation)
{
  assert(0 < Animation->KeyframeCount);
  return Animation->SampleTimes[Animation->KeyframeCount - 1] - Animation->SampleTimes[0];
}

void
Anim::GetRootAndInvRootMatrices(mat4* OutRootMatrix, mat4* OutInvRootMatrix, mat4 HipMatrix)
{
  vec3 Up      = { 0, 1, 0 };
  vec3 Left    = Math::Normalized(Math::Cross(Up, HipMatrix.Z));
  vec3 Forward = Math::Cross(Left, Up);

  mat4 Mat4Root = Math::Mat4Ident();
  Mat4Root.T    = { HipMatrix.T.X, 0, HipMatrix.T.Z };
  Mat4Root.Y    = Up;
  Mat4Root.X    = Left;
  Mat4Root.Z    = Forward;

  if(OutRootMatrix)
  {
    *OutRootMatrix = Mat4Root;
  }

  if(OutInvRootMatrix)
  {
    *OutInvRootMatrix = Math::InvMat4(Mat4Root);
  }
}

void
Anim::ComputeBoneSpacePoses(mat4* BoneSpaceMatrices, const transform* Transforms, int Count)
{
  for(int i = 0; i < Count; i++)
  {
    BoneSpaceMatrices[i] =
      Math::MulMat4(Math::Mat4Translate(Transforms[i].T), Math::Mat4Rotate(Transforms[i].R));
  }
}

void
Anim::InverseComputeBoneSpacePoses(transform* Transforms, const mat4* BoneSpaceMatrices, int Count)
{
  for(int i = 0; i < Count; i++)
  {
    // Extract bone space translation
    Transforms[i].R = Math::Mat4ToQuat(BoneSpaceMatrices[i]);
    Transforms[i].T = BoneSpaceMatrices[i].T;
    Transforms[i].S = { 1, 1, 1 };
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
Anim::InverseComputeModelSpacePoses(mat4* BoneSpaceMatrices, const mat4* ModelSpaceMatrices,
                                    const Anim::skeleton* Skeleton)
{
  for(int i = 0; i < Skeleton->BoneCount; i++)
  {
    const Anim::bone* Bone = Skeleton->Bones + i;
    BoneSpaceMatrices[i] =
      Math::MulMat4(Bone->InverseBindPose, Math::MulMat4(ModelSpaceMatrices[i], Bone->BindPose));
  }
}

void
Anim::ComputeFinalHierarchicalPoses(mat4* FinalPoseMatrices, const mat4* ModelSpaceMatrices,
                                    const Anim::skeleton* Skeleton)
{
  // Assumes that LocalPoses are ordered from parent to child
  FinalPoseMatrices[0] = ModelSpaceMatrices[0];
  for(int i = 1; i < Skeleton->BoneCount; i++)
  {
    FinalPoseMatrices[i] =
      Math::MulMat4(FinalPoseMatrices[Skeleton->Bones[i].ParentIndex], ModelSpaceMatrices[i]);
  }
}

/*
void
Anim::InverseComputeFinalHierarchicalPoses(mat4* ModelSpaceMatrices, const mat4* FinalPoseMatrices,
                                           const Anim::skeleton* Skeleton)
{
  // Assumes that LocalPoses are ordered from parent to child
  ModelSpaceMatrices[0] = FinalPoseMatrices[0];
  for(int i = Skeleton->BoneCount - 1; 1 <= i; i--)
  {
    ModelSpaceMatrices[i] =
      Math::MulMat4(Math::InvMat4(FinalPoseMatrices[Skeleton->Bones[i].ParentIndex]),
                    FinalPoseMatrices[i]);
  }
}*/

void
Anim::GenerateSkeletonMirroringInfo(Anim::skeleton_mirror_info* OutMirrorInfo,
                                    const Anim::skeleton*       Skeleton)
{
  OutMirrorInfo->MirrorBasisScales = { -1, 1, 1 };
  OutMirrorInfo->PairCount         = 0;

  fixed_stack<int, SKELETON_MAX_BONE_COUNT> RemainingBoneIndices;
  RemainingBoneIndices.Clear();
  for(int i = 0; i < Skeleton->BoneCount; i++)
  {
    RemainingBoneIndices.Push(i);
  }

  const int i = 0;
  while(!RemainingBoneIndices.Empty())
  {
    int A = RemainingBoneIndices[i];

    bool        SearchForLeft  = false;
    bool        SearchForRight = false;
    const char* FirstStart     = &Skeleton->Bones[A].Name[0];
    char*       FirstMiddle;
    char*       FirstEnd;
    size_t      FirstLength = strlen(Skeleton->Bones[A].Name);
    char*       Tmp;
    if((Tmp = strstr(Skeleton->Bones[A].Name, "Left")) != NULL)
    {
      SearchForRight = true;
      FirstMiddle    = Tmp;
      FirstEnd       = FirstMiddle + strlen("Left");
    }
    if((Tmp = strstr(Skeleton->Bones[A].Name, "Right")) != NULL)
    {
      SearchForLeft = true;
      FirstMiddle   = Tmp;
      FirstEnd      = FirstMiddle + strlen("Right");
    }

    if(!SearchForLeft && !SearchForRight)
    {
      RemainingBoneIndices.Remove(i);
      OutMirrorInfo->BoneMirrorIndices[OutMirrorInfo->PairCount++] = { A, A };
      continue;
    }
    assert(SearchForLeft != SearchForRight);

    char        TempBuff[BONE_NAME_LENGTH + 1];
    size_t      FirstStartLength   = FirstMiddle - FirstStart;
    const char* SecondMiddleString = (SearchForRight) ? "Right" : "Left";
    size_t      SecondMiddleLength = strlen(SecondMiddleString);
    strncpy(TempBuff, FirstStart, FirstStartLength);
    strncpy(TempBuff + FirstStartLength, SecondMiddleString, SecondMiddleLength);
    // Using sprintf instead of snprintf to get the '\0' at the end
    strcpy(TempBuff + FirstStartLength + SecondMiddleLength, FirstEnd);

    int B = -1;
    int j = i + 1;
    for(; j < RemainingBoneIndices.Count; j++)
    {
      int BoneIndex = RemainingBoneIndices[j];
      if(strcmp(Skeleton->Bones[BoneIndex].Name, TempBuff) == 0)
      {
        B = BoneIndex;
        break;
      }
    }
    assert(B != -1);

    OutMirrorInfo->BoneMirrorIndices[OutMirrorInfo->PairCount++] = { A, B };
    RemainingBoneIndices.Remove(j);
    RemainingBoneIndices.Remove(i);
  }
}
