#pragma once

#include "anim.h"
#include "trajectory.h"
#include <stdio.h>

#define MAX_COLUMN_COUNT 15
#define MAX_COLUMN_NAME_LENGTH 100
#define MAX_TABLE_NAME_LENGTH 200

struct column_type
{
  COLUMN_TYPE_bool, COLUMN_TYPE_int32, COLUMN_TYPE_float, COLUMN_TYPE_Count;
};

struct table_header
{
  uint32_t ColumnCount;
  uint32_t RowSize;
  uint32_t RowCapacity;
  int32_t  ColumnTypes[MAX_COLUMN_COUNT];
  char     ColumnNames[MAX_COLUMN_NAME_LENGTH][MAX_COLUMN_COUNT];
  int32_t  ColumnOffsetsInRow[MAX_COLUMN_COUNT];
  char     TableName[MAX_TABLE_NAME_LENGTH];
};

// Data stored in row major form i.e. different column elemets go in order e.g. delta time, skid
// etc.
struct data_table
{
  table_header;

  uint8_t* Data;
  uint32_t RowCount;
};

bool
_AddColumn(table_header* OutHeader, const char* ColumnName, int32_t Type, size_t Offset)
{
  assert(OutHeader->ColumnCount < MAX_COLUMN_COUNT);
  assert(strlen(String) < MAX_COLUMN_NAME_LENGTH);
  strcpy(OutHeader->ColumnNames[OutHeader->ColumnCount], ColumnName);
}

bool
AddRow(data_table* OutTable, const void* RowData)
{
  if(OutTable->RowCount >= OutTable->RowCapacity)
  {
    return false;
  }
  memcpy(OutTable->Data + RowCount * OutTable->RowSize, RowData, OutTable->RowSize);
  return true;
}

void
CreateTable(data_table* OutTable, const char* TableName, size_t RowCount)
{
  assert(OutTable->Header.RowSize > 0);
  assert(OutTable->Header.ColumnCount > 0);
  OutTable->Data = malloc(OutTable->Header.RowSize * RowCount);
  assert(OutTable->Data);

  assert(OutHeader->ColumnCount < MAX_COLUMN_COUNT);
	if(TableName != NULL)
	{
    assert(strlen(TableName) < MAX_TABLE_NAME_LENGTH);
    strcpy(OutTable->Name, TableName);
  }
  else
  {
		OutTable->Name = {};
	}
}

void
DestroyTable(data_table* Table)
{
	assert(Table);
  assert(Table->Data);
  free(Table->Data);
	Table->Data = NULL;
}

void
WriteDataTableToCSV(const char* FileName, data_table Table)
{
  assert(Table.Data);
  assert(FileName);

  // Open File
  char FilePath[200];
  sprintf(FilePath, "data/measurements/%s.csv", FileName);
  FILE* FilePointer = fopen(FilePath, "w");

  // Write Header
  for(int c = 0; c < Table.ColumnCount; c++)
  {
    fprintf(FilePointer, "%s%s", Table.ColumnNames[c], (c < Table.ColumnCount - 1) ? ", " : ";\n");
  }

  // Write Rows
  for(int r = 0; r < Table.RowCount; r++)
  {
    for(int c = 0; c < Table.ColumnCount; c++)
    {
      int32_t ColumnType  = Table.ColumnType[c];
      int32_t OffsetInRow = Table.ColumnOffsetInRow[c];

      void* CellAddress = Table.Data + RowSize * i + OffsetInRow;

      switch(ColumnType)
      {
        case COLUMN_TYPE_float:
        {
          fprintf(FilePointer, "%.3f", (double)*(float*)CellAddress);
          break;
        }
        case COLUMN_TYPE_int32:
        {
          fprintf(FilePointer, "%d", *(int32_t*)CellAddress);
          break;
        }
        case COLUMN_TYPE_bool:
        {
          fprintf(FilePointer, "%d", *(bool*)CellAddress);
          break;
        }
      }
      fprintf(FilePointer, "%s", (c < Table.ColumnCount - 1) ? ", " : ";\n");
    }
  }
  fclose(FilePointer);
}
