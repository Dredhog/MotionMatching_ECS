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

  // Test End Conditions
  float TargetAngleThreshold;
  float MaxWaitTime;

  // Current Test Data
  bool  HasActiveCase;
  float ElapsedTime;

  float TestStartLocalTargetAngle;
  mat3  InvTargetBasis;
  vec3  TargetWorldFacing;

  // Total Direction Changes Remaining
  int32_t RemainingCaseCount;
};

inline facing_test
GetDefaultFacingTest()
{
  facing_test Result = {};
  Result.TargetAngleThreshold = 10;
  Result.MaxWaitTime          = 3;
  Result.MaxTestAngle         = 180;
  Result.TestLeftSideTurns    = true;
  Result.TestRightSideTurns   = true;
  Result.RemainingCaseCount   = 50;
  return Result;
};
