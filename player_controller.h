#pragma once

#include "entity.h"
#include "common.h"
#include "camera.h"
#include "motion_matching.h"
#include "resource_manager.h"
#include "blend_stack.h"

namespace Gameplay
{
  void ResetPlayer(entity* Player, blend_stack* BlendStack, Resource::resource_manager* Resources,
                   const mm_controller_data* MMData);
  void UpdatePlayer(entity* Player, blend_stack* BlendStack, Memory::stack_allocator* TempAlocator,
                    const game_input* Input, const camera* Camera, const mm_controller_data* MMData,
                    const mm_debug_settings* MMDebug, float Speed);
}
//END OF player_conroller.h

#include "player_controller.h"
#include "math.h"
#include "blend_stack.h"
#include "debug_drawing.h"
#include "profile.h"

const vec3 YAxis = { 0, 1, 0 };
const vec3 ZAxis = { 0, 0, 1 };

transform GetAnimRootMotionDelta(Anim::animation*                  RootMotionAnim,
                                 const Anim::animation_controller* C, bool MirrorRootMotionInX,
                                 float LocalSampleTime, float dt);

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

    /*PlayAnimation(Player->AnimController, &BlendStack, MMData->Params.AnimRIDs[InitialAnimIndex],
                  InitialAnimIndex, StartTime, MMData->Params.DynamicParams.BelndInTime,
                  PlayMirrored);*/
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
      AnimGoal                 = {};
      // GetMMGoal(TempAlocator, CurrentAnimIndex, BlendStack.Peek().Mirror, Player->AnimController,
      // DesiredModelSpaceVelocity, MMData->Params.FixedParams);
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
        /*PlayAnimation(Player->AnimController, &BlendStack, MMData->Params.AnimRIDs[NewAnimIndex],
                      NewAnimIndex, NewAnimStartTime, MMData->Params.DynamicParams.BelndInTime,
                      NewAnimIsMirrored);*/
      }
    }

    ThirdPersonBelndFuncStopUnusedAnimations(Player->AnimController, &BlendStack);

    // Root motion
    if(MMDebug->ApplyRootMotion)
    {
      blend_in_info AnimBlend = BlendStack.Peek();

			//Note(Lukas) this is deprecated and just made to compile
      Anim::animation* RootMotionAnim = Player->AnimController->Animations[AnimBlend.AnimStateIndex];

      float LocalSampleTime =
        Anim::GetLocalSampleTime(RootMotionAnim,
                                 &Player->AnimController->States[AnimBlend.AnimStateIndex],
                                 Player->AnimController->GlobalTimeSec);

      /*transform LocalRootDelta =
        GetAnimRootMotionDelta(RootMotionAnim, Player->AnimController, AnimBlend.Mirror,
                               LocalSampleTime, Input->dt);*/
      //TODO(Lukas) The translation has to be rotated by the entity matrix for this to work again
      transform RootDelta = IdentityTransform(); // LocalRootDelta;

      Player->Transform.T += RootDelta.T;
      Player->Transform.R = Player->Transform.R * RootDelta.R;
    }
  }
}

