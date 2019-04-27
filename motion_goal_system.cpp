#include "motion_goal_system.h"
#include "debug_drawing.h"

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

transform
GetAnimRootMotionDelta(Anim::animation* RootMotionAnim, const Anim::animation_controller* C,
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

  {
    // mat4 Mat4CurrentHip = TransformToMat4(CurrentHipTransform);
    mat4 Mat4CurrentHip = Math::MulMat4(C->Skeleton->Bones[HipBoneIndex].BindPose,
                                        TransformToMat4(CurrentHipTransform));
    mat4 Mat4CurrentRoot;
    mat4 Mat4InvCurrentRoot;
    Anim::GetRootAndInvRootMatrices(&Mat4CurrentRoot, &Mat4InvCurrentRoot, Mat4CurrentHip);
    {
      // mat4 Mat4NextHip = TransformToMat4(NextHipTransform);
      mat4 Mat4NextHip =
        Math::MulMat4(C->Skeleton->Bones[HipBoneIndex].BindPose, TransformToMat4(NextHipTransform));
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
        // TODO(Lukas) Remember to apply the entity's rotation to the delta
        // Mat4Entity.T    = {};
        // mat4 Mat4DeltaRoot = Math::MulMat4(Mat4Entity, Mat4LocalRootDelta);

        vec3 dT     = Mat4LocalRootDelta.T;
        RootDelta.T = dT;
        RootDelta.R = dR;
        return RootDelta;
      }
    }
  }
}
