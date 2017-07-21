#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "linear_math/quaternion.h"
#include "collision_testing.h"
#include "mesh.h"

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

struct velocity
{
  vec3 v;
  vec3 w;
};

struct rigid_body
{
  state         State;
  Render::mesh* Mesh;

  // Constant state
  float Mass;
  float MassInv;
  mat3  InertiaBody;
  mat3  InertiaBodyInv;

  // Compute on iteration
  mat3 InertiaInv;
  mat3 R;
  bool RegardGravity;
};

vec3  g_Force;
vec3  g_ForceStart;
float g_Slop        = 0.01f;
float g_Bias        = 0;
float g_Restitution = 0.5;
bool  g_ApplyingForce;
bool  g_UseGravity;
bool  g_ApplyingTorque;

const int  RIGID_BODY_COUNT = 2;
rigid_body g_RigidBodies[RIGID_BODY_COUNT]; // Indices correspond to entities

#define DYDT_FUNC(name) void name(state_derivative* dY, state* Y, int Count, float t0, float t1)
typedef DYDT_FUNC(dydt_func);

void
ComputeExternalForcesAndTorques(state_derivative* dYi, const rigid_body* RigidBody)
{
  dYi->Force  = {};
  dYi->Torque = {};

  if(g_UseGravity && RigidBody->RegardGravity)
  {
    dYi->Force += (9.81f * RigidBody->Mass) * vec3{ 0, -1, 0 };
  }

  // TempTesting code
  if(g_ApplyingForce)
  {
    dYi->Force += g_Force;
  }
  if(g_ApplyingTorque)
  {
    vec3 Radius = (g_ForceStart + g_Force) - RigidBody->State.X;
    dYi->Torque += Math::Cross(Radius, g_Force);
  }
}

DYDT_FUNC(DYDT)
{
  velocity    V[RIGID_BODY_COUNT];
  const float dt = t1 - t0;
  for(int i = 0; i < Count; i++)
  {
    assert(FloatsEqualByThreshold(Math::Length(Y[i].q), 1.0f, 0.001f));
    g_RigidBodies[i].R = Math::QuatToMat3(Y[i].q);
    g_RigidBodies[i].InertiaInv =
      Math::MulMat3(g_RigidBodies[i].R, Math::MulMat3(g_RigidBodies[i].InertiaBodyInv,
                                                      Math::Transposed3(g_RigidBodies[i].R)));

    // Apply external forces
    ComputeExternalForcesAndTorques(&dY[i], &g_RigidBodies[i]);
    Y[i].P += dt * (g_RigidBodies[i].MassInv * dY[i].Force);
    Y[i].L += dt * Math::MulMat3Vec3(g_RigidBodies[i].InertiaInv, dY[i].Torque);

    // Extract tentative velocity
    V[i].v = Y[i].P * g_RigidBodies[i].MassInv;
    V[i].w = Math::MulMat3Vec3(g_RigidBodies[i].InertiaInv, Y[i].L);
  }

  // Collision impulses
  mat4 TransformA =
    Math::MulMat4(Math::Mat4Translate(Y[0].X), Math::Mat3ToMat4(g_RigidBodies[0].R));
  mat4 TransformB =
    Math::MulMat4(Math::Mat4Translate(Y[1].X), Math::Mat3ToMat4(g_RigidBodies[1].R));

  sat_contact_manifold Manifold;
  if(TestHullvsHull(&Manifold, g_RigidBodies[0].Mesh, g_RigidBodies[1].Mesh, TransformA,
                    TransformB))
  {
    assert(Manifold.PointCount == 1);
    vec3 n     = Manifold.Normal;
    vec3 P     = Manifold.Points[0].Position;
    vec3 rA    = P - Y[0].X;
    vec3 rB    = P - Y[1].X;
    vec3 pAdot = V[0].v + Math::Cross(V[0].w, rA);
    vec3 pBdot = V[1].v + Math::Cross(V[1].w, rB);
    mat3 InvIA = g_RigidBodies[0].InertiaInv;
    mat3 InvIB = g_RigidBodies[1].InertiaInv;

    vec3  vRel      = -(1.0f + g_Restitution) * (pBdot - pAdot);
    float Nominator = Math::Dot(vRel, n);
    float term1     = g_RigidBodies[0].MassInv + g_RigidBodies[1].MassInv;
    float term2     = Math::Dot(n, Math::MulMat3Vec3(InvIA, Math::Cross(Math::Cross(rA, n), rA)));
    float term3     = Math::Dot(n, Math::MulMat3Vec3(InvIB, Math::Cross(Math::Cross(rB, n), rB)));

    vec3 Impulse = (Nominator / (term1 + term2 + term3)) * n;
    V[0].v -= Impulse * g_RigidBodies[0].MassInv;
    Y[0].P -= Impulse;
    V[1].v += Impulse * g_RigidBodies[1].MassInv;
    Y[1].P += Impulse;

    V[0].w -= Math::MulMat3Vec3(InvIA, Math::Cross(rA, Impulse));
    Y[0].L -= Math::Cross(rA, Impulse);
    V[1].w += Math::MulMat3Vec3(InvIB, Math::Cross(rB, Impulse));
    Y[1].L += Math::Cross(rB, Impulse);
  }

  for(int i = 0; i < Count; i++)
  {
    dY[i].v = V[i].v;

    quat qOmega;
    qOmega.S   = 0;
    qOmega.V   = V[i].w;
    dY[i].qDot = 0.5f * (qOmega * Y[i].q);
  }
}

