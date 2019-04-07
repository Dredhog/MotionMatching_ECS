#include "player_controller.h"
#include "math.h"
#include "motion_matching.h"
#include "blend_stack.h"
#include "debug_drawing.h"
#include "profile.h"

const vec3 YAxis = { 0, 1, 0 };
const vec3 ZAxis = { 0, 0, 1 };

void
Gameplay::ResetPlayer(entity* Player)
{
  if(Player->AnimController)
  {
    Player->AnimController->BlendFunc = NULL;
  }
  ResetBlendStack();
}

void
Gameplay::UpdatePlayer(entity* Player, Memory::stack_allocator* TempAlocator,
                       Resource::resource_manager* Resources, const game_input* Input,
                       const camera* Camera, const mm_controller_data* MMData,
                       const mm_debug_settings* MMDebug, float Speed)
{
  TIMED_BLOCK(UpdatePlayer);
  assert(Player->AnimController);
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
  }

  if(MMData->FrameInfos.IsValid())
  {
    assert(0 < MMData->Params.AnimRIDs.Count);
    Player->AnimController->BlendFunc = ThirdPersonAnimationBlendFunction;

    if(g_BlendInfos.Empty())
    {
      int   InitialAnimIndex = 0;
      float StartTime        = 0;
      bool  PlayMirrored     = false;
      Player->AnimController->Animations[InitialAnimIndex] =
        Resources->GetAnimation(MMData->Params.AnimRIDs[InitialAnimIndex]);
      PlayAnimation(Player->AnimController, MMData->Params.AnimRIDs[InitialAnimIndex], StartTime,
                    MMData->Params.DynamicParams.BelndInTime, PlayMirrored);
    }

    mat4 ModelMatrix    = Anim::TransformToMat4(Player->Transform);
    mat4 InvModelMatrix = Math::InvMat4(ModelMatrix);

    mm_frame_info AnimGoal = {};
    {
      vec3 DesiredModelSpaceVelocity =
        Math::MulMat4Vec4(InvModelMatrix,
                          vec4{ MMData->Params.DynamicParams.TrajectoryTimeHorizon * Dir * Speed,
                                0 })
          .XYZ;
      int32_t CurrentAnimIndex = g_BlendInfos.Peek().AnimStateIndex;
      AnimGoal =
        GetCurrentFrameGoal(TempAlocator, CurrentAnimIndex, g_BlendInfos.Peek().Mirror,
                            Player->AnimController, DesiredModelSpaceVelocity, MMData->Params);
    }

    // Visualize the current goal
    {
      for(int i = 0; i < MM_COMPARISON_BONE_COUNT; i++)
      {
        vec4 HomogLocalBoneP = { AnimGoal.BonePs[i], 1 };
        vec3 WorldBoneP      = Math::MulMat4Vec4(ModelMatrix, HomogLocalBoneP).XYZ;
        vec4 HomogLocalBoneV = { AnimGoal.BoneVs[i], 0 };
        vec3 WorldBoneV      = Math::MulMat4Vec4(ModelMatrix, HomogLocalBoneV).XYZ;
        vec3 WorldVEnd       = WorldBoneP + WorldBoneV;

        if(MMDebug->CurrentGoal.ShowBonePositions)
        {
          Debug::PushWireframeSphere(WorldBoneP, 0.02f, { 1, 0, 1, 1 });
        }
        if(MMDebug->CurrentGoal.ShowBoneVelocities)
        {
          Debug::PushLine(WorldBoneP, WorldVEnd, { 1, 0, 1, 1 });
          Debug::PushWireframeSphere(WorldVEnd, 0.01f, { 1, 0, 1, 1 });
        }
      }
      vec3 PrevWorldTrajectoryPointP = ModelMatrix.T;
      if(MMDebug->CurrentGoal.ShowTrajectory)
      {
        for(int i = 0; i < MM_POINT_COUNT; i++)
        {
          vec4 HomogTrajectoryPointP = { AnimGoal.TrajectoryPs[i], 1 };
          vec3 WorldTrajectoryPointP = Math::MulMat4Vec4(ModelMatrix, HomogTrajectoryPointP).XYZ;
          Debug::PushLine(PrevWorldTrajectoryPointP, WorldTrajectoryPointP, { 0, 0, 1, 1 });
          Debug::PushWireframeSphere(WorldTrajectoryPointP, 0.02f, { 0, 0, 1, 1 });
          PrevWorldTrajectoryPointP = WorldTrajectoryPointP;
        }
      }
    }

    static mm_frame_info LastMatch = {};

    // Visualize the most recent match
    {
      for(int i = 0; i < MM_COMPARISON_BONE_COUNT; i++)
      {
        vec4 HomogLocalBoneP = { LastMatch.BonePs[i], 1 };
        vec3 WorldBoneP      = Math::MulMat4Vec4(ModelMatrix, HomogLocalBoneP).XYZ;
        vec4 HomogLocalBoneV = { LastMatch.BoneVs[i], 0 };
        vec3 WorldBoneV      = Math::MulMat4Vec4(ModelMatrix, HomogLocalBoneV).XYZ;
        vec3 WorldVEnd       = WorldBoneP + WorldBoneV;

        if(MMDebug->MatchedGoal.ShowBonePositions)
        {
          Debug::PushWireframeSphere(WorldBoneP, 0.01f, { 1, 1, 0, 1 });
        }
        if(MMDebug->MatchedGoal.ShowBoneVelocities)
        {
          Debug::PushLine(WorldBoneP, WorldVEnd, { 1, 1, 0, 1 });
          Debug::PushWireframeSphere(WorldVEnd, 0.01f, { 1, 1, 0, 1 });
        }
      }
      vec3 PrevWorldTrajectoryPointP = ModelMatrix.T;
      if(MMDebug->MatchedGoal.ShowTrajectory)
      {
        for(int i = 0; i < MM_POINT_COUNT; i++)
        {
          vec4 HomogTrajectoryPointP = { LastMatch.TrajectoryPs[i], 1 };
          vec3 WorldTrajectoryPointP = Math::MulMat4Vec4(ModelMatrix, HomogTrajectoryPointP).XYZ;
          Debug::PushLine(PrevWorldTrajectoryPointP, WorldTrajectoryPointP, { 0, 1, 0, 1 });
          Debug::PushWireframeSphere(WorldTrajectoryPointP, 0.02f, { 0, 1, 0, 1 });
          PrevWorldTrajectoryPointP = WorldTrajectoryPointP;
        }
      }
    }

    {
      int32_t NewAnimIndex;
      float   NewAnimStartTime;
      bool    NewAnimIsMirrored = false;

      mm_frame_info BestMatch         = {};
      if(!MMData->Params.DynamicParams.MatchMirroredAnimations)
      {
        MotionMatch(&NewAnimIndex, &NewAnimStartTime, &BestMatch, MMData, AnimGoal);
      }
      else
      {
        MotionMatchWithMirrors(&NewAnimIndex, &NewAnimStartTime, &BestMatch, &NewAnimIsMirrored,
                               MMData, AnimGoal);
      }

      // TODO(Lukas) Prefetch and store all used animations in an array at build time
      const Anim::animation* MatchedAnim =
        Resources->GetAnimation(MMData->Params.AnimRIDs[NewAnimIndex]);

      int   ActiveStateIndex = (g_BlendInfos.Empty()) ? -1 : g_BlendInfos.Peek().AnimStateIndex;
      float ActiveAnimLocalTime =
        (ActiveStateIndex != -1)
          ? Anim::GetLocalSampleTime(Player->AnimController, ActiveStateIndex,
                                     Player->AnimController->GlobalTimeSec)
          : 0;

      // Figure out if matched frame is sufficiently far away from the current to start a new
      // animation
      if(Player->AnimController->AnimationIDs[ActiveStateIndex].Value !=
           MMData->Params.AnimRIDs[NewAnimIndex].Value ||
         (AbsFloat(ActiveAnimLocalTime - NewAnimStartTime) >=
            MMData->Params.DynamicParams.MinTimeOffsetThreshold &&
          NewAnimIsMirrored == g_BlendInfos.Peek().Mirror) ||
         NewAnimIsMirrored != g_BlendInfos.Peek().Mirror)
      {
        LastMatch = (NewAnimIsMirrored)
                      ? GetMirroredFrameGoal(BestMatch, { -1, 1, 1 }, MMData->Params.FixedParams)
                      : BestMatch;
        PlayAnimation(Player->AnimController, MMData->Params.AnimRIDs[NewAnimIndex],
                      NewAnimStartTime, MMData->Params.DynamicParams.BelndInTime,
                      NewAnimIsMirrored);
      }
    }

    ThirdPersonBelndFuncStopUnusedAnimations(Player->AnimController);

    // Root motion
    if(0 < g_BlendInfos.m_Count)
    {
      int              AnimationIndex   = g_BlendInfos.Peek().AnimStateIndex;
      bool             MirrorRootMotion = g_BlendInfos.Peek().Mirror;
      Anim::animation* RootMotionAnim =
        Resources->GetAnimation(Player->AnimController->AnimationIDs[AnimationIndex]);
      // TODO(Lukas): there should not be any fetching from RIDs here, the animations are known in
      // advance
      Player->AnimController->Animations[AnimationIndex] = RootMotionAnim;

      float CurrentSampleTime = Anim::GetLocalSampleTime(Player->AnimController, AnimationIndex,
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
            /*mat4 Mat4DeltaRoot =
              Math::MulMat4(Mat4Player, Math::MulMat4(Mat4InvCurrentRoot, Mat4NextRoot));
            vec3 DeltaTranslaton = Mat4DeltaRoot.T;*/
            mat4 Mat4LocalRootDelta = Math::MulMat4(Mat4InvCurrentRoot, Mat4NextRoot);
            quat DeltaRotation      = Math::QuatFromTo(Mat4CurrentRoot.Z, Mat4NextRoot.Z);
            if(MirrorRootMotion)
            {
              Mat4LocalRootDelta = Math::MulMat4(Math::Mat4Scale(-1, 1, 1), Mat4LocalRootDelta);
              DeltaRotation      = Math::QuatFromTo(Mat4NextRoot.Z, Mat4CurrentRoot.Z);
            }
            mat4 Mat4DeltaRoot = Math::MulMat4(Mat4Player, Mat4LocalRootDelta);

            vec3 DeltaTranslaton = Mat4DeltaRoot.T;
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
