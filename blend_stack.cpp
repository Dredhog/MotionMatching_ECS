#include "blend_stack.h"
#include "misc.h"

circular_stack<blend_in_info, ANIM_CONTROLLER_MAX_ANIM_COUNT> g_BlendInfos = {};

void ResetBlendStack()
{
  g_BlendInfos.Clear();
}

// TODO(Lukas): Remove assumption that playback rate is 1
void
PlayAnimation(Anim::animation_controller* C, rid NewAnimRID, float LocalStartTime,
              float BlendInTime, bool Mirror)
{
  blend_in_info AnimBlend   = {};
  AnimBlend.Duration        = BlendInTime;
  AnimBlend.GlobalStartTime = C->GlobalTimeSec;
  AnimBlend.Mirror       = Mirror;
  AnimBlend.AnimStateIndex  = (g_BlendInfos.Empty()) ? 0
                                                    : (g_BlendInfos.Peek().AnimStateIndex + 1) %
                                                        ANIM_CONTROLLER_MAX_ANIM_COUNT;
  g_BlendInfos.Push(AnimBlend);

  Anim::SetAnimation(C, NewAnimRID, AnimBlend.AnimStateIndex);
  Anim::StartAnimationAtGlobalTime(C, AnimBlend.AnimStateIndex, false, LocalStartTime);

  C->AnimStateCount = g_BlendInfos.GetCapacity();
}

// Deferred execution inside of the animation system
void
ThirdPersonAnimationBlendFunction(Anim::animation_controller* C)
{
  Anim::skeleton_mirror_info TestMirrorInfo = {};

  TestMirrorInfo.MirrorBasisScales    = { -1, 1, 1 };
  TestMirrorInfo.BoneMirrorIndices[0] = { 0, 0 };
	//Legs/feet
  TestMirrorInfo.BoneMirrorIndices[1] = { 2, 8 };
  TestMirrorInfo.BoneMirrorIndices[2] = { 3, 9 };
  TestMirrorInfo.BoneMirrorIndices[3] = { 4, 10 };
	//shoulders/arms
  TestMirrorInfo.BoneMirrorIndices[4] = { 20, 29 };
  TestMirrorInfo.BoneMirrorIndices[5] = { 21, 30 };
  TestMirrorInfo.BoneMirrorIndices[6] = { 22, 31 };
	//Spine/Neck
  TestMirrorInfo.BoneMirrorIndices[7] = { 13, 13 };
  TestMirrorInfo.BoneMirrorIndices[8] = { 14, 14 };
  TestMirrorInfo.BoneMirrorIndices[9] = { 15, 15 };
  TestMirrorInfo.BoneMirrorIndices[10] = { 16, 16 };
  TestMirrorInfo.BoneCount             = 11;

  if(!g_BlendInfos.Empty())
  {
    // In order from oldest to most recent
    if(g_BlendInfos[0].Mirror)
    {
      Anim::SampleAtGlobalTime(C, g_BlendInfos[0].AnimStateIndex, 0, &TestMirrorInfo);
    }
    else
    {
      Anim::SampleAtGlobalTime(C, g_BlendInfos[0].AnimStateIndex, 0);
    }

    for(int i = 1; i < g_BlendInfos.m_Count; i++)
    {
      if(g_BlendInfos[i].Mirror)
      {
        Anim::SampleAtGlobalTime(C, g_BlendInfos[i].AnimStateIndex, 1, &TestMirrorInfo);
      }
      else
      {
        Anim::SampleAtGlobalTime(C, g_BlendInfos[i].AnimStateIndex, 1);
      }

      float UnclampedBlendAmount =
        (C->GlobalTimeSec - g_BlendInfos[i].GlobalStartTime) / g_BlendInfos[i].Duration;
      float t = ClampFloat(0.0f, UnclampedBlendAmount, 1.0f);
      Anim::LinearBlend(C, 0, 1, t, 0);
    }
  }
}

void
ThirdPersonBelndFuncStopUnusedAnimations(Anim::animation_controller* C)
{
  for(int i = g_BlendInfos.m_Count - 1; i > 0; i--)
  {
    float UnclampedBlendAmount =
      (C->GlobalTimeSec - g_BlendInfos[i].GlobalStartTime) / g_BlendInfos[i].Duration;
    if(1.0f <= UnclampedBlendAmount)
    {
      for(int a = 0; a < i; a++)
      {
        blend_in_info RemovedInfo                   = g_BlendInfos.PopBack();
        C->AnimationIDs[RemovedInfo.AnimStateIndex] = {};
        C->Animations[RemovedInfo.AnimStateIndex]   = {};
      }
      break;
    }
  }
}
