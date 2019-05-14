#pragma once

struct facing_rech_time_data_row
{
  float t;
  float dt;

  float   RootSpeed;
  float   StartAngle;
  float   CurrentAngle;
  float   DesiredAngle;
  int32_t TestNum;
};

struct facing_test_instance
{
};

facing_rech_time_data_row
MeasureFacingAngleDeviation(float DesiredAngle, float CurrentAngle, float t, float dt,
                            bool UsingMirrors)
{
	facing_rech_time_data_row Result = {};
	//Get StartAngle

  // if(UsingMirrors)
  // Only generate angles on the same side

  // If Threshold is reached
  // Increment the TestNum
  // Change DesiredAngle
  // SetStartAngle

  // Store Data

  return Result;
}

struct ground_truth_movement_object
{
	float Position;
}

facing_reach_time_data_row
MeasureAndUpdateControlObjectFacingAngleDeviation(ground_truth_movement_object* GTObject,
                                                  float DesiredAngle, float t, float dt)
{
	
}
