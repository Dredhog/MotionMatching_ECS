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
};

struct active_test
{
  data_table DataTable;
  int32_t    EntityIndex;
  int32_t    Type;
  union {
    foot_skate_test FootSkateTest;
    facing_test     ControllerFacingTest;
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
    CreateFootSkateTestAndTable(Resources, Name,TEST_AnimationFootSkate, Test, EntityIndex);

    // Add animation reference
    Resources->Animations.AddReference(Test.AnimationRID);
  }

  inline void
  CreateControllerFootSkateTest(Resource::resource_manager* Resources, const foot_skate_test& Test,
                                rid ControllerRID, int32_t EntityIndex)
  {
    // Creat table name
    char Name[MAX_TABLE_NAME_LENGTH];
    path ControllerPath =
      Resources->MMControllerPaths[Resources->GetMMControllerPathIndex(ControllerRID)];
    mm_params* Params = &Resources->GetMMController(ControllerRID)->Params;

    sprintf(Name, "%s_foot_skate%s", strrchr(ControllerPath.Name, '/') + 1,
            Params->DynamicParams.MatchMirroredAnimations ? "Mirror" : "");

    // Create the table and the active_test structure
    CreateFootSkateTestAndTable(Resources, Name, TEST_ControllerFootSkate, Test, EntityIndex);
  }

  inline void
  CreateTrajectoryDeviationTest(int32_t EntityIndex)
  { // Create table name

    // Create table columns

    // Add active test to stack
  }

  inline void
  CreateFacingChangeTest(foot_skate_test Test, int32_t EntityIndex)
  {
    // Create table name

    // Create table columns

    // Add active test to stack
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
  GetEntityTestIndex(int32_t EntityIndex, int32_t TestType = -1)
  {
    for(int i = 0; i < ActiveTests.Count; i++)
    {
      if(ActiveTests[i].EntityIndex == EntityIndex &&
         (TestType == -1 || TestType == ActiveTests[i].Type))
      {
        return i;
      }
    }
    return -1;
  }

  /*inline bool
  GetEntityTestCopy(active_test* OutTest, int32_t EntityIndex, int32_t TestType = -1,
                    int32_t* OutTestIndex = NULL)
  {
    int32_t TestIndex = GetEntityTestIndex(EntityIndex, TestType);

    if(OutTestIndex)
    {
      *OutTestIndex = TestIndex;
    }

    if(TestIndex != -1)
    {
      if(OutTest)
      {
        *OutTest = ActiveTests[TestIndex];
      }
      return true;
    }
    return false;
  }*/

private:
  inline void
  DestroyAnimFootSkateTest(Resource::resource_manager* Resources, foot_skate_test* DestroyedTest)
  {
    Resources->Animations.RemoveReference(DestroyedTest->AnimationRID);
  }
};
