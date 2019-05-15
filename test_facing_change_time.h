#pragma once

struct facing_turn_time_data_row
{
  float t;
  float dt;

  float   RootSpeed;
  float   StartAngle;
  float   CurrentAngle;
  float   DesiredAngle;
  int32_t TestNum;
};

struct facing_test
{
  float AngleThreshold;
  float MaxWaitTime;
  float MinimalSpeedToStart;
  float MaxTestAngle;

  bool  TestLeftSideTurns;
  bool  TestRightSideTurns;

  float   DesiredWordlFacing;
  int32_t RemainingDirectionTests;
};

inline facing_test
GetDefaultFacingTest()
{
  facing_test Result = {
    .AngleThreshold      = 5,
    .MaxWaitTime         = 1,
    .MinimalSpeedToStart = 0.3f,
    .MaxTestAngle        = 0.3f,
    .TestLeftSideTurns   = true,
    .TestRightSideTurns  = true,
  };
  return Result;
};

/*inline void
GenerateGoalsFromInput(mm_frame_info* OutGoals, mm_frame_info* OutMirroredGoals,
                       trajectory* Trajectories, Memory::stack_allocator* TempAlloc,
                       const blend_stack* BlendStacks, const float* GlobalTimes,
                       const Anim::skeleton* const*     Skeletons,
                       const mm_controller_data* const* MMControllers,
                       const mm_input_controller* InputControllers, const int32_t* EntityIndices,
                       int32_t Count, const entity* Entities, const game_input* Input,
                       vec3 CameraForward)
{
  vec3 Dir         = {};
  vec3 ViewForward = Math::Normalized(vec3{ CameraForward.X, 0, CameraForward.Z });

  for(int e = 0; e < Count; e++)
  {
    quat R = Entities[EntityIndices[e]].Transform.R;
    R.V *= -1;
    vec3 GoalVelocity = Math::MulMat3Vec3(Math::QuatToMat3(R), InputControllers[e].MaxSpeed * Dir);
    vec3 GoalFacing =
      InputControllers[e].UseStrafing
        ? Math::MulMat3Vec3(Math::QuatToMat3(R), ViewForward)
        : (Math::Length(Dir) != 0 ? Math::MulMat3Vec3(Math::QuatToMat3(R), Dir) : vec3{ 0, 0, 1 });

    blend_in_info DominantBlend = BlendStacks[e].Peek();
    float         LocalAnimTime = GetLocalSampleTime(DominantBlend.Animation, GlobalTimes[e],
                                             DominantBlend.GlobalAnimStartTime);

    mat4 InvEntityMatrix = Math::InvMat4(TransformToMat4(Entities[EntityIndices[e]].Transform));
    trajectory_update_args TrajectoryArgs =
      { .PositionBias    = InputControllers[e].PositionBias,
        .DirectionBias   = InputControllers[e].DirectionBias,
        .InvEntityMatrix = InvEntityMatrix };
    GetMMGoal(&OutGoals[e], &OutMirroredGoals[e], &Trajectories[e], TempAlloc, Skeletons[e],
              DominantBlend.Animation, DominantBlend.Mirror, LocalAnimTime, GoalVelocity,
              GoalFacing, MMControllers[e]->Params.DynamicParams.TrajectoryTimeHorizon,
              MMControllers[e]->Params.FixedParams, &TrajectoryArgs);
  }
}*/

inline facing_turn_time_data_row
MeasureFacingAngleDeviation(float DesiredAngle, float CurrentAngle, float t, float dt,
                            bool UsingMirrors)
{
  facing_turn_time_data_row Result = {};
  // Get StartAngle

  // If Threshold is reached
  // Increment the TestNum
  // Change DesiredAngle
  // SetStartAngle

  // Store Data

  return Result;
}
