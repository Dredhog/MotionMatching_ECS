#include "player_controller.h"
#include "math.h"
#include "basic_data_structures.h"
#include "motion_matching.h"

static float g_SpeedBlend = 0;

const vec3 YAxis = { 0, 1, 0 };
const vec3 ZAxis = { 0, 0, 1 };

void
Gameplay::ResetPlayer()
{
}

void
Gameplay::UpdatePlayer(entity* Player, const game_input* Input, const camera* Camera)
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

	//Fill goal struct
	/*animation_goal AnimGoal = {};
	{
		GetCurrentFrameGoal();
	}

	//Match animation
  rid NewAnim, int32_t StartFrame = MotionMatch(MMSet, AnimGoal, CurrentAnim, CurrentAnimThresh);

	if(NewAnimInd != -1)
	{
		PlayAnimation(NewAnim, 0.1f);
	}*/
}
