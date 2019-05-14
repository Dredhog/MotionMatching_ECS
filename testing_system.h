#pragma once

#include "entity.h"
#include "csv_data_table.h"
#include "basic_data_structures.h"

#define MAX_SIMULTANEOUS_TEST_COUNT 5

struct active_test
{
  int32_t EntityIndex;
  int32_t DataTable;
};

struct testing_system
{
	stack<data_tables, 5> ActiveTestData;


};


