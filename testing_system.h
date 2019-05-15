#pragma once

#include "csv_data_table.h"
#include "basic_data_structures.h"
#include "test_foot_skate.h"
#include "test_facing_change_time.h"
#include "test_trajectory_following.h"

#define MAX_SIMULTANEOUS_TEST_COUNT 10

enum test_type
{
  TEST_AnimationFootSkate,
  TEST_ControllerFootSkate,
  TEST_TrajectoryFollowing,
  TEST_FacingChange,
	TEST_Count,
};

struct active_test
{
  data_table DataTable;
  int32_t    EntityIndex;
  int32_t    Type;
  union {
    foot_skate_test FootSkateTest;
    facing_test     FacingTest;
    follow_test     FollowTest;
  };
};

struct testing_system
{
  fixed_stack<active_test, MAX_SIMULTANEOUS_TEST_COUNT> ActiveTests;

  inline void
  CreateFootSkateTestAndTable(Resource::resource_manager* Resrouces, const char* FileName,
                              int32_t TestType, const foot_skate_test& Test, int32_t EntityIndex)
  {
    active_test NewTest = {};

    // Copy data to new active_test struct
    NewTest.EntityIndex   = EntityIndex;
    NewTest.FootSkateTest = Test;
    NewTest.Type          = TestType;

    // Create table columns
    AddColumn(&NewTest.DataTable.Header, "t", COLUMN_TYPE_float, offsetof(foot_skate_data_row, t));
    AddColumn(&NewTest.DataTable.Header, "dt", COLUMN_TYPE_float,
              offsetof(foot_skate_data_row, dt));
    AddColumn(&NewTest.DataTable.Header, "l_vel_x", COLUMN_TYPE_float,
              offsetof(foot_skate_data_row, LeftFootXVel));
    AddColumn(&NewTest.DataTable.Header, "l_vel_z", COLUMN_TYPE_float,
              offsetof(foot_skate_data_row, LeftFootZVel));
    AddColumn(&NewTest.DataTable.Header, "l_h", COLUMN_TYPE_float,
              offsetof(foot_skate_data_row, LeftFootHeight));
    AddColumn(&NewTest.DataTable.Header, "r_vel_x", COLUMN_TYPE_float,
              offsetof(foot_skate_data_row, RightFootXVel));
    AddColumn(&NewTest.DataTable.Header, "r_vel_z", COLUMN_TYPE_float,
              offsetof(foot_skate_data_row, RightFootZVel));
    AddColumn(&NewTest.DataTable.Header, "r_h", COLUMN_TYPE_float,
              offsetof(foot_skate_data_row, RightFootHeight));
    AddColumn(&NewTest.DataTable.Header, "anim_count", COLUMN_TYPE_int32,
              offsetof(foot_skate_data_row, AnimCount));
    AddColumn(&NewTest.DataTable.Header, "anim_index", COLUMN_TYPE_int32,
              offsetof(foot_skate_data_row, AnimIndex));
    // Allocate table
    const int32_t MaxRowCount = 100 * 90;
    CreateTable(&NewTest.DataTable, FileName, MaxRowCount, sizeof(foot_skate_data_row));

    // Add active test to stack
    ActiveTests.Push(NewTest);
  }

  inline void
  CreateAnimationFootSkateTest(Resource::resource_manager* Resources, const foot_skate_test& Test,
                               int32_t EntityIndex)
  {
    // Create table name
    char Name[MAX_TABLE_NAME_LENGTH];

    path AnimationPath =
      Resources->AnimationPaths[Resources->GetAnimationPathIndex(Test.AnimationRID)];
    sprintf(Name, "%s_foot_skate", strrchr(AnimationPath.Name, '/') + 1);

    // Create the data table
    CreateFootSkateTestAndTable(Resources, Name, TEST_AnimationFootSkate, Test, EntityIndex);

    // Add animation reference
    Resources->Animations.AddReference(Test.AnimationRID);
  }

