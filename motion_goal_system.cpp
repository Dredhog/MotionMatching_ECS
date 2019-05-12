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

void
DrawTrajectory(mat4 CoordinateFrame, const trajectory* Trajectory, vec3 PastColor,
               vec3 PresentColor, vec3 FutureColor)
{
  for(int i = 0; i < TRAJECTORY_TRANSFORM_COUNT; i++)
  {
#if 0
    vec3 CurrentWorldPos = Math::MulMat4Vec4(CoordinateFrame, { Trajectory->Transforms[i].T.X, 0,
                                                                Trajectory->Transforms[i].T.Y, 1 })
                             .XYZ;
#endif
#if 0
    vec3 CurrentWorldPos =
      CoordinateFrame.T + vec3{ Trajectory->Transforms[i].T.X, 0, Trajectory->Transforms[i].T.Y };
#endif
#if 1
    vec3 CurrentWorldPos = vec3{ Trajectory->Transforms[i].T.X, 0, Trajectory->Transforms[i].T.Y };
#endif
    vec3 Forward = { 0, 0, 1 };
    vec3 FacingDir = Math::MulMat3Vec3(Math::QuatToMat3(Trajectory->Transforms[i].R), Forward);

    vec3 PointColor = (i < HALF_TRAJECTORY_TRANSFORM_COUNT)
                        ? PastColor
                        : ((i == HALF_TRAJECTORY_TRANSFORM_COUNT) ? PresentColor : FutureColor);
    Debug::PushWireframeSphere(CurrentWorldPos, 0.005f, { PointColor, 0.5 });
    Debug::PushLine(CurrentWorldPos, CurrentWorldPos + FacingDir * 0.1f, { 0, 1, 1, 1 });
  }
}
