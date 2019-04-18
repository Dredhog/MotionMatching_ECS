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
  transform PrevRootTransform;
};

extern circular_stack<blend_in_info, ANIM_CONTROLLER_MAX_ANIM_COUNT> g_BlendInfos;

void PlayAnimation(Anim::animation_controller* C, rid NewAnimRID, float LocalStartTime,
                   float BlendInTime, bool Mirror);
void ResetBlendStack();
void ThirdPersonAnimationBlendFunction(Anim::animation_controller* C);
void ThirdPersonBelndFuncStopUnusedAnimations(Anim::animation_controller* C);
