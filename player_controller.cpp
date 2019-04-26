#include "player_controller.h"
#include "math.h"
#include "blend_stack.h"
#include "debug_drawing.h"
#include "profile.h"

const vec3 YAxis = { 0, 1, 0 };
const vec3 ZAxis = { 0, 0, 1 };

transform GetAnimRootMotionDelta(const entity* Entity, Anim::animation* RootMotionAnim,
                                 bool MirrorRootMotionInX, float LocalSampleTime, float dt);

void DrawFrameInfo(mm_frame_info AnimGoal, mat4 CoordinateFrame,
                   mm_info_debug_settings DebugSettings, vec3 BoneColor, vec3 VelocityColor,
                   vec3 TrajectoryColor, vec3 DirectionColor);

void
Gameplay::ResetPlayer(entity* Player, blend_stack* BlendStack,
                      Resource::resource_manager* Resources, const mm_controller_data* MMData)
{
  assert(Player->AnimController);

  Player->AnimController->BlendFunc = NULL;
  for(int i = 0; i < Player->AnimController->AnimStateCount; i++)
  {
    if(Player->AnimController->AnimationIDs[i].Value != 0)
    {

      bool AnimIsUsedByMMData = false;
      for(int j = 0; MMData->FrameInfos.IsValid() && j < MMData->Params.AnimRIDs.Count; j++)
      {
        if(Player->AnimController->AnimationIDs[i].Value == MMData->Params.AnimRIDs[j].Value)
        {
          AnimIsUsedByMMData = true;
          break;
        }
      }
      if(!AnimIsUsedByMMData)
      {
        Resources->Animations.RemoveReference(Player->AnimController->AnimationIDs[i]);
      }
      Player->AnimController->AnimationIDs[i] = {};
      Player->AnimController->States[i]       = {};
      Player->AnimController->Animations[i]   = NULL;
    }
  }
  Player->AnimController->AnimStateCount = 0;
  ResetBlendStack(BlendStack);
}

