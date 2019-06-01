#pragma once

#include "basic_data_structures.h"
#include "anim.h"

struct blend_in_info
{
  // Refreshed at frame start
  Anim::animation* Animation;

  // Stored Permanently
  int32_t IndexInSet;
  float   GlobalAnimStartTime;
  float   GlobalBlendStartTime;
  float   BlendDuration;
  bool    Mirror;
  bool    Loop;
};

typedef circular_stack<blend_in_info, ANIM_PLAYER_MAX_ANIM_COUNT> blend_stack;

struct playback_info
{
  const Anim::skeleton_mirror_info* MirrorInfo;
  const blend_stack*                BlendStack;
};

void PlayAnimation(blend_stack* BlendStack, Anim::animation* NewAnim, int32_t IndexInSet,
                   float LocalStartTime, float GlobalTime, float BlendInTime, bool Mirror,
                   bool Loop = false);
void BlendStackBlendFunc(Anim::animation_player* P, void* UserData);
