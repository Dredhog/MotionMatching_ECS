#include "player_controller.h"
#include "math.h"
#include "motion_matching.h"
#include "blend_stack.h"

static float g_SpeedBlend = 0;

const vec3 YAxis = { 0, 1, 0 };
const vec3 ZAxis = { 0, 0, 1 };

void
Gameplay::ResetPlayer()
{
}

void
Gameplay::UpdatePlayer(entity* Player, const game_input* Input, const camera* Camera,
                       const animation_set* MMSet)
{
  vec3 CameraForward = Camera->Forward;
  vec3 ViewForward   = Math::Normalized(vec3{ CameraForward.X, 0, CameraForward.Z });
  vec3 ViewRight     = Math::Cross(ViewForward, YAxis);

  const float Speed = 3.0f;

  vec3 Dir = {};
  if(Input->ArrowUp.EndedDown)
  {
    Dir += ViewForward;
  }
  if(Input->ArrowDown.EndedDown)
  {
    Dir -= ViewForward;
  }
  if(Input->ArrowRight.EndedDown)
  {
    Dir += ViewRight;
  }
  if(Input->ArrowLeft.EndedDown)
  {
    Dir -= ViewRight;
  }
  if(Math::Length(Dir) > 0.5f)
  {
    Dir = Math::Normalized(Dir);

    Player->Transform.Translation += Input->dt * Speed * Dir;
    Player->Transform.Rotation = Math::QuatFromTo({0, 0, 1}, Dir);
  }

	if(Player->AnimController)
	{
		Player->AnimController->BlendFunc = ThirdPersonAnimationBlendFunction;
    //TODO(Lukas) Fill goal struct
    /*animation_goal AnimGoal = {};
    {
      GetCurrentFrameGoal();
    }

    //TODO(Lukas) Match animation
    rid NewAnim, float StartTime = MotionMatch(MMSet, AnimGoal, CurrentAnim, CurrentAnimThresh);
    */
		if(0 < MMSet->AnimRIDs.Count)
		{
      static int CurrentAnimIndex = 0;
      if(Input->n.EndedDown && Input->n.Changed)
      {
				CurrentAnimIndex--;
      }
      if(Input->m.EndedDown && Input->m.Changed)
      {
				CurrentAnimIndex++;
      }
      CurrentAnimIndex = (CurrentAnimIndex + MMSet->AnimRIDs.Count) % MMSet->AnimRIDs.Count;

      if(Input->Space.EndedDown && Input->Space.Changed)
      {
        PlayAnimation(Player->AnimController, MMSet->AnimRIDs[CurrentAnimIndex], 0.0, 1.0f);
      }
    }
  }
}
