#include "blend_stack.h"
#include "misc.h"

void
ResetBlendStack(blend_stack* BlendStack)
{
  BlendStack->Clear();
}

// TODO(Lukas): Remove assumption that playback rate is 1
void
PlayAnimation(Anim::animation_controller* C, blend_stack* BlendStack, rid NewAnimRID,
              int32_t IndexInSet, float LocalStartTime, float BlendInTime, bool Mirror)
{
  blend_in_info AnimBlend   = {};
  AnimBlend.Duration        = BlendInTime;
  AnimBlend.GlobalStartTime = C->GlobalTimeSec;
  AnimBlend.Mirror       = Mirror;
  AnimBlend.IndexInSet      = IndexInSet;
  AnimBlend.AnimStateIndex  = (BlendStack->Empty()) ? 0
                                                  : (BlendStack->Peek().AnimStateIndex + 1) %
                                                      ANIM_CONTROLLER_MAX_ANIM_COUNT;
  BlendStack->Push(AnimBlend);

  Anim::SetAnimation(C, NewAnimRID, AnimBlend.AnimStateIndex);
  Anim::StartAnimationAtGlobalTime(C, AnimBlend.AnimStateIndex, false, LocalStartTime);

  C->AnimStateCount = BlendStack->GetCapacity();
}

// Deferred execution inside of the animation system
void
ThirdPersonAnimationBlendFunction(Anim::animation_controller* C, void* UserData)
{
	assert(UserData);
  blend_stack& BlendStack = *(blend_stack*)UserData;

  Anim::skeleton_mirror_info TestMirrorInfo = {};

  TestMirrorInfo.MirrorBasisScales = { -1, 1, 1 };
  // Legs/feet
  TestMirrorInfo.BoneMirrorIndices[0] = { 57, 62 };
  TestMirrorInfo.BoneMirrorIndices[1] = { 58, 63 };
  TestMirrorInfo.BoneMirrorIndices[2] = { 59, 64 };
  TestMirrorInfo.BoneMirrorIndices[3] = { 60, 65 };
  TestMirrorInfo.BoneMirrorIndices[4] = { 61, 66 };
  // shoulders/arms
  TestMirrorInfo.BoneMirrorIndices[5] = { 9, 33 };
  TestMirrorInfo.BoneMirrorIndices[6] = { 10, 34 };
  TestMirrorInfo.BoneMirrorIndices[7] = { 11, 35 };
  TestMirrorInfo.BoneMirrorIndices[8] = { 12, 36 };
  TestMirrorInfo.BoneMirrorIndices[9] = { 13, 37 };
  TestMirrorInfo.BoneMirrorIndices[10] = { 14, 38 };
  TestMirrorInfo.BoneMirrorIndices[11] = { 15, 39 };
  TestMirrorInfo.BoneMirrorIndices[12] = { 16, 40 };
  TestMirrorInfo.BoneMirrorIndices[13] = { 17, 41 };
  TestMirrorInfo.BoneMirrorIndices[14] = { 18, 42 };
  TestMirrorInfo.BoneCount = 15;
	for(int i = 0; i <= 5; i++)
	{
    TestMirrorInfo.BoneMirrorIndices[TestMirrorInfo.BoneCount + i] = { i, i };
    TestMirrorInfo.BoneCount++;
  }
  // Spine/Neck

  if(!BlendStack.Empty())
  {
    // In order from oldest to most recent
    if(BlendStack[0].Mirror)
    {
      Anim::SampleAtGlobalTime(C, BlendStack[0].AnimStateIndex, 0, &TestMirrorInfo);
    }
    else
    {
      Anim::SampleAtGlobalTime(C, BlendStack[0].AnimStateIndex, 0);
    }

    for(int i = 1; i < BlendStack.m_Count; i++)
    {
      if(BlendStack[i].Mirror)
      {
        Anim::SampleAtGlobalTime(C, BlendStack[i].AnimStateIndex, 1, &TestMirrorInfo);
      }
      else
      {
        Anim::SampleAtGlobalTime(C, BlendStack[i].AnimStateIndex, 1);
      }

      float UnclampedBlendAmount =
        (C->GlobalTimeSec - BlendStack[i].GlobalStartTime) / BlendStack[i].Duration;
      float t = ClampFloat(0.0f, UnclampedBlendAmount, 1.0f);
      Anim::LinearBlend(C, 0, 1, t, 0);
    }
  }
}

void
ThirdPersonBelndFuncStopUnusedAnimations(Anim::animation_controller* C,
                                         blend_stack*                InOutBlendStack)
{
  blend_stack& BlendStack = *InOutBlendStack;
  for(int i = BlendStack.m_Count - 1; i > 0; i--)
  {
    float UnclampedBlendAmount =
      (C->GlobalTimeSec - BlendStack[i].GlobalStartTime) / BlendStack[i].Duration;
    if(1.0f <= UnclampedBlendAmount)
    {
      for(int a = 0; a < i; a++)
      {
        blend_in_info RemovedInfo                   = BlendStack.PopBack();
        C->AnimationIDs[RemovedInfo.AnimStateIndex] = {};
        C->Animations[RemovedInfo.AnimStateIndex]   = {};
      }
      break;
    }
  }
}
