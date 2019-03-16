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
Gameplay::UpdatePlayer(entity* Player, Resource::resource_manager* Resources,
                       const game_input* Input, const camera* Camera, const mm_animation_set* MMSet,
                       float Speed)
{
  vec3 CameraForward = Camera->Forward;
  vec3 ViewForward   = Math::Normalized(vec3{ CameraForward.X, 0, CameraForward.Z });
  vec3 ViewRight     = Math::Cross(ViewForward, YAxis);

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

    // if(CurrentBlendInfo.AnimationIndex
    mm_frame_info AnimGoal = {};
    {
      vec3 ModelSpaceVelocity =
        Math::MulMat4Vec4(InvModelMatrix,
                          vec4{ MMSet->FormatInfo.TrajectoryTimeHorizon * Dir * Speed, 0 })
          .XYZ;
      AnimGoal = GetCurrentFrameGoal(Player->AnimController, ModelSpaceVelocity, MMSet->FormatInfo);
    }

    // Visualize the current goal
    {
      for(int i = 0; i < MM_COMPARISON_BONE_COUNT; i++)
      {
        vec4 HomogLocalBonePos = { AnimGoal.BonePs[i], 1 };
        Debug::PushWireframeSphere(Math::MulMat4Vec4(ModelMatrix, HomogLocalBonePos).XYZ, 0.02f,
                                   { 1, 0, 1, 1 });
      }
      vec3 PrevWorldTrajectoryPointP = ModelMatrix.T;
      for(int i = 0; i < MM_POINT_COUNT; i++)
      {
        vec4 HomogTrajectoryPointP = { AnimGoal.TrajectoryPs[i], 1 };
        vec3 WorldTrajectoryPointP = Math::MulMat4Vec4(ModelMatrix, HomogTrajectoryPointP).XYZ;
        Debug::PushLine(PrevWorldTrajectoryPointP, WorldTrajectoryPointP, { 0, 0, 1, 1 });
        Debug::PushWireframeSphere(WorldTrajectoryPointP, 0.02f, { 0, 0, 1, 1 });
        PrevWorldTrajectoryPointP = WorldTrajectoryPointP;
      }
    }

    static mm_frame_info LastMatch = {};

    // Visualize the most recent match
    {
      for(int i = 0; i < MM_COMPARISON_BONE_COUNT; i++)
      {
        vec4 HomogLocalBoneP = { LastMatch.BonePs[i], 1 };
        vec3 WorldBoneP      = Math::MulMat4Vec4(ModelMatrix, HomogLocalBoneP).XYZ;
        vec4 HomogLocalBoneV = { LastMatch.BonePs[i], 0 };
        vec3 WorldBoneV      = Math::MulMat4Vec4(ModelMatrix, HomogLocalBoneV).XYZ;
        vec3 WorldVEnd       = WorldBoneP + WorldBoneV;

        /*Debug::PushWireframeSphere(WorldBoneP, 0.01f, { 1, 1, 0, 1 });
        Debug::PushLine(WorldBoneP, WorldVEnd, { 1, 1, 0, 1 });
        Debug::PushWireframeSphere(WorldVEnd, 0.01f, { 1, 1, 0, 1 });*/
      }
      vec3 PrevWorldTrajectoryPointP = ModelMatrix.T;
      for(int i = 0; i < MM_POINT_COUNT; i++)
      {
        vec4 HomogTrajectoryPointP = { LastMatch.TrajectoryPs[i], 1 };
        vec3 WorldTrajectoryPointP = Math::MulMat4Vec4(ModelMatrix, HomogTrajectoryPointP).XYZ;
        Debug::PushLine(PrevWorldTrajectoryPointP, WorldTrajectoryPointP, { 0, 1, 0, 1 });
        Debug::PushWireframeSphere(WorldTrajectoryPointP, 0.02f, { 0, 1, 0, 1 });
        PrevWorldTrajectoryPointP = WorldTrajectoryPointP;
      }
    }

    if(Input->MouseLeft.EndedDown)
    {
      int32_t NewAnimIndex;
      int32_t StartFrameIndex;
      float   BestCost = MotionMatch(&NewAnimIndex, &StartFrameIndex, MMSet, AnimGoal);
      const Anim::animation* MatchedAnim = Resources->GetAnimation(MMSet->AnimRIDs[NewAnimIndex]);
      const float AnimStartTime          = (((float)StartFrameIndex) / MatchedAnim->KeyframeCount) *
                                  Anim::GetAnimDuration(MatchedAnim);
      LastMatch = MMSet->FrameInfos[MMSet->AnimFrameRanges[NewAnimIndex].Start + StartFrameIndex];

      PlayAnimation(Player->AnimController, MMSet->AnimRIDs[NewAnimIndex], AnimStartTime,
                    MMSet->FormatInfo.BelndInTime);
    }

    ThirdPersonBelndFuncStopUnusedAnimations(Player->AnimController);

    // Root motion
    if(0 < g_BlendInfos.m_Count)
    {
      int              AnimationIndex = g_BlendInfos.PeekBack().AnimStateIndex;
      Anim::animation* RootMotionAnim =
        Resources->GetAnimation(Player->AnimController->AnimationIDs[AnimationIndex]);
			//TODO(Lukas): there should not be any fetching here, the data is known in advance
      Player->AnimController->Animations[AnimationIndex] = RootMotionAnim;

      float CurrentSampleTime = Anim::GetLoopedSampleTime(Player->AnimController, AnimationIndex,
                                                          Player->AnimController->GlobalTimeSec);
      float NextSampleTime =
        ClampFloat(RootMotionAnim->SampleTimes[0], CurrentSampleTime + Input->dt,
                   RootMotionAnim->SampleTimes[RootMotionAnim->KeyframeCount - 1]);

      int             HipBoneIndex = 0;
      Anim::transform CurrentHipTransform =
        Anim::LinearAnimationBoneSample(RootMotionAnim, HipBoneIndex, CurrentSampleTime);
      Anim::transform NextHipTransform =
        Anim::LinearAnimationBoneSample(RootMotionAnim, HipBoneIndex, NextSampleTime);

      mat4 Mat4Player = Anim::TransformToMat4(Player->Transform);
      Mat4Player.T    = {};
      {
        mat4 Mat4CurrentHip = Anim::TransformToMat4(CurrentHipTransform);
        mat4 Mat4CurrentRoot;
        mat4 Mat4InvCurrentRoot;
        Anim::GetRootAndInvRootMatrices(&Mat4CurrentRoot, &Mat4InvCurrentRoot, Mat4CurrentHip);
        {
          mat4 Mat4NextHip = Anim::TransformToMat4(NextHipTransform);
          mat4 Mat4NextRoot;
          Anim::GetRootAndInvRootMatrices(&Mat4NextRoot, NULL, Mat4NextHip);
          {
            // TODO(Lukas): Multiply by mat4InvPlayer on the left for more generality
            mat4 Mat4DeltaRoot =
              Math::MulMat4(Mat4Player, Math::MulMat4(Mat4InvCurrentRoot, Mat4NextRoot));
            vec3 DeltaTranslaton = Mat4DeltaRoot.T;
            quat DeltaRotation   = Math::QuatFromTo(Mat4CurrentRoot.Z, Mat4NextRoot.Z);
            Player->Transform.Translation += DeltaTranslaton;
            Player->Transform.Rotation = Player->Transform.Rotation * DeltaRotation;
            Debug::PushLine(Player->Transform.Translation,
                            Player->Transform.Translation + DeltaTranslaton, { 1, 0, 0, 1 });
          }
        }
      }
    }
  }
}
