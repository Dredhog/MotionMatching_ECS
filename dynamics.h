#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "linear_math/quaternion.h"
#include "collision.h"
#include "mesh.h"
#include "misc.h"
#include <string.h>

struct velocity
{
  vec3 v;
  vec3 w;
};

vec3  g_Force;
vec3  g_ForceStart;
float g_Restitution;
float g_Bias;
float g_Slop;
bool  g_ApplyingForce;
bool  g_UseGravity;
bool  g_ApplyingTorque;

const int  RIGID_BODY_COUNT = 2;
rigid_body g_RigidBodies[RIGID_BODY_COUNT]; // Indices correspond to entities

hull g_CubeHull;

#define DYDT_FUNC(name) void name(state_derivative* dY, state* Y, int Count, float t0, float t1)
typedef DYDT_FUNC(dydt_func);

void
ComputeExternalForcesAndTorques(state_derivative* dYi, const rigid_body* RigidBody)
{
  dYi->Force  = {};
  dYi->Torque = {};

  if(g_UseGravity && RigidBody->RegardGravity)
  {
    dYi->Force += vec3{ 0, -9.81f * RigidBody->Mass, 0 };
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
    assert(g_RigidBodies[i].Mass == 0 ||
           FloatsEqualByThreshold((g_RigidBodies[i].MassInv * g_RigidBodies[i].Mass), 1.0f, 0.01f));
    Y[i].P += dt * (g_RigidBodies[i].MassInv * dY[i].Force);
    Y[i].L += dt * Math::MulMat3Vec3(g_RigidBodies[i].InertiaInv, dY[i].Torque);

    // Extract tentative velocity
    V[i].v = g_RigidBodies[i].MassInv * Y[i].P;
    V[i].w = Math::MulMat3Vec3(g_RigidBodies[i].InertiaInv, Y[i].L);
  }

  // Collision impulses
  mat4 TransformA =
    Math::MulMat4(Math::Mat4Translate(Y[0].X),
                  Math::MulMat4(g_RigidBodies[0].Mat4Scale, Math::Mat3ToMat4(g_RigidBodies[0].R)));
  mat4 TransformB =
    Math::MulMat4(Math::Mat4Translate(Y[1].X),
                  Math::MulMat4(g_RigidBodies[1].Mat4Scale, Math::Mat3ToMat4(g_RigidBodies[1].R)));

  sat_contact_manifold Manifold;
  if(SAT(&Manifold, TransformA, &g_CubeHull, TransformB, &g_CubeHull))
  {
    for(int i = 0; i < Manifold.PointCount; ++i)
    {
      vec3 n     = Manifold.Normal;
      vec3 P     = Manifold.Points[i].Position;
      vec3 rA    = P - Y[0].X;
      vec3 rB    = P - Y[1].X;
      vec3 pAdot = V[0].v + Math::Cross(V[0].w, rA);
      vec3 pBdot = V[1].v + Math::Cross(V[1].w, rB);
      mat3 InvIA = g_RigidBodies[0].InertiaInv;
      mat3 InvIB = g_RigidBodies[1].InertiaInv;

      vec3  vRel     = (pAdot - pBdot);
      float vRelNorm = Math::Dot(vRel, n);
      if(vRelNorm <= 0)
      {
        float term1 = g_RigidBodies[0].MassInv + g_RigidBodies[1].MassInv;
        float term2 = Math::Dot(n, Math::MulMat3Vec3(InvIA, Math::Cross(Math::Cross(rA, n), rA)));
        float term3 = Math::Dot(n, Math::MulMat3Vec3(InvIB, Math::Cross(Math::Cross(rB, n), rB)));

        float vBias     = (g_Bias / dt) * MaxFloat(0, Manifold.Points[i].Penetration - g_Slop);
        float Numerator = -vRelNorm * (1.0f + g_Restitution) + vBias;
        // float Numerator = -vRelNorm;
        vec3 Impulse = MaxFloat((Numerator / (term1 + term2 + term3)), 0) * n;
        V[0].v += Impulse * g_RigidBodies[0].MassInv;
        V[0].w += Math::MulMat3Vec3(InvIA, Math::Cross(rA, Impulse));

        V[1].v -= Impulse * g_RigidBodies[1].MassInv;
        V[1].w -= Math::MulMat3Vec3(InvIB, Math::Cross(rB, Impulse));

        if(0 < g_RigidBodies[0].Mass)
        {
          Y[0].P += Impulse;
          Y[0].L += Math::Cross(rA, Impulse);
        }
        if(0 < g_RigidBodies[1].Mass)
        {
          Y[1].P -= Impulse;
          Y[1].L -= Math::Cross(rB, Impulse);
        }
        // Debug::PushWireframeSphere(P, 1.0f);
      }
      Debug::PushLine(P, P + n * Manifold.Points[i].Penetration, { 0, 1, 0, 1 });
      Debug::PushWireframeSphere(P, 0.05f, { 0, 1, 0, 1 });
    }
  }

  if(isnan(Y[0].L.X) || isnan(Y[1].L.X))
  {
    char a = 9;
    assert(0 && "Error with NaN");
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

    if(0.0001f < Math::Length(Y[i].q))
    {
      Math::Normalize(&Y[i].q);
    }
    else
    {
      Y[i].q = { 1, 0, 0, 0 };
    }
  }
}

void
SimulateDynamics(game_state* GameState)
{
  state Y[RIGID_BODY_COUNT];

  if(2 <= GameState->EntityCount)
  {
    for(int i = 0; i < RIGID_BODY_COUNT; i++)
    {
      GameState->Entities[i].RigidBody.Mat4Scale =
        Math::Mat4Scale(GameState->Entities[i].Transform.Scale);
      GameState->Entities[i].RigidBody.Collider =
        GameState->Resources.GetModel(GameState->Entities[i].ModelID)->Meshes[0];

      g_RigidBodies[i] = GameState->Entities[i].RigidBody;

      Y[i] = g_RigidBodies[i].State;
    }

    ODE(Y, RIGID_BODY_COUNT, 0.0f, 0.0f + (FRAME_TIME_MS / 1000.0f), DYDT);

    for(int i = 0; i < RIGID_BODY_COUNT; i++)
    {
      g_RigidBodies[i].State = Y[i];

      GameState->Entities[i].RigidBody             = g_RigidBodies[i];
      GameState->Entities[i].Transform.Rotation    = Math::QuatToEuler(g_RigidBodies[i].State.q);
      GameState->Entities[i].Transform.Translation = g_RigidBodies[i].State.X;
    }
  }
}
