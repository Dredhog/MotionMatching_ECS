#pragma once

#include "anim.h"
#include "movement_spline.h"
#include <stdio.h>

#define MAX_COLUMN_COUNT 15
#define MAX_COLUMN_NAME_LENGTH 100
#define MAX_TABLE_NAME_LENGTH 200

enum column_type
{
  COLUMN_TYPE_bool,
  COLUMN_TYPE_int32,
  COLUMN_TYPE_float,
  COLUMN_TYPE_Count,
};

struct table_header
{
  uint32_t ColumnCount;
  int32_t  ColumnTypes[MAX_COLUMN_COUNT];
  char     ColumnNames[MAX_COLUMN_COUNT][MAX_COLUMN_NAME_LENGTH];
  int32_t  ColumnOffsetsInRow[MAX_COLUMN_COUNT];
};

// Data stored in row major form i.e. different column elemets go in order e.g. delta time, skid
// etc.
struct data_table
{
  table_header Header;

  // All fields set during CreateTable()
  char     Name[MAX_TABLE_NAME_LENGTH];
  uint8_t* Data;
  uint32_t RowCount;
  uint32_t RowCapacity;
  uint32_t RowSize;
};

inline void
AddColumn(table_header* OutHeader, const char* ColumnName, int32_t Type, size_t Offset)
{
  assert(OutHeader->ColumnCount < MAX_COLUMN_COUNT);
  assert(strlen(ColumnName) < MAX_COLUMN_NAME_LENGTH);
  strcpy(OutHeader->ColumnNames[OutHeader->ColumnCount], ColumnName);
  OutHeader->ColumnOffsetsInRow[OutHeader->ColumnCount] = (uint32_t)Offset;
  OutHeader->ColumnTypes[OutHeader->ColumnCount]        = Type;
  OutHeader->ColumnCount++;
}

inline bool
AddRow(data_table* OutTable, const void* NewRow, size_t NewRowSize)
{
	assert(OutTable->RowSize == NewRowSize);
  if(OutTable->RowCount >= OutTable->RowCapacity)
  {
    return false;
  }
  memcpy(OutTable->Data + OutTable->RowCount * OutTable->RowSize, NewRow, OutTable->RowSize);
  OutTable->RowCount++;
  return true;
}

inline void
CreateTable(data_table* OutTable, const char* TableName, size_t RowCapacity, size_t RowSize)
{
  assert(RowSize > 0);
  assert(OutTable->Header.ColumnCount > 0);
  assert(RowCapacity >= 1);
  assert(OutTable->Header.ColumnCount < MAX_COLUMN_COUNT);
  assert(TableName);
  assert(strlen(TableName) < MAX_TABLE_NAME_LENGTH);

  OutTable->RowCount    = 0;
  OutTable->RowSize     = (uint32_t)RowSize;
  OutTable->RowCapacity = (uint32_t)RowCapacity;

  OutTable->Data = (uint8_t*)malloc(OutTable->RowSize * RowCapacity);
  assert(OutTable->Data && "Assert: malloc failed");

  strcpy(OutTable->Name, TableName);
}

inline void
DestroyTable(data_table* Table)
{
	assert(Table);
  assert(Table->Data);
  free(Table->Data);
	Table->Data = NULL;
}

inline void
WriteDataTableToCSV(data_table Table, const char* FileName = NULL)
{
  assert(Table.Data);

  // Open File
  char FilePath[200];
  sprintf(FilePath, "data/measurements/%s.csv", FileName == NULL ? Table.Name : FileName);
  FILE* FilePointer = fopen(FilePath, "w");
	assert(FilePointer);

  // Write Header
  for(int c = 0; c < Table.Header.ColumnCount; c++)
  {
    fprintf(FilePointer, "%s%s", Table.Header.ColumnNames[c],
            (c < Table.Header.ColumnCount - 1) ? ", " : ",\n");
  }

  // Write Rows
  for(int r = 0; r < Table.RowCount; r++)
  {
    for(int c = 0; c < Table.Header.ColumnCount; c++)
    {
      int32_t ColumnType  = Table.Header.ColumnTypes[c];
      int32_t OffsetInRow = Table.Header.ColumnOffsetsInRow[c];

      void* CellAddress = Table.Data + Table.RowSize * r + OffsetInRow;

      switch(ColumnType)
      {
        case COLUMN_TYPE_float:
        {
          fprintf(FilePointer, "%.4f", (double)*(float*)CellAddress);
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
      fprintf(FilePointer, "%s", (c < Table.Header.ColumnCount - 1) ? ", " : ",\n");
    }
  }
  fclose(FilePointer);
}
