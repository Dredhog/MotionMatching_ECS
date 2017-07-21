#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "linear_math/quaternion.h"

struct state
{
  // Yi(t)
  vec3 X;
  quat q;
  vec3 P;
  vec3 L;
};

struct state_derivative
{
  // dYi(t)
  vec3 v;
  quat qDot;
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

  // Compute on iteration
  mat3 InertiaInv;
  mat3 R;
  vec3 Omega;
};

const int  RIGID_BODY_COUNT = 2;
rigid_body g_RigidBodies[RIGID_BODY_COUNT]; // Indices correspond to entities

vec3 g_Force;
vec3 g_ForceStart;
bool g_ApplyingForce;
bool g_ApplyingTorque;

#define DYDT_FUNC(name) void name(state_derivative* dY, state* Y, int Count, float t)
typedef DYDT_FUNC(dydt_func);

void
ComputeExternalForcesAndTorques(state_derivative* dYi, const rigid_body* RigidBody, float t)
{
  dYi->Force  = {};
  dYi->Torque = {};
  if(RigidBody->IsDynamic)
  {
#if 0
     dYi->Force = (9.81f * RigidBody->Mass) * vec3{ 0, -1, 0 };
#else
    // TempTesting code
    if(g_ApplyingForce)
    {
      dYi->Force = g_Force;
    }
    if(g_ApplyingTorque)
    {
      vec3 Radius = (g_ForceStart + g_Force) - RigidBody->State.X;
      dYi->Torque = Math::Cross(Radius, g_Force);
    }
#endif
  }
}

DYDT_FUNC(DYDT)
{
  for(int i = 0; i < Count; i++)
  {
    ComputeExternalForcesAndTorques(&dY[i], &g_RigidBodies[i], t);

    dY[i].v = Y[i].P / g_RigidBodies[i].Mass;

    if(!FloatsEqualByThreshold(Math::Length(Y[i].q), 1.0f, 0.001f))
    {
      Y[i].q = { 0, 1, 0, 0 };
    }
    Math::Normalize(&Y[i].q);
    assert(FloatsEqualByThreshold(Math::Length(Y[i].q), 1.0f, 0.001f));

    g_RigidBodies[i].R = Math::QuatToMat3(Y[i].q);
    g_RigidBodies[i].InertiaInv =
      Math::MulMat3(g_RigidBodies[i].R, Math::MulMat3(g_RigidBodies[i].InertiaBodyInv,
                                                      Math::Transposed3(g_RigidBodies[i].R)));
    g_RigidBodies[i].Omega = Math::MulMat3Vec3(g_RigidBodies[i].InertiaInv, Y[i].L);

    quat qOmega;
    qOmega.S   = 0;
    qOmega.V   = g_RigidBodies[i].Omega;
    dY[i].qDot = 0.5f * (qOmega * Y[i].q);
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
    for(int j = 0; j < sizeof(state) / sizeof(float); j++)
    {
      *StateFloat += *DerivativeFloat * dt;
      ++StateFloat;
      ++DerivativeFloat;
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
    g_RigidBodies[0].State.X = GameState->Entities[0].Transform.Translation;
    g_RigidBodies[1].State.X = GameState->Entities[1].Transform.Translation;
    g_RigidBodies[0].State.q = Math::EulerToQuat(GameState->Entities[0].Transform.Rotation);
    g_RigidBodies[1].State.q = Math::EulerToQuat(GameState->Entities[1].Transform.Rotation);
    //--------------------------------

    for(int i = 0; i < RIGID_BODY_COUNT; i++)
    {
      Y[i] = g_RigidBodies[i].State;
      if(g_RigidBodies[i].Mass != 1)
      {
        g_RigidBodies[i].Mass           = 1;
        g_RigidBodies[i].InertiaBody    = Math::Mat3Scale(1, 3, 5);
        g_RigidBodies[i].InertiaBodyInv = Math::Mat3Scale(1, 1.0f / 3.0f, 1.0f / 5.0f);
        g_RigidBodies[i].IsDynamic      = true;
      }
    }

    ODE(Y, RIGID_BODY_COUNT, 0.0f, 0.0f + (FRAME_TIME_MS / 1000.0f), DYDT);

    for(int i = 0; i < RIGID_BODY_COUNT; i++)
    {
      g_RigidBodies[i].State                       = Y[i];
      GameState->Entities[i].Transform.Rotation    = Math::QuatToEuler(Y[i].q);
      GameState->Entities[i].Transform.Translation = g_RigidBodies[i].State.X;
      {
        Debug::PushLine(Y[i].X, Y[i].X + g_RigidBodies[i].Omega, { 0, 1, 0, 1 });
        Debug::PushWireframeSphere(Y[i].X + g_RigidBodies[i].Omega, 0.05f, { 0, 1, 0, 1 });
        Debug::PushLine(Y[i].X, Y[i].X + Y[i].L);
        Debug::PushWireframeSphere(Y[i].X + Y[i].L, 0.05f);
      }
    }
  }
}
