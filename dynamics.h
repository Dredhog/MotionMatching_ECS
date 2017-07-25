#include "rigid_body.h"
#include "collision.h"
#include "basic_data_structures.h"

const int RIGID_BODY_MAX_COUNT = 2;
const int CONSTRAINT_MAX_COUNT = 1;

vec3 g_Force;
vec3 g_ForceStart;
bool g_ApplyingForce;
bool g_UseGravity;
bool g_ApplyingTorque;
hull g_CubeHull;

rigid_body g_RigidBodies[RIGID_BODY_MAX_COUNT]; // Indices correspond to entities
fixed_stack<constraint, CONSTRAINT_COUNT> g_Constraints;

#define DYDT_FUNC(name)                                                                            \
  void name(rigid_body RigidBodies[], int RBCount, const constraint Constraints[],                 \
            int ConstraintCount, float t0, float t1)
typedef DYDT_FUNC(dydt_func);

void
ComputeExternalForcesAndTorques(vec3 F[][2], const rigid_body RigidBody[], int Count)
{
  for(int i = 0; i < Count; i++)
  {
    F[i][0] = {};
    F[i][1] = {};

    if(g_UseGravity && RigidBody->RegardGravity)
    {
      F[i][0] += vec3{ 0, -9.81f * RigidBody->Mass, 0 };
    }

    // TempTesting code
    if(g_ApplyingForce)
    {
      F[i][0] += g_Force;
    }
    if(g_ApplyingTorque)
    {
      vec3 Radius = (g_ForceStart + g_Force) - RigidBody->X;
      F[i][1] += Math::Cross(Radius, g_Force);
    }
  }
}
void
FillEpsilonJmapJsp(float Epsilon[], int Jmap[][2], vec3 Jsp[][4], const rigid_body RigidBodies[],
                   int RBCount, const constraint Constraints[], int ConstraintCount)
{
  for(int i = 0; i < ConstraintCount; i++)
  {
    int IndA = Constraints[i].IndA;
    int IndB = Constraints[i].IndB;
    assert(0 <= IndA && IndA < RBCount);
    assert(0 <= IndB && IndB < RBCount);

    // Currently only distance constraints are allowed
    assert(Constraints[i].Type == CONSTRAINT_Distance);
    if(Constraints[i].Type == CONSTRAINT_Distance)
    {
      vec3 d = RigidBodies[IndA].X - RigidBodies[IndB].X;

      float C    = 0.5f * (Math::Dot(d, d) - Constraints[i].L * Constraints[i].L);
      Epsilon[i] = -C;

      Jmap[i][0] = IndA;
      Jmap[i][1] = IndB;

      Jsp[i][0] = -d;
      Jsp[i][1] = {};
      Jsp[i][2] = d;
      Jsp[i][3] = {};
    }
  }
}

void
FillMDiagInvMatrix(mat3 MDiagInv[][2], rigid_body RB[], int RBCount)
{
  for(int i = 0; i < RBCount; i++)
  {
    assert(FloatsEqualByThreshold(Math::Length(RB[i].q), 1.0f, 0.001f));
    RB[i].R = Math::QuatToMat3(RB[i].q);
    RB[i].InertiaInv =
      Math::MulMat3(RB[i].R, Math::MulMat3(RB[i].InertiaBodyInv, Math::Transposed3(RB[i].R)));

    MDiagInv[i][0] = Math::Mat3Scale(1.0f / RB[i].Mass, 1.0f / RB[i].Mass, 1.0f / RB[i].Mass);
    MDiagInv[i][1] = RB[i].InertiaInv;
  }
}

void
FillV1(vec3 V[][2], const rigid_body RigidBodies[], int RBCount)
{
  for(int i = 0; i < RBCount; i++)
  {
    V[i][0] = RigidBodies[i].v;
    V[i][1] = RigidBodies[i].w;
  }
}