void
Gameplay::UpdatePlayer(entity* Player, blend_stack* InOutBlendStack,
                       Memory::stack_allocator* TempAlocator, const game_input* Input,
                       const camera* Camera, const mm_controller_data* MMData,
                       const mm_debug_settings* MMDebug, float Speed)
{
  blend_stack& BlendStack = *InOutBlendStack;

  TIMED_BLOCK(UpdatePlayer);
  assert(MMData);
  assert(MMData->FrameInfos.IsValid());
  assert(Player->AnimController);
  assert(0 < MMData->Params.AnimRIDs.Count);

  for(int i = 0; i < MMData->Params.AnimRIDs.Count; i++)
  {
    assert(MMData->Animations[i]);
  }

  // Determine the input direction
  vec3 Dir = {};
  {
    vec3 CameraForward = Camera->Forward;
    vec3 ViewForward   = Math::Normalized(vec3{ CameraForward.X, 0, CameraForward.Z });
    vec3 ViewRight     = Math::Cross(ViewForward, YAxis);

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
  }

  Player->AnimController->BlendFunc = ThirdPersonAnimationBlendFunction;

  if(BlendStack.Empty())
  {
    int   InitialAnimIndex = 0;
    float StartTime        = 0;
    bool  PlayMirrored     = false;

    PlayAnimation(Player->AnimController, &BlendStack, MMData->Params.AnimRIDs[InitialAnimIndex],
                  InitialAnimIndex, StartTime, MMData->Params.DynamicParams.BelndInTime,
                  PlayMirrored);
  }
  else
  {
    mat4 ModelMatrix = TransformToMat4(Player->Transform);

    mm_frame_info AnimGoal = {};
    {
      mat4 InvModelMatrix = Math::InvMat4(ModelMatrix);
      vec3 DesiredModelSpaceVelocity =
        Math::MulMat4Vec4(InvModelMatrix,
                          vec4{ MMData->Params.DynamicParams.TrajectoryTimeHorizon * Dir * Speed,
                                0 })
          .XYZ;

      int32_t CurrentAnimIndex = BlendStack.Peek().AnimStateIndex;
      AnimGoal                 = GetMMGoal(TempAlocator, CurrentAnimIndex, BlendStack.Peek().Mirror,
                           Player->AnimController, DesiredModelSpaceVelocity, MMData->Params);
    }

    DrawFrameInfo(AnimGoal, ModelMatrix, MMDebug->CurrentGoal, { 1, 0, 1 }, { 1, 0, 1 },
                  { 0, 0, 1 }, { 1, 0, 0 });

    static mm_frame_info LastMatch = {};

    DrawFrameInfo(LastMatch, ModelMatrix, MMDebug->MatchedGoal, { 1, 1, 0 }, { 1, 1, 0 },
                  { 0, 1, 0 }, { 1, 0, 0 });

    {
      int32_t NewAnimIndex;
      float   NewAnimStartTime;
      bool    NewAnimIsMirrored = false;

      mm_frame_info BestMatch = {};
      if(!MMData->Params.DynamicParams.MatchMirroredAnimations)
      {
        MotionMatch(&NewAnimIndex, &NewAnimStartTime, &BestMatch, MMData, AnimGoal);
      }
      else
      {
        MotionMatchWithMirrors(&NewAnimIndex, &NewAnimStartTime, &BestMatch, &NewAnimIsMirrored,
                               MMData, AnimGoal);
      }

      const Anim::animation* MatchedAnim = MMData->Animations[NewAnimIndex];

      // Note: this will always run at least one frame after matching the head of the stack so
      // animation pointer should always be present in the AnimController
      int   ActiveStateIndex    = BlendStack.Peek().AnimStateIndex;
      float ActiveAnimLocalTime = Anim::GetLocalSampleTime(Player->AnimController, ActiveStateIndex,
                                                           Player->AnimController->GlobalTimeSec);

      // Figure out if matched frame is sufficiently far away from the current to start a new
      // animation
      if(Player->AnimController->AnimationIDs[ActiveStateIndex].Value !=
           MMData->Params.AnimRIDs[NewAnimIndex].Value ||
         (AbsFloat(ActiveAnimLocalTime - NewAnimStartTime) >=
            MMData->Params.DynamicParams.MinTimeOffsetThreshold &&
          NewAnimIsMirrored == BlendStack.Peek().Mirror) ||
         NewAnimIsMirrored != BlendStack.Peek().Mirror)
      {
        LastMatch = (NewAnimIsMirrored)
                      ? GetMirroredFrameGoal(BestMatch, { -1, 1, 1 }, MMData->Params.FixedParams)
                      : BestMatch;
        PlayAnimation(Player->AnimController, &BlendStack, MMData->Params.AnimRIDs[NewAnimIndex],
                      NewAnimIndex, NewAnimStartTime, MMData->Params.DynamicParams.BelndInTime,
                      NewAnimIsMirrored);
      }
    }

    ThirdPersonBelndFuncStopUnusedAnimations(Player->AnimController, &BlendStack);

    // Root motion
    if(MMDebug->ApplyRootMotion)
    {
      blend_in_info AnimBlend = BlendStack.Peek();

      Anim::animation* RootMotionAnim = MMData->Animations[AnimBlend.IndexInSet];

      float LocalSampleTime =
        Anim::GetLocalSampleTime(RootMotionAnim,
                                 &Player->AnimController->States[AnimBlend.AnimStateIndex],
                                 Player->AnimController->GlobalTimeSec);

      transform RootDelta = GetAnimRootMotionDelta(Player, RootMotionAnim, AnimBlend.Mirror,
                                                   LocalSampleTime, Input->dt);
      Player->Transform.T += RootDelta.T;
      Player->Transform.R = Player->Transform.R * RootDelta.R;
    }
  }
}

