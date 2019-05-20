#pragma once
#include "transform.h"
#include "rigid_body.h"
#include "anim.h"

/*
enum Component
{
  COMPONENT_Transform,
  COMPONENT_MVPMatrix,
  COMPONENT_MovementDelta,
  COMPONENT_AnimController,
  COMPONENT_RigidBody,
  COMPONENT_ModelRenderer,
  COMPONENT_InputController,
  COMPONENT_TrajectoryController,
  COMPONENT_MMData,
  COMPONENT_MMAnimationGoal,
};*/

#define FOR_ALL_NAMES(DO_FUNC) DO_FUNC(,transform) DO_FUNC(Anim,animation_player) DO_FUNC(,rigid_body)
#define GENERATE_ENUM(Unused, Name) COMPONENT_##Name,
#define GENERATE_STRING(Unused, Name) #Name,
#define GENERATE_COMPONENT_STRUCT_INFO(Namespace, Name) { (uint8_t)alignof(Namespace::Name), (uint16_t)sizeof(Namespace::Name) },

enum component_type
{
  FOR_ALL_NAMES(GENERATE_ENUM) GENERATE_ENUM(,Count)
};

static const char*    g_ComponentNameTable[COMPONENT_Count] = { FOR_ALL_NAMES(GENERATE_STRING) };
component_struct_info g_ComponentStructInfoTable[COMPONENT_Count] = { FOR_ALL_NAMES(
  GENERATE_COMPONENT_STRUCT_INFO) };
#undef FOR_ALL_NAMES
#undef GENERATE_ENUM
#undef GENERATE_STRING