  inline void
  GenerateControllerTestName(char* OutName, size_t MaxNameLength,
                             Resource::resource_manager* Resources, rid ControllerRID,
                             int32_t TestType)
  {
    path ControllerPath =
      Resources->MMControllerPaths[Resources->GetMMControllerPathIndex(ControllerRID)];
		//Making sure that .controller does not appear in the .csv file name
    char* DotBeforeExtensionPtr = strrchr(ControllerPath.Name, '.');
    *DotBeforeExtensionPtr      = '\0';

    mm_params* Params           = &Resources->GetMMController(ControllerRID)->Params;

    assert(TestType == TEST_ControllerFootSkate || TestType == TEST_FacingChange ||
           TestType == TEST_TrajectoryFollowing);
    const char* TestFileString = TestType == TEST_ControllerFootSkate
                                   ? "ctrl_skate"
                                   : (TestType == TEST_FacingChange ? "facing" : "follow");

    snprintf(OutName, MaxNameLength, "%s_%s%s", strrchr(ControllerPath.Name, '/') + 1,
             TestFileString, Params->DynamicParams.MatchMirroredAnimations ? "_mirror" : "");
  }

  inline void
  CreateControllerFootSkateTest(Resource::resource_manager* Resources, const foot_skate_test& Test,
                                rid ControllerRID, int32_t EntityIndex)
  {
    // Creat table name
    char Name[MAX_TABLE_NAME_LENGTH];
    GenerateControllerTestName(Name, ArrayCount(Name), Resources, ControllerRID,
                               TEST_ControllerFootSkate);

    // Create the table and the active_test structure
    CreateFootSkateTestAndTable(Resources, Name, TEST_ControllerFootSkate, Test, EntityIndex);
  }

  inline void
  CreateFacingChangeTest(Resource::resource_manager* Resources, rid ControllerRID,
                         const facing_test& Test, int32_t EntityIndex)
  {
    active_test NewTest = {};

    // Copy data to new active_test struct
    NewTest.EntityIndex = EntityIndex;
    NewTest.FacingTest  = Test;
    NewTest.Type        = TEST_FacingChange;

    char FileName[MAX_TABLE_NAME_LENGTH];
    GenerateControllerTestName(FileName, ArrayCount(FileName), Resources, ControllerRID,
                               TEST_FacingChange);

    // Create table columns
    AddColumn(&NewTest.DataTable.Header, "padded", COLUMN_TYPE_int32,
              offsetof(facing_turn_time_data_row, Passed));
    AddColumn(&NewTest.DataTable.Header, "turn_time", COLUMN_TYPE_float,
              offsetof(facing_turn_time_data_row, TimeTaken));
    AddColumn(&NewTest.DataTable.Header, "angle", COLUMN_TYPE_float,
              offsetof(facing_turn_time_data_row, LocalTargetAngle));
    AddColumn(&NewTest.DataTable.Header, "angle_threshold", COLUMN_TYPE_float,
              offsetof(facing_turn_time_data_row, AngleThreshold));
    // Allocate table
    const int32_t MaxRowCount = 100 * 90;
    CreateTable(&NewTest.DataTable, FileName, MaxRowCount, sizeof(facing_turn_time_data_row));

    // Add active test to stack
    ActiveTests.Push(NewTest);
  }

