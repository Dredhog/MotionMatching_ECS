#include "blend_stack.h"
#include "misc.h"

void
ResetBlendStack(blend_stack* BlendStack)
{
  BlendStack->Clear();
}

void
PlayAnimation(Anim::animation_controller* C, blend_stack* BlendStack, rid NewAnimRID,
              Anim::animation* NewAnim, float LocalStartTime, float BlendInTime, bool Mirror)
{
  blend_in_info AnimBlend   = {};
  AnimBlend.Duration        = BlendInTime;
  AnimBlend.GlobalStartTime = C->GlobalTimeSec;
  AnimBlend.Mirror          = Mirror;
  AnimBlend.AnimStateIndex  = (BlendStack->Empty()) ? 0
                                                   : (BlendStack->Peek().AnimStateIndex + 1) %
                                                       ANIM_CONTROLLER_MAX_ANIM_COUNT;
  BlendStack->Push(AnimBlend);

  Anim::SetAnimation(C, NewAnimRID, AnimBlend.AnimStateIndex);
  Anim::StartAnimationAtGlobalTime(C, AnimBlend.AnimStateIndex, false, LocalStartTime);
  C->Animations[AnimBlend.AnimStateIndex] = NewAnim;

  C->AnimStateCount = BlendStack->GetCapacity();
}

// Deferred execution inside of the animation system
void
ThirdPersonAnimationBlendFunction(Anim::animation_controller* C, void* UserData)
{
	assert(UserData);
  playback_info     PlaybackInfo = *(playback_info*)UserData;
  const blend_stack& BlendStack = *PlaybackInfo.BlendStack;
  const Anim::skeleton_mirror_info& MirrorInfo   = *PlaybackInfo.MirrorInfo;

  if(!BlendStack.Empty())
  {
    // In order from oldest to most recent
    if(BlendStack[0].Mirror)
    {
      Anim::SampleAtGlobalTime(C, BlendStack[0].AnimStateIndex, 0, &MirrorInfo);
    }
    else
    {
      Anim::SampleAtGlobalTime(C, BlendStack[0].AnimStateIndex, 0);
    }

    for(int i = 1; i < BlendStack.m_Count; i++)
    {
      if(BlendStack[i].Mirror)
      {
        Anim::SampleAtGlobalTime(C, BlendStack[i].AnimStateIndex, 1, &MirrorInfo);
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
