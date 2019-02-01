#pragma once

#include "rigid_body.h"
#include "basic_data_structures.h"

enum constraint_type
{
  CONSTRAINT_Distance,
  CONSTRAINT_Point,
  CONSTRAINT_Contact,
  CONSTRAINT_Friction,
  CONSTRAINT_Count,
};

struct constraint
{
  uint32_t Type;
  int32_t  IndA;
  int32_t  IndB;

  float L;
  vec3  BodyRa;
  vec3  BodyRb;
  float Penetration;
  vec3  n;
  vec3  P;
  vec3  Tangent;
  // For friction
  int32_t ContactIndex;
};

const int RIGID_BODY_MAX_COUNT = 20;
const int CONSTRAINT_MAX_COUNT = 200;

struct physics_params
{
  vec3    ExternalForce;
  vec3    ExternalForceStart;
  int32_t PGSIterationCount;
  float   Beta;
  float   Mu;
};

struct physics_switches
{
  bool SimulateDynamics;
  bool PerformDynamicsStep;
  bool ApplyExternalForce;
  bool ApplyExternalTorque;
  bool UseGravity;
  bool SimulateFriction;

  bool VisualizeV;
  bool VisualizeOmega;
  bool VisualizeFc;
  bool VisualizeFcComponents;
  bool VisualizeFriction;
  bool VisualizeContactPoints;
  bool VisualizeContactManifold;
};

struct physics_world
{
  rigid_body RigidBodies[RIGID_BODY_MAX_COUNT]; // Indices correspond to entities
  fixed_stack<constraint, CONSTRAINT_MAX_COUNT> Constraints;
  int                                           RBCount;
  physics_params                                Params;
  physics_switches                              Switches;
};

void SimulateDynamics(physics_world* World);
