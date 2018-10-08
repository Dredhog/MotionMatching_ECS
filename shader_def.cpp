#include "shader_def.h"

#include <cassert>
#include <cstring>
//-----------------------------------API INTERNAL CODE------------------------------------

static const int NAME_MAX_LENGTH        = 30;
static const int SHADER_PARAM_MAX_COUNT = 10;
static const int SHADER_DEF_MAX_COUNT   = 10;

struct fixed_string
{
  char Str[NAME_MAX_LENGTH];
};

struct shader_def
{
  int32_t          Count;
  int32_t          CurrentIndex;
  fixed_string     Names[SHADER_PARAM_MAX_COUNT];
  fixed_string     UniformNames[SHADER_PARAM_MAX_COUNT];
  shader_param_def Params[SHADER_PARAM_MAX_COUNT];
};

struct shader_defs
{
  int32_t      Count;
  fixed_string Names[SHADER_DEF_MAX_COUNT];
  int32_t      Types[SHADER_DEF_MAX_COUNT];
  uint32_t     IDs[SHADER_DEF_MAX_COUNT];
  shader_def   Defs[SHADER_DEF_MAX_COUNT];
} static g_ShaderDefs;

static int32_t
CompareFixedString(const fixed_string* FixedString, const char* String)
{
  assert(String);
  size_t SourceStringLength = strnlen(String, sizeof(fixed_string));
  assert(SourceStringLength < sizeof(fixed_string));

  return strncmp((const char*)FixedString, String, SourceStringLength);
}

static void
SetFixedString(fixed_string* Dest, const char* Source)
{
  assert(Source);
  assert(Dest);
  size_t SourceLength = strnlen(Source, sizeof(fixed_string));
  assert(SourceLength < sizeof(fixed_string));
  strncpy((char*)Dest->Str, Source, SourceLength);
}

//---------------------------------PUBLIC API IMPLEMENTATION-------------------------------

shader_def*
AddShaderDef(int32_t ShaderType, uint32_t ShaderID, const char* ShaderName)
{
  SetFixedString(&g_ShaderDefs.Names[g_ShaderDefs.Count], ShaderName);
  g_ShaderDefs.Types[g_ShaderDefs.Count]      = ShaderType;
  g_ShaderDefs.IDs[g_ShaderDefs.Count]        = ShaderID;
  g_ShaderDefs.Defs[g_ShaderDefs.Count].Count = 0;
  return &g_ShaderDefs.Defs[g_ShaderDefs.Count++];
}

void
AddParamDef(shader_def* ShaderDefPtr, const char* ParamName, const char* UniformName,
            shader_param_def ParamDef)
{
  assert(ShaderDefPtr);
  SetFixedString(&ShaderDefPtr->Names[ShaderDefPtr->Count], ParamName);
  if(UniformName)
  {
    SetFixedString(&ShaderDefPtr->UniformNames[ShaderDefPtr->Count], UniformName);
  }
  else
  {
    memset(&ShaderDefPtr->UniformNames[ShaderDefPtr->Count], 0, sizeof(fixed_string));
  }
  ShaderDefPtr->Params[ShaderDefPtr->Count++] = ParamDef;
}

int32_t
GetShaderType(const struct shader_def* ShaderDef)
{
  assert(ShaderDef);
  size_t DefIndex = ShaderDef - g_ShaderDefs.Defs;
  assert(DefIndex >= 0);

  return g_ShaderDefs.Types[DefIndex];
}

uint32_t
GetShaderID(const shader_def* ShaderDef)
{
  assert(ShaderDef);
  size_t DefIndex = ShaderDef - g_ShaderDefs.Defs;
  assert(DefIndex >= 0);

  return g_ShaderDefs.IDs[DefIndex];
}

const char*
GetShaderName(const shader_def* ShaderDef)
{
  assert(ShaderDef);
  size_t DefIndex = ShaderDef - g_ShaderDefs.Defs;
  assert(DefIndex >= 0);

  return (char*)g_ShaderDefs.Names[DefIndex].Str;
}

bool
GetShaderDef(struct shader_def** OutputDefPtrPtr, const char* ShaderName)
{
  for(int i = 0; i < g_ShaderDefs.Count; i++)
  {
    if(CompareFixedString(&g_ShaderDefs.Names[i], ShaderName) == 0)
    {
      *OutputDefPtrPtr = g_ShaderDefs.Defs + i;
      return true;
    }
  }
  *OutputDefPtrPtr = NULL;
  return false;
}

bool
GetShaderDef(struct shader_def** OutputDefPtrPtr, int32_t ShaderType)
{
  for(int i = 0; i < g_ShaderDefs.Count; i++)
  {
    if(g_ShaderDefs.Types[i] == ShaderType)
    {
      *OutputDefPtrPtr = g_ShaderDefs.Defs + i;
      return true;
    }
  }
  *OutputDefPtrPtr = NULL;
  return false;
}

bool
GetShaderParamDef(shader_param_def* OutputParamPtr, const shader_def* ShaderDefPtr,
                  const char* ParamName)
{
  int Count = ShaderDefPtr->Count;
  for(int i = 0; i < Count; i++)
  {
    if(CompareFixedString(&ShaderDefPtr->Names[i], ParamName) == 0)
    {
      *OutputParamPtr = ShaderDefPtr->Params[i];
      return true;
    }
  }
  return false;
}

void
ResetShaderDefIterator(shader_def* ShaderDef)
{
  assert(ShaderDef);
  ShaderDef->CurrentIndex = 0;
}

// returns0 on error
bool
GetNextShaderParam(named_shader_param_def* Output, struct shader_def* ShaderDef)
{
  int Index = ShaderDef->CurrentIndex;
  if(Index < ShaderDef->Count)
  {
    named_shader_param_def Result = {};
    Result.Name                   = (char*)ShaderDef->Names[Index].Str;
    Result.OffsetIntoMaterial     = ShaderDef->Params[Index].OffsetIntoMaterial;
    Result.Type                   = ShaderDef->Params[Index].Type;
    ShaderDef->CurrentIndex++;
    *Output = Result;
    return true;
  }
  ShaderDef->CurrentIndex = 0;
  return false;
}
