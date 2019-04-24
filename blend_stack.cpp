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

  for(int i = 0; i < C->Skeleton->BoneCount; i++)
  {
    //TestMirrorInfo.BoneMirrorIndices[i] = { i, i };
    /*int MirrorIndex = -1;
    for(int j = 0; j < C->Skeleton->BoneCount; j++)
    {
      if();
    }
    if(MirrorIndex == -1)
    {
      TestMirrorInfo
    }*/
  }
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