  inline void
  CreateTrajectoryDeviationTest(Resource::resource_manager* Resources, rid ControllerRID,
                                const follow_test& Test, int32_t EntityIndex)
  {
    active_test NewTest = {};

    // Copy data to new active_test struct
    NewTest.EntityIndex = EntityIndex;
    NewTest.FollowTest  = Test;
    NewTest.Type        = TEST_TrajectoryFollowing;

    char FileName[MAX_TABLE_NAME_LENGTH];
    GenerateControllerTestName(FileName, ArrayCount(FileName), Resources, ControllerRID,
                               TEST_TrajectoryFollowing);

    //Create table collumns
    AddColumn(&NewTest.DataTable.Header, "t", COLUMN_TYPE_float,
              offsetof(trajectory_follow_data_row, t));
    AddColumn(&NewTest.DataTable.Header, "dt", COLUMN_TYPE_float,
              offsetof(trajectory_follow_data_row, dt));
    AddColumn(&NewTest.DataTable.Header, "distance_from_segment", COLUMN_TYPE_float,
              offsetof(trajectory_follow_data_row, DistanceToSegment));
    AddColumn(&NewTest.DataTable.Header, "distance_from_spline", COLUMN_TYPE_float,
              offsetof(trajectory_follow_data_row, DistanceToSpline));
    AddColumn(&NewTest.DataTable.Header, "signed_angle_from_segment", COLUMN_TYPE_float,
              offsetof(trajectory_follow_data_row, SignedAngleFromLine));
    AddColumn(&NewTest.DataTable.Header, "signed_angle_from_spline", COLUMN_TYPE_float,
              offsetof(trajectory_follow_data_row, SignedAngleFromSpline));
    AddColumn(&NewTest.DataTable.Header, "next_waypoint_index", COLUMN_TYPE_int32,
              offsetof(trajectory_follow_data_row, NextWaypointIndex));
    AddColumn(&NewTest.DataTable.Header, "total_distance_traveled", COLUMN_TYPE_int32,
              offsetof(trajectory_follow_data_row, TotalDistanceTraveled));
    AddColumn(&NewTest.DataTable.Header, "projected_distance_traveled", COLUMN_TYPE_int32,
              offsetof(trajectory_follow_data_row, TotalProjectedDistanceTraveled));
    AddColumn(&NewTest.DataTable.Header, "percentage_in_trajectory", COLUMN_TYPE_int32,
              offsetof(trajectory_follow_data_row, PercentageInTrajectory));

    //Allocate table
    const int32_t MaxRowCount = 100 * 90;
    CreateTable(&NewTest.DataTable, FileName, MaxRowCount, sizeof(trajectory_follow_data_row));

    // Add active test to stack
    ActiveTests.Push(NewTest);
  }

  inline void
  DestroyTest(Resource::resource_manager* Resources, int32_t EntityIndex, int32_t TestType)
  {
    int32_t DestroyedTestIndex;
    assert((DestroyedTestIndex = GetEntityTestIndex(EntityIndex, TestType)) != -1);
    DestroyTest(Resources, DestroyedTestIndex);
  }

  inline void
  WriteTestToCSV(int32_t TestIndex, const char* OptionalName = NULL)
  {
    WriteDataTableToCSV(ActiveTests[TestIndex].DataTable, OptionalName);
  }

  inline void
  DestroyTest(Resource::resource_manager* Resources, int32_t TestIndex)
  {
    switch(ActiveTests[TestIndex].Type)
    {
      case TEST_AnimationFootSkate:
      {
        DestroyAnimFootSkateTest(Resources, &ActiveTests[TestIndex].FootSkateTest);
        break;
      }
      case TEST_ControllerFootSkate:
      {
        break;
      }
      case TEST_TrajectoryFollowing:
      {
        break;
      }
      case TEST_FacingChange:
      {
        break;
      }
      default:
      {
        assert(0 && "Assert: cannot find such test type to remove");
        break;
      }
    }
    DestroyTable(&ActiveTests[TestIndex].DataTable);
    ActiveTests.Remove(TestIndex);
  }

  int32_t
  GetEntityTestIndex(int32_t EntityIndex, int32_t TestType)
  {
    assert(0 <= TestType && TestType < TEST_Count);
    for(int i = 0; i < ActiveTests.Count; i++)
    {
      if(ActiveTests[i].EntityIndex == EntityIndex && ActiveTests[i].Type == TestType)
      {
        return i;
      }
    }
    return -1;
  }

private:
  inline void
  DestroyAnimFootSkateTest(Resource::resource_manager* Resources, foot_skate_test* DestroyedTest)
  {
    Resources->Animations.RemoveReference(DestroyedTest->AnimationRID);
  }
};