transform
GetAnimRootMotionDelta(const entity* Entity, Anim::animation* RootMotionAnim,
                       bool MirrorRootMotionInX, float LocalSampleTime, float dt)
{
  assert(RootMotionAnim);

  const int HipBoneIndex = 0;

  transform RootDelta = IdentityTransform();

  float NextSampleTime = LocalSampleTime + dt;

  transform CurrentHipTransform =
    Anim::LinearAnimationBoneSample(RootMotionAnim, HipBoneIndex, LocalSampleTime);
  transform NextHipTransform =
    Anim::LinearAnimationBoneSample(RootMotionAnim, HipBoneIndex, NextSampleTime);

  mat4 Mat4Entity = TransformToMat4(Entity->Transform);
  Mat4Entity.T    = {};
  {
    // mat4 Mat4CurrentHip = TransformToMat4(CurrentHipTransform);
    mat4 Mat4CurrentHip =
      Math::MulMat4(Entity->AnimController->Skeleton->Bones[HipBoneIndex].BindPose,
                    TransformToMat4(CurrentHipTransform));
    mat4 Mat4CurrentRoot;
    mat4 Mat4InvCurrentRoot;
    Anim::GetRootAndInvRootMatrices(&Mat4CurrentRoot, &Mat4InvCurrentRoot, Mat4CurrentHip);
    {
      // mat4 Mat4NextHip = TransformToMat4(NextHipTransform);
      mat4 Mat4NextHip =
        Math::MulMat4(Entity->AnimController->Skeleton->Bones[HipBoneIndex].BindPose,
                      TransformToMat4(NextHipTransform));
      mat4 Mat4NextRoot;
      Anim::GetRootAndInvRootMatrices(&Mat4NextRoot, NULL, Mat4NextHip);
      {
        mat4 Mat4LocalRootDelta = Math::MulMat4(Mat4InvCurrentRoot, Mat4NextRoot);
        quat dR                 = Math::QuatFromTo(Mat4CurrentRoot.Z, Mat4NextRoot.Z);
        if(MirrorRootMotionInX)
        {
          Mat4LocalRootDelta = Math::MulMat4(Math::Mat4Scale(-1, 1, 1), Mat4LocalRootDelta);
          dR                 = Math::QuatFromTo(Mat4NextRoot.Z, Mat4CurrentRoot.Z);
        }
        mat4 Mat4DeltaRoot = Math::MulMat4(Mat4Entity, Mat4LocalRootDelta);

        vec3 dT     = Mat4DeltaRoot.T;
        RootDelta.T = dT;
        RootDelta.R = dR;
        return RootDelta;
      }
    }
  }
}

void
DrawFrameInfo(mm_frame_info AnimGoal, mat4 CoordinateFrame, mm_info_debug_settings DebugSettings,
              vec3 BoneColor, vec3 VelocityColor, vec3 TrajectoryColor, vec3 DirectionColor)
{
  for(int i = 0; i < MM_COMPARISON_BONE_COUNT; i++)
  {
    vec4 HomogLocalBoneP = { AnimGoal.BonePs[i], 1 };
    vec3 WorldBoneP      = Math::MulMat4Vec4(CoordinateFrame, HomogLocalBoneP).XYZ;
    vec4 HomogLocalBoneV = { AnimGoal.BoneVs[i], 0 };
    vec3 WorldBoneV      = Math::MulMat4Vec4(CoordinateFrame, HomogLocalBoneV).XYZ;
    vec3 WorldVEnd       = WorldBoneP + WorldBoneV;

    if(DebugSettings.ShowBonePositions)
    {
      Debug::PushWireframeSphere(WorldBoneP, 0.02f, { BoneColor, 1 });
    }
    if(DebugSettings.ShowBoneVelocities)
    {
      Debug::PushLine(WorldBoneP, WorldVEnd, { VelocityColor, 1 });
      Debug::PushWireframeSphere(WorldVEnd, 0.01f, { VelocityColor, 1 });
    }
  }
  vec3 PrevWorldTrajectoryPointP = CoordinateFrame.T;
  if(DebugSettings.ShowTrajectory)
  {
    for(int i = 0; i < MM_POINT_COUNT; i++)
    {
      vec4 HomogTrajectoryPointP = { AnimGoal.TrajectoryPs[i], 1 };
      vec3 WorldTrajectoryPointP = Math::MulMat4Vec4(CoordinateFrame, HomogTrajectoryPointP).XYZ;
      {
        Debug::PushLine(PrevWorldTrajectoryPointP, WorldTrajectoryPointP, { TrajectoryColor, 1 });
        Debug::PushWireframeSphere(WorldTrajectoryPointP, 0.02f, { TrajectoryColor, 1 });
        PrevWorldTrajectoryPointP = WorldTrajectoryPointP;
      }
      const float GoalDirectionLength = 0.2f;
      if(DebugSettings.ShowTrajectoryAngles)
      {
        vec4 LocalSpaceFacingDirection = { sinf(AnimGoal.TrajectoryAngles[i]), 0,
                                           cosf(AnimGoal.TrajectoryAngles[i]), 0 };
        vec3 WorldSpaceFacingDirection =
          Math::MulMat4Vec4(CoordinateFrame, LocalSpaceFacingDirection).XYZ;
        Debug::PushLine(WorldTrajectoryPointP,
                        WorldTrajectoryPointP + GoalDirectionLength * WorldSpaceFacingDirection,
                        { DirectionColor, 1 });
      }
    }
  }
}
