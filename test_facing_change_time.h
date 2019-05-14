#pragma once

struct facing_change_time_data_row
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

inline facing_change_time_data_row
MeasureFacingAngleDeviation(float DesiredAngle, float CurrentAngle, float t, float dt,
                            bool UsingMirrors)
{
  facing_change_time_data_row Result = {};
  // Get StartAngle

  // if(UsingMirrors)
  // Only generate angles on the same side

  // If Threshold is reached
  // Increment the TestNum
  // Change DesiredAngle
  // SetStartAngle

  // Store Data

  return Result;
}
