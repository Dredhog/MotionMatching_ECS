#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "misc.h"

struct state
{
  // Yi(t)
  vec3 X;
  mat3 R;
  vec3 P;
  vec3 L;
};

struct state_derivative
{
  // dYi(t)
  vec3 v;
  mat3 RDot;
  vec3 Force;
  vec3 Torque;
};

struct rigid_body
{
  state State;

  // Constant state
  float Mass;
  float MassInv;
  mat3  InertiaBody;
  mat3  InertiaBodyInv;

  bool IsDynamic;
};

const int  RIGID_BODY_COUNT = 2;
rigid_body g_RigidBodies[RIGID_BODY_COUNT]; // Indices correspond to entities

#define DYDT_FUNC(name) void name(state_derivative* dY, const state* Y, int Count, float t)

typedef DYDT_FUNC(dydt_func);

void
ComputeExternalForcesAndTorques(state_derivative* dYi, const rigid_body* RigidBody, float t)
{
  if(RigidBody->IsDynamic)
  {
    dYi->Force = (9.81f * RigidBody->Mass) * vec3{ 0, -1, 0 };
  }
  else
  {
    dYi->Force = {};
  }
  dYi->Torque = {};
}

DYDT_FUNC(DYDT)
{
  for(int i = 0; i < Count; i++)
  {
    dY[i].v = Y[i].P / g_RigidBodies[i].Mass;
    ComputeExternalForcesAndTorques(&dY[i], &g_RigidBodies[i], t);
  }
}

void
ODE(state* Y, int Count, float t1, float t2, dydt_func dydt)
{
  state_derivative dY[RIGID_BODY_COUNT];
  dydt(dY, Y, Count, t1);

  // Y(t1) = Y(t0) + dY(t0)
  for(int i = 0; i < Count; i++)
  {
    float* StateFloat      = (float*)&Y[i];
    float* DerivativeFloat = (float*)&dY[i];

    const float dt = t2 - t1;
    /*for(int j = 0; j < sizeof(state) / sizeof(float); j++)
    {
      *StateFloat += *DerivativeFloat * dt;
      ++StateFloat;
      ++DerivativeFloat;
    }*/
    {
      Y[i].X += dt * dY[i].v;
      for(int j = 0; j < ARRAY_SIZE(Y[i].R.e); j++)
      {
        Y[i].R.e[j] += dt * dY[i].RDot.e[j];
      }
      Y[i].P += dt * dY[i].Force;
      Y[i].L += dt * dY[i].Torque;
    }
  }
}

void
SimulateDynamics(game_state* GameState)
{
  assert(sizeof(state) == sizeof(state_derivative));

  state Y[RIGID_BODY_COUNT]; // Should not assume persistent state

  if(2 <= GameState->EntityCount)
  {
    // Coppying here only for testing
    g_RigidBodies[0].State.X   = GameState->Entities[0].Transform.Translation;
    g_RigidBodies[0].Mass      = 1;
    g_RigidBodies[0].IsDynamic = true;
    g_RigidBodies[1].State.X   = GameState->Entities[1].Transform.Translation;
    g_RigidBodies[1].Mass      = 1;
    g_RigidBodies[1].IsDynamic = true;
    //--------------------------------

    for(int i = 0; i < RIGID_BODY_COUNT; i++)
    {
      Y[i] = g_RigidBodies[i].State;
    }

    ODE(Y, 2, 0.0f, 0.0f + (FRAME_TIME_MS / 1000.0f), DYDT);

    for(int i = 0; i < RIGID_BODY_COUNT; i++)
    {
      g_RigidBodies[i].State = Y[i];
    }

    // Coppying here only for testing
    GameState->Entities[0].Transform.Translation = g_RigidBodies[0].State.X;
    GameState->Entities[1].Transform.Translation = g_RigidBodies[1].State.X;
    //--------------------------------
  }
}
