#pragma once

struct facing_turn_time_data_row
{
  float   TimeTaken;
  int32_t Passed;

  float LocalTargetAngle;
  float AngleThreshold;
};

struct facing_test
{
	// Test Limitations
  bool  TestLeftSideTurns;
  bool  TestRightSideTurns;
  float MaxTestAngle;

  // Test Start Conditions
  float MinimalSpeedToStart;

  // Test End Conditions
  float TargetAngleThreshold;
  float MaxWaitTime;

	//Current Test Data
  bool HasActiveCase;
  float ElapsedTime;

  float TestStartLocalTargetAngle;
  mat3  InvTargetBasis;
  vec3  TargetWorldFacing;

  //Total Direction Changes Remaining
  int32_t RemainingCaseCount;
};

inline facing_test
GetDefaultFacingTest()
{
  facing_test Result = {
    .TargetAngleThreshold = 5,
    .MaxWaitTime          = 1,
    .MinimalSpeedToStart  = 0.3f,
    .MaxTestAngle         = 0.3f,
    .TestLeftSideTurns    = true,
    .TestRightSideTurns   = true,
    .RemainingCaseCount   = 10,
  };
  return Result;
};
