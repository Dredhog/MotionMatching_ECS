#pragma once

#include "basic_data_structures.h"
#include "anim.h"
#include "rid.h"

struct blend_in_info
{
  float   GlobalStartTime;
  float   Duration;
  int32_t AnimStateIndex;
};

circular_stack<blend_in_info, ANIM_CONTROLLER_MAX_ANIM_COUNT> g_BlendInfos;
int                                                           g_CurrentAnimStateIndex = 0;

void
PlayAnimation(Anim::animation_controller* C, rid NewAnimRID, float LocalStartTime,
              float BlendInTime)
{
  blend_in_info AnimBlend   = {};
  AnimBlend.Duration        = BlendInTime;
  AnimBlend.GlobalStartTime = C->GlobalTimeSec;
  AnimBlend.AnimStateIndex  = g_CurrentAnimStateIndex;
	g_BlendInfos.Push(AnimBlend);

  Anim::SetAnimation(C, NewAnimRID, g_CurrentAnimStateIndex);
  Anim::StartAnimationAtGlobalTime(C, g_CurrentAnimStateIndex, true, LocalStartTime);

  C->AnimStateCount = g_BlendInfos.GetCount();

  g_CurrentAnimStateIndex = (g_CurrentAnimStateIndex + 1) % g_BlendInfos.GetCapacity();
}

// Deferred execution inside of the animation system
void
ThirdPersonAnimationBlendFunction(Anim::animation_controller* C)
{
	if(0 < g_BlendInfos.m_Count)
	{
    // In order from oldest to most recent
    Anim::SampleAtGlobalTime(C, g_BlendInfos[0].AnimStateIndex, 0);

    for(int i = 1; i < g_BlendInfos.m_Count; i++)
    {
      Anim::SampleAtGlobalTime(C, g_BlendInfos[i].AnimStateIndex, 1);
      float t =
        ClampFloat(0.0f,
                   (C->GlobalTimeSec - g_BlendInfos[i].GlobalStartTime) / g_BlendInfos[i].Duration,
                   1.0f);
      Anim::LinearBlend(C, 0, 1, t, 0);
    }
  }
}
