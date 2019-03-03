#include "player_controller.h"
#include "math.h"
#include "motion_matching.h"
#include "blend_stack.h"
#include "debug_drawing.h"

static float g_SpeedBlend = 0;

const vec3 YAxis = { 0, 1, 0 };
const vec3 ZAxis = { 0, 0, 1 };

void
Gameplay::ResetPlayer()
{
}

void
Gameplay::UpdatePlayer(entity* Player, Resource::resource_manager* Resources, const game_input* Input, const camera* Camera,
                       const mm_animation_set* MMSet)
{
  vec3 CameraForward = Camera->Forward;
  vec3 ViewForward   = Math::Normalized(vec3{ CameraForward.X, 0, CameraForward.Z });
  vec3 ViewRight     = Math::Cross(ViewForward, YAxis);

  const float Speed = 1.5f;

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

    //Player->Transform.Translation += Input->dt * Speed * Dir;
    //Player->Transform.Rotation = Math::QuatFromTo({ 0, 0, 1 }, Dir);
  }

  if(Player->AnimController && 0 < MMSet->IsBuilt)
  {
		assert(0 < MMSet->AnimRIDs.Count);
    Player->AnimController->BlendFunc = ThirdPersonAnimationBlendFunction;

    mat4 ModelMatrix    = Anim::TransformToMat4(Player->Transform);
    mat4 InvModelMatrix = Math::InvMat4(ModelMatrix);

    mm_frame_info AnimGoal = {};
    {
      vec3 ModelSpaceVelocity =
        Math::MulMat4Vec4(InvModelMatrix, vec4{ MMSet->FormatInfo.TrajectoryTimeHorizon * Dir * Speed, 0 })
          .XYZ;
      AnimGoal = GetCurrentFrameGoal(Player->AnimController, ModelSpaceVelocity, MMSet->FormatInfo);
    }

    //Visualize the current goal
    {
        for(int i = 0; i < MM_COMPARISON_BONE_COUNT; i++)
        {
          vec4 HomogLocalBonePos = { AnimGoal.BonePs[i].X, AnimGoal.BonePs[i].Y,
                                     AnimGoal.BonePs[i].Z, 1 };
          Debug::PushWireframeSphere(Math::MulMat4Vec4(ModelMatrix, HomogLocalBonePos).XYZ, 0.04f,
                                     { 1, 0, 1, 1 });
        }
        for(int i = 0; i < MM_POINT_COUNT; i++)
        {
          vec4 HomogTrajectoryPointPos = { AnimGoal.TrajectoryPs[i].X, AnimGoal.TrajectoryPs[i].Y,
                                           AnimGoal.TrajectoryPs[i].Z, 1 };
          Debug::PushWireframeSphere(Math::MulMat4Vec4(ModelMatrix, HomogTrajectoryPointPos).XYZ,
                                     0.04f, { 0, 0, 1, 1 });
        }
    }

    static mm_frame_info LastMatch = {};
		
    //Visualize the current match goal
    {
        for(int i = 0; i < MM_COMPARISON_BONE_COUNT; i++)
        {
          vec4 HomogLocalBonePos = { LastMatch.BonePs[i].X, LastMatch.BonePs[i].Y,
                                     LastMatch.BonePs[i].Z, 1 };
          Debug::PushWireframeSphere(Math::MulMat4Vec4(ModelMatrix, HomogLocalBonePos).XYZ, 0.05f,
                                     { 1, 1, 0, 1 });
        }
        for(int i = 0; i < MM_POINT_COUNT; i++)
        {
          vec4 HomogTrajectoryPointPos = { LastMatch.TrajectoryPs[i].X, LastMatch.TrajectoryPs[i].Y,
                                           LastMatch.TrajectoryPs[i].Z, 1 };
          Debug::PushWireframeSphere(Math::MulMat4Vec4(ModelMatrix, HomogTrajectoryPointPos).XYZ,
                                     0.05f, { 0, 1, 0, 1 });
        }
    }

    //if(Input->Space.EndedDown)
    {
        // TODO(Lukas) Match animation
        int32_t NewAnimIndex;
        int32_t StartFrameIndex;
        float   BestCost = MotionMatch(&NewAnimIndex, &StartFrameIndex, MMSet, AnimGoal);
        const Anim::animation* MatchedAnim = Resources->GetAnimation(MMSet->AnimRIDs[NewAnimIndex]);
        const float AnimStartTime = (((float)StartFrameIndex) / MatchedAnim->KeyframeCount) *
                                    Anim::GetAnimDuration(MatchedAnim);
        LastMatch = MMSet->FrameInfos[MMSet->AnimFrameRanges[NewAnimIndex].Start + StartFrameIndex];

        PlayAnimation(Player->AnimController, MMSet->AnimRIDs[NewAnimIndex], AnimStartTime, 0.0f);
    }
  }
}