DYDT_FUNC(DYDT_PGS)
{
  vec3 Jsp[CONSTRAINT_MAX_COUNT][4];
  int  Jmap[CONSTRAINT_MAX_COUNT][2];

  vec3  V1[RIGID_BODY_MAX_COUNT][2];
  float Epsilon[CONSTRAINT_MAX_COUNT];

  float Lambda[CONSTRAINT_MAX_COUNT];
  mat3  MDiagInv[RIGID_BODY_MAX_COUNT][2];
  vec3  Fext[RIGID_BODY_MAX_COUNT][2];

  float a[CONSTRAINT_MAX_COUNT][CONSTRAINT_MAX_COUNT]; // J*(M^-1)*Jt
  float b[CONSTRAINT_MAX_COUNT]; // epsilon/dt - J*V1/dt - J*(M^-1) - J*(M^-1)*Fext

  FillV1(V1, RigidBodies, RBCount);
  FillEpsilonJmapJsp(Epsilon, Jmap, Jsp, RigidBodies, RBCount, Constraints, ConstraintCount);
  FillMDiagInvMatrix(MDiagInv, RigidBodies, RBCount);
  ComputeExternalForcesAndTorques(Fext, RigidBodies, RBCount);

  const float dt = t1 - t0;
  for(int i = 0; i < ConstraintCount; i++)
  {
    // b
    b[i] = 0;

    b[i] += Epsilon[i] / dt;

    int IndA = Jmap[i][0];
    int IndB = Jmap[i][1];

    b[i] -= (Math::Dot(Jsp[i][0], V1[IndA][0]) + Math::Dot(Jsp[i][1], V1[IndA][1]) +
             Math::Dot(Jsp[i][2], V1[IndB][0]) + Math::Dot(Jsp[i][3], V1[IndB][1])) /
            dt;

    vec3 MInvFext_a_f = Math::MulMat3Vec3(MDiagInv[IndA][0], Fext[IndA][0]);
    vec3 MInvFext_a_t = Math::MulMat3Vec3(MDiagInv[IndA][1], Fext[IndA][1]);
    vec3 MInvFext_b_f = Math::MulMat3Vec3(MDiagInv[IndB][0], Fext[IndB][0]);
    vec3 MInvFext_b_t = Math::MulMat3Vec3(MDiagInv[IndB][1], Fext[IndB][1]);

    b[i] -= Math::Dot(Jsp[i][0], MInvFext_a_f) + Math::Dot(Jsp[i][1], MInvFext_a_t) +
            Math::Dot(Jsp[i][2], MInvFext_b_f) + Math::Dot(Jsp[i][3], MInvFext_b_t);

    // a
    a[0][0] = 0;
    a[0][0] += Math::Dot(Jsp[i][0], Math::MulMat3Vec3(MDiagInv[IndA][0], Jsp[i][0]));
    a[0][0] += Math::Dot(Jsp[i][1], Math::MulMat3Vec3(MDiagInv[IndA][1], Jsp[i][1]));
    a[0][0] += Math::Dot(Jsp[i][2], Math::MulMat3Vec3(MDiagInv[IndB][0], Jsp[i][2]));
    a[0][0] += Math::Dot(Jsp[i][3], Math::MulMat3Vec3(MDiagInv[IndB][1], Jsp[i][3]));
  }
}

void
ODE(rigid_body RigidBodies[], int RBCount, const constraint Constraints[], int ConstraintCount,
    float t1, float t2, dydt_func dydt)
{
  dydt(RigidBodies, RBCount, Constraints, ConstraintCount, t1, t2);

  const float dt = t2 - t1;
  // Y(t1) = Y(t0) + dY(t0)
  for(int i = 0; i < RBCount; i++)
  {
    if(0.0001f < Math::Length(RigidBodies[i].q))
    {
      Math::Normalize(&RigidBodies[i].q);
    }
    else
    {
      RigidBodies[i].q = { 1, 0, 0, 0 };
    }
  }
}

void
SimulateDynamics(game_state* GameState)
{
  if(2 <= GameState->EntityCount)
  {
    for(int i = 0; i < RIGID_BODY_MAX_COUNT; i++)
    {
      GameState->Entities[i].RigidBody.Mat4Scale =
        Math::Mat4Scale(GameState->Entities[i].Transform.Scale);

      GameState->Entities[i].RigidBody.Collider =
        GameState->Resources.GetModel(GameState->Entities[i].ModelID)->Meshes[0];

      g_RigidBodies[i] = GameState->Entities[i].RigidBody;
    }

    { // Constrainttest
      g_Constraints.Clear();
      constraint TestConstraint = {};
      TestConstraint.Type       = CONSTRAINT_Distance;
      TestConstraint.IndA       = 0;
      TestConstraint.IndB       = 1;
      TestConstraint.L          = 1;
      g_Constraints.Push(TestConstraint);
    }

    ODE(g_RigidBodies, RIGID_BODY_MAX_COUNT, &g_Constraints[0], g_Constraints.Count, 0.0f,
        (FRAME_TIME_MS / 1000.0f), DYDT_PGS);

    for(int i = 0; i < RIGID_BODY_MAX_COUNT; i++)
    {
      GameState->Entities[i].RigidBody             = g_RigidBodies[i];
      GameState->Entities[i].Transform.Rotation    = Math::QuatToEuler(g_RigidBodies[i].q);
      GameState->Entities[i].Transform.Translation = g_RigidBodies[i].X;
    }
  }
}

/*DYDT_FUNC(DYDT_IMPULSE)
{
  velocity    V[RIGID_BODY_MAX_COUNT];
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
    assert(Manifold.PointCount == 1);
    vec3 n     = Manifold.Normal;
    vec3 P     = Manifold.Points[0].Position;
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

      float vBias     = (g_Bias / dt) * MaxFloat(0, Manifold.Points[0].Penetration - g_Slop);
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
    Debug::PushLine(P, P + n * Manifold.Points[0].Penetration, { 0, 1, 0, 1 });
    Debug::PushWireframeSphere(P, 0.05f, { 0, 1, 0, 1 });
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
}*/
