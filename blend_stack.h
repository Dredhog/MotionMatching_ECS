#pragma once

#include "basic_data_structures.h"
#include "anim.h"
#include "rid.h"

struct blend_in_info
{
  float     GlobalStartTime;
  float     Duration;
  int32_t   AnimStateIndex;
  bool      Mirror;
  transform Transform;
};

typedef circular_stack<blend_in_info, ANIM_CONTROLLER_MAX_ANIM_COUNT> blend_stack;

struct playback_info
{
	blend_stack* BlendStack;
  Anim::skeleton_mirror_info* MirrorInfo;
};

void PlayAnimation(Anim::animation_controller* C, blend_stack* BlendStac, rid NewAnimRID,
                   Anim::animation* NewAnim, float LocalStartTime, float BlendInTime, bool Mirror);
void ResetBlendStack(blend_stack* BlendStack);
void ThirdPersonAnimationBlendFunction(Anim::animation_controller* C, void* UserData);
void ThirdPersonBelndFuncStopUnusedAnimations(Anim::animation_controller* C,
                                              blend_stack*                BlendStack);
