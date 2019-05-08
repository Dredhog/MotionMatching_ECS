#include "blend_stack.h"
#include "misc.h"

void
PlayAnimation(blend_stack* BlendStack, Anim::animation* NewAnim, float LocalAnimTime,
              float GlobalTime, float BlendInTime, bool Mirror, bool Loop)
{
	assert(NewAnim);
  blend_in_info NewBlend = {
    .Animation            = NewAnim,
    .GlobalAnimStartTime  = GlobalTime - LocalAnimTime,
    .GlobalBlendStartTime = GlobalTime,
    .BlendDuration        = BlendInTime,
    .Mirror               = Mirror,
    .Loop                 = Loop,
  };
  BlendStack->Push(NewBlend);
}

// Deferred execution inside of the animation system
void
BlendStackBlendFunc(Anim::animation_controller* C, void* UserData)
{
  playback_info                     PlaybackInfo = *(playback_info*)UserData;
  const Anim::skeleton_mirror_info* MirrorInfo   = PlaybackInfo.MirrorInfo;
  const blend_stack&                BlendStack   = *PlaybackInfo.BlendStack;
  assert(C->AnimStateCount == BlendStack.Count);

	if(C->AnimStateCount > 0)
	{
    Anim::SampleAtGlobalTime(C, 0, 0, C->States[0].Mirror ? MirrorInfo : NULL);
  }
  for(int i = 1; i < C->AnimStateCount; i++)
  {
    Anim::SampleAtGlobalTime(C, i, 1, C->States[i].Mirror ? MirrorInfo : NULL);
    float t = ClampFloat(0,
                         (C->GlobalTimeSec - BlendStack[i].GlobalBlendStartTime) /
                           BlendStack[i].BlendDuration,
                         1);
    Anim::LinearBlend(C, 0, 1, t, 0);
  }
}
