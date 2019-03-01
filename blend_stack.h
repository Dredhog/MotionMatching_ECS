struct blend_state
{
	int32_t CurrentIndex
}

float PlayAnimation(rid NewAnimRID, int32_t StartFrame, float FadeInTime, bool Looped = true)
{
}

// Defered execution inside of the animation system
void
ThirdPersonAnimationBlendFunction(Anim::animation_controller* C)
{
	//In order from oldest to most recent
	Anim::SampleAtGlobalTime(C, 0, 0);

	for(int i = 1; i < CurrentAnimCount; i++)
	{
		Anim::SampleAtGlobalTime(C, i, 1);
		Anim::LinearBlend(C, 0, 1, FadeFraction[i], 0);
	}

  /*Anim::SetPlaybackRate(C, 0, g_MovePlaybackRate);
  Anim::SetPlaybackRate(C, 1, g_MovePlaybackRate);
  Anim::SampleAtGlobalTime(C, 0, 0);           // Sample Walk
  Anim::SampleAtGlobalTime(C, 1, 1);           // Sample Run
  Anim::LinearBlend(C, 0, 1, g_SpeedBlend, 1); // LERP(Walk, Run) => move
  Anim::SampleAtGlobalTime(C, 2, 0);           // Sample Idle
  Anim::LinearBlend(C, 0, 1, g_SpeedBlend, 0); // LERP(move, Idle) => ground*/
}