void
ODE(state* Y, int Count, float t1, float t2, dydt_func dydt)
{
  state_derivative dY[RIGID_BODY_COUNT];
  dydt(dY, Y, Count, t1, t2);

  const float dt = t2 - t1;
  // Y(t1) = Y(t0) + dY(t0)
  for(int i = 0; i < Count; i++)
  {
    Y[i].X += dY[i].v * dt;
    Y[i].q = Y[i].q + (dY[i].qDot * dt);
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
        g_RigidBodies[i].Mass           = 1.0f;
        g_RigidBodies[i].MassInv        = 1.0f / g_RigidBodies[i].Mass;
        g_RigidBodies[i].InertiaBody    = Math::Mat3Scale(1, 3, 5);
        g_RigidBodies[i].InertiaBodyInv = Math::Mat3Scale(1, 1.0f / 3.0f, 1.0f / 5.0f);
      }
    }
    g_RigidBodies[0].Mesh          = GameState->MeshA;
    g_RigidBodies[0].RegardGravity = true;
    g_RigidBodies[1].Mesh          = GameState->MeshB;
    g_RigidBodies[1].RegardGravity = false;

    g_RigidBodies[1].Mass           = 0;
    g_RigidBodies[1].MassInv        = 0;
    g_RigidBodies[1].InertiaBodyInv = {};

    ODE(Y, RIGID_BODY_COUNT, 0.0f, 0.0f + (FRAME_TIME_MS / 1000.0f), DYDT);

    for(int i = 0; i < RIGID_BODY_COUNT; i++)
    {
      g_RigidBodies[i].State                       = Y[i];
      GameState->Entities[i].Transform.Rotation    = Math::QuatToEuler(Y[i].q);
      GameState->Entities[i].Transform.Translation = g_RigidBodies[i].State.X;
      {
        vec3 Omega = Math::MulMat3Vec3(g_RigidBodies[i].InertiaInv, Y[i].L);
        Debug::PushLine(Y[i].X, Y[i].X + Omega, { 0, 1, 0, 1 });
        Debug::PushWireframeSphere(Y[i].X + Omega, 0.05f, { 0, 1, 0, 1 });
        Debug::PushLine(Y[i].X, Y[i].X + Y[i].L);
        Debug::PushWireframeSphere(Y[i].X + Y[i].L, 0.05f);
      }
    }
  }
}
