#include "rigid_body.h"
#include "collision.h"
#include "basic_data_structures.h"

const int RIGID_BODY_MAX_COUNT = 10;
const int CONSTRAINT_MAX_COUNT = 30;

vec3  g_Force;
vec3  g_ForceStart;
bool  g_ApplyingForce;
bool  g_UseGravity;
bool  g_ApplyingTorque;
bool  g_VisualizeFc;
float g_Bias;
hull  g_CubeHull;

rigid_body g_RigidBodies[RIGID_BODY_MAX_COUNT]; // Indices correspond to entities
fixed_stack<constraint, CONSTRAINT_MAX_COUNT> g_Constraints;

#define DYDT_FUNC(name)                                                                            \
  void name(rigid_body RigidBodies[], int RBCount, const constraint Constraints[],                 \
            int ConstraintCount, float t0, float t1, int IterationCount)
typedef DYDT_FUNC(dydt_func);

void
ComputeExternalForcesAndTorques(vec3 F[][2], const rigid_body RigidBodies[], int RBCount)
{
  for(int i = 0; i < RBCount; i++)
  {
    F[i][0] = {};
    F[i][1] = {};

    if(g_UseGravity && RigidBodies[i].RegardGravity)
    {
      F[i][0] += vec3{ 0, -9.81f * RigidBodies[i].Mass, 0 };
    }

    // TempTesting code
    if(g_ApplyingForce)
    {
      F[i][0] += g_Force;
    }
    if(g_ApplyingTorque)
    {
      vec3 Radius = (g_ForceStart + g_Force) - RigidBodies[i].X;
      F[i][1] += Math::Cross(Radius, g_Force);
    }
  }
}
void
FillEpsilonJmapJspLambdaMinMax(float Epsilon[], int Jmap[][2], vec3 Jsp[][4],
                               float LambdaMinMax[][2], const rigid_body RigidBodies[], int RBCount,
                               const constraint Constraints[], int ConstraintCount)
{
  for(int i = 0; i < ConstraintCount; i++)
  {
    int IndA = Constraints[i].IndA;
    int IndB = Constraints[i].IndB;
    assert(0 <= IndA && IndA < RBCount);
    assert(IndA < IndB || IndB < 0);

    assert(0 <= Constraints[i].Type && Constraints[i].Type < CONSTRAINT_Count);
    if(Constraints[i].Type == CONSTRAINT_Distance)
    {
      LambdaMinMax[i][0] = -INFINITY;
      LambdaMinMax[i][1] = INFINITY;

      vec3 rA = Math::MulMat3Vec3(RigidBodies[IndA].R, Constraints[i].BodyRa);
      vec3 rB = Math::MulMat3Vec3(RigidBodies[IndB].R, Constraints[i].BodyRb);
      vec3 d  = RigidBodies[IndB].X + rB - RigidBodies[IndA].X - rA;

      float C    = 0.5f * (Math::Dot(d, d) - Constraints[i].L * Constraints[i].L);
      Epsilon[i] = -(g_Bias * C);

      Jmap[i][0] = IndA;
      Jmap[i][1] = IndB;

      Jsp[i][0] = -d;
      Jsp[i][1] = -Math::Cross(rA, d);
      Jsp[i][2] = d;
      Jsp[i][3] = Math::Cross(rB, d);
    }
    else if(Constraints[i].Type == CONSTRAINT_Point)
    {
      LambdaMinMax[i][0] = -INFINITY;
      LambdaMinMax[i][1] = INFINITY;

      vec3 rA = Math::MulMat3Vec3(RigidBodies[IndA].R, Constraints[i].BodyRa);
      vec3 d  = Constraints[i].P - (RigidBodies[IndA].X + rA);

      float C    = 0.5f * (Math::Dot(d, d) - Constraints[i].L * Constraints[i].L);
      Epsilon[i] = -(g_Bias * C);

      Jmap[i][0] = IndA;
      Jmap[i][1] = -1;

      Jsp[i][0] = -d;
      Jsp[i][1] = -Math::Cross(rA, d);
      Jsp[i][2] = {};
      Jsp[i][3] = {};
    }
    else if(Constraints[i].Type == CONSTRAINT_Contact)
    {
      LambdaMinMax[i][0] = 0;
      LambdaMinMax[i][1] = INFINITY;

      vec3  rA = Constraints[i].BodyRa;
      vec3  rB = Constraints[i].BodyRb;
      vec3  n  = Constraints[i].n;
      float C  = -Constraints[i].Penetration;

      Epsilon[i] = -(g_Bias * C);

      Jmap[i][0] = IndA;
      Jmap[i][1] = IndB;

      Jsp[i][0] = -n;
      Jsp[i][1] = -Math::Cross(rA, n);
      Jsp[i][2] = n;
      Jsp[i][3] = Math::Cross(rB, n);
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

    MDiagInv[i][0] = Math::Mat3Scale(RB[i].MassInv, RB[i].MassInv, RB[i].MassInv);
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
  float LambdaMinMax[CONSTRAINT_MAX_COUNT][2];
  mat3  MDiagInv[RIGID_BODY_MAX_COUNT][2];
  vec3  Fext[RIGID_BODY_MAX_COUNT][2];
  vec3  dtFc[RIGID_BODY_MAX_COUNT][2];

  float a[CONSTRAINT_MAX_COUNT][CONSTRAINT_MAX_COUNT]; // J*(M^-1)*Jt
  float b[CONSTRAINT_MAX_COUNT]; // epsilon/dt - J*V1/dt - J*(M^-1) - J*(M^-1)*Fext

  FillV1(V1, RigidBodies, RBCount);
  FillMDiagInvMatrix(MDiagInv, RigidBodies, RBCount);
  FillEpsilonJmapJspLambdaMinMax(Epsilon, Jmap, Jsp, LambdaMinMax, RigidBodies, RBCount,
                                 Constraints, ConstraintCount);
  ComputeExternalForcesAndTorques(Fext, RigidBodies, RBCount);

  const float dt = t1 - t0;
  for(int i = 0; i < ConstraintCount; i++)
  {
    // b
    b[i] = 0;

    b[i] += Epsilon[i] / dt;

    int IndA = Jmap[i][0];
    int IndB = Jmap[i][1];

    b[i] -= (Math::Dot(Jsp[i][0], V1[IndA][0]) + Math::Dot(Jsp[i][1], V1[IndA][1])) / dt;

    vec3 MInvFext_a_f = Math::MulMat3Vec3(MDiagInv[IndA][0], Fext[IndA][0]);
    vec3 MInvFext_a_t = Math::MulMat3Vec3(MDiagInv[IndA][1], Fext[IndA][1]);

    b[i] -= Math::Dot(Jsp[i][0], MInvFext_a_f) + Math::Dot(Jsp[i][1], MInvFext_a_t);

    if(0 < IndB)
    {
      b[i] -= (Math::Dot(Jsp[i][2], V1[IndB][0]) + Math::Dot(Jsp[i][3], V1[IndB][1])) / dt;
      vec3 MInvFext_b_f = Math::MulMat3Vec3(MDiagInv[IndB][0], Fext[IndB][0]);
      vec3 MInvFext_b_t = Math::MulMat3Vec3(MDiagInv[IndB][1], Fext[IndB][1]);
      b[i] -= Math::Dot(Jsp[i][2], MInvFext_b_f) + Math::Dot(Jsp[i][3], MInvFext_b_t);
    }

    // a
    for(int j = 0; j < ConstraintCount; j++)
    {
      int  IndAj    = Jmap[j][0];
      int  IndBj    = Jmap[j][1];
      bool MatchAtA = (IndA == IndAj);
      bool MatchAtB = (IndB == IndBj && 0 < IndB && 0 < IndBj);
      a[i][j]       = 0;
      a[i][j] +=
        MatchAtA ? Math::Dot(Jsp[i][0], Math::MulMat3Vec3(MDiagInv[IndA][0], Jsp[j][0])) : 0;
      a[i][j] +=
        MatchAtA ? Math::Dot(Jsp[i][1], Math::MulMat3Vec3(MDiagInv[IndA][1], Jsp[j][1])) : 0;
      a[i][j] +=
        MatchAtB ? Math::Dot(Jsp[i][2], Math::MulMat3Vec3(MDiagInv[IndB][0], Jsp[j][2])) : 0;
      a[i][j] +=
        MatchAtB ? Math::Dot(Jsp[i][3], Math::MulMat3Vec3(MDiagInv[IndB][1], Jsp[j][3])) : 0;
    }
  }

// Solve for lambda (GS)
#if 1
  for(int k = 0; k < IterationCount; k++)
  {
    for(int i = 0; i < ConstraintCount; i++)
    {
      float Delta = 0.0f;
      {
        for(int j = 0; j < i; j++)
        {
          Delta += a[i][j] * Lambda[j];
        }
        for(int j = i + 1; j < ConstraintCount; j++)
        {
          Delta += a[i][j] * Lambda[j];
        }
      }
      // TODO(Lukas) Remove magic value or other solution
      if(0.0000001f < a[i][i])
      {
        Lambda[i] = (b[i] - Delta) / a[i][i];
      }
      assert(LambdaMinMax[0] < LambdaMinMax[1]);
      if(Lambda[i] < LambdaMinMax[i][0])
      {
        Lambda[i] = LambdaMinMax[i][0];
      }
      if(LambdaMinMax[i][1] < Lambda[i])
      {
        Lambda[i] = LambdaMinMax[i][1];
      }
    }
  }
#else
  assert(ConstraintCount == 1);
  if(ConstraintCount == 1)
  {
    Lambda[0] = b[0] / a[0][0];
  }
#endif

  for(int i = 0; i < RBCount; i++)
  {
    dtFc[i][0] = {};
    dtFc[i][1] = {};
  }
  for(int i = 0; i < ConstraintCount; i++)
  {
    int IndA = Jmap[i][0];
    int IndB = Jmap[i][1];

    dtFc[IndA][0] += Jsp[i][0] * Lambda[i];
    dtFc[IndA][1] += Jsp[i][1] * Lambda[i];
    dtFc[IndB][0] += Jsp[i][2] * Lambda[i];
    dtFc[IndB][1] += Jsp[i][3] * Lambda[i];

    if(g_VisualizeFc)
    {
      Debug::PushLine(RigidBodies[IndA].X, RigidBodies[IndA].X + dtFc[IndA][0]);
      Debug::PushWireframeSphere(RigidBodies[IndA].X + dtFc[IndA][0], 0.05f);
    }
  }

  for(int i = 0; i < RBCount; i++)
  {
    RigidBodies[i].v += dt * (RigidBodies[i].MassInv * (Fext[i][0] + dtFc[i][0]));
    RigidBodies[i].w += dt * Math::MulMat3Vec3(RigidBodies[i].InertiaInv, Fext[i][1] + dtFc[i][1]);
  }
}

void
ODE(rigid_body RigidBodies[], int RBCount, const constraint Constraints[], int ConstraintCount,
    float t0, float t1, dydt_func dydt, int32_t IterationCount)
{
  dydt(RigidBodies, RBCount, Constraints, ConstraintCount, t0, t1, IterationCount);

  const float dt = t1 - t0;
  // Y(t1) = Y(t0) + dY(t0)
  for(int i = 0; i < RBCount; i++)
  {
    RigidBodies[i].X += dt * RigidBodies[i].v;

    quat qOmega = {};
    qOmega.V    = RigidBodies[i].w;
    quat qDot   = 0.5f * (qOmega * RigidBodies[i].q);

    RigidBodies[i].q = RigidBodies[i].q + dt * qDot;
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
    assert(GameState->EntityCount <= RIGID_BODY_MAX_COUNT);
    const int BodyCount = GameState->EntityCount;

    for(int i = 0; i < BodyCount; i++)
    {
      GameState->Entities[i].RigidBody.R =
        Math::Mat4ToMat3(Math::Mat4Rotate(GameState->Entities[i].Transform.Rotation));

      GameState->Entities[i].RigidBody.Mat4Scale =
        Math::Mat4Scale(GameState->Entities[i].Transform.Scale);

      GameState->Entities[i].RigidBody.Collider =
        GameState->Resources.GetModel(GameState->Entities[i].ModelID)->Meshes[0];

      g_RigidBodies[i] = GameState->Entities[i].RigidBody;
    }

    { // Constrainttest
      g_Constraints.Clear();
      /*
      constraint TestConstraint = {};
      TestConstraint.Type       = CONSTRAINT_Distance;
      TestConstraint.IndA       = 0;
      TestConstraint.IndB       = 1;
      TestConstraint.L          = 0;
      TestConstraint.BodyRa     = { 1, -1, 1 };
      TestConstraint.BodyRb     = { -1, -1, 1 };
      g_Constraints.Push(TestConstraint);
      g_Constraints.Push(TestConstraint);
      TestConstraint.BodyRa = { 1, -1, -1 };
      TestConstraint.BodyRb = { -1, -1, -1 };
      g_Constraints.Push(TestConstraint);
      TestConstraint.IndA   = 1;
      TestConstraint.IndB   = 2;
      TestConstraint.BodyRa = { -1, 1, 1 };
      TestConstraint.BodyRb = { -1, -1, -1 };
      g_Constraints.Push(TestConstraint);
      TestConstraint.IndA   = 1;
      TestConstraint.IndB   = 2;
      TestConstraint.BodyRa = { 1, 1, 1 };
      TestConstraint.BodyRb = { -1, 1, -1 };
      g_Constraints.Push(TestConstraint);
      TestConstraint.Type   = CONSTRAINT_Point;
      TestConstraint.P      = {};
      TestConstraint.L      = 0;
      TestConstraint.IndA   = 0;
      TestConstraint.BodyRa = { 0, 1, 0 };
      g_Constraints.Push(TestConstraint);
      */

      mat4 TransformA = Math::MulMat4(Math::Mat4Translate(g_RigidBodies[0].X),
                                      Math::MulMat4(g_RigidBodies[0].Mat4Scale,
                                                    Math::Mat3ToMat4(g_RigidBodies[0].R)));

      mat4 TransformB = Math::MulMat4(Math::Mat4Translate(g_RigidBodies[1].X),
                                      Math::MulMat4(g_RigidBodies[1].Mat4Scale,
                                                    Math::Mat3ToMat4(g_RigidBodies[1].R)));

      sat_contact_manifold Manifold;
      if(SAT(&Manifold, TransformA, &g_CubeHull, TransformB, &g_CubeHull))
      {
        constraint Constraint;
        Constraint.Type = CONSTRAINT_Contact;
        for(int i = 0; i < Manifold.PointCount; ++i)
        {
          assert(0 < Manifold.Points[i].Penetration);
          Constraint.n = Manifold.Normal;
          vec3 P       = TransformVector(Manifold.Points[i].Position, TransformB);
          if(Manfold.BelongsToA)
          {
            Constraint.BodyRa = P - g_RigidBodies[0].X;
            Constraint.BodyRb =
              P + (Manifold.Points[i].Penetration * Manifold.Normal) - g_RigidBodies[1].X;
            Constraint.Penetration = Manifold.Points[i].Penetration;
            Constraint.IndA        = 0;
            Constraint.IndB        = 1;
          }
          else
          {
            Constraint.BodyRa =
              P + (Manifold.Points[i].Penetration * Manifold.Normal) - g_RigidBodies[1].X;
            Constraint.BodyRb = P - g_RigidBodies[0].X;

            Constraint.Penetration = Manifold.Points[i].Penetration;
            Constraint.IndA        = 1;
            Constraint.IndB        = 0;
          }
          g_Constraints.Push(Constraint);

          Debug::PushWireframeSphere(P, 0.05f, { 0, 1, 0, 1 });
        }
      }
    }

    ODE(g_RigidBodies, BodyCount, g_Constraints.Elements, g_Constraints.Count, 0.0f,
        (FRAME_TIME_MS / 1000.0f), DYDT_PGS, GameState->PGSIterationCount);

    for(int i = 0; i < BodyCount; i++)
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
           FloatsEqualByThreshold((g_RigidBodies[i].MassInv * g_RigidBodies[i].Mass), 1.0f,
0.01f)); Y[i].P += dt * (g_RigidBodies[i].MassInv * dY[i].Force); Y[i].L += dt *
Math::MulMat3Vec3(g_RigidBodies[i].InertiaInv, dY[i].Torque);

    // Extract tentative velocity
    V[i].v = g_RigidBodies[i].MassInv * Y[i].P;
    V[i].w = Math::MulMat3Vec3(g_RigidBodies[i].InertiaInv, Y[i].L);
  }

  // Collision impulses
  mat4 TransformA =
    Math::MulMat4(Math::Mat4Translate(Y[0].X),
                  Math::MulMat4(g_RigidBodies[0].Mat4Scale,
Math::Mat3ToMat4(g_RigidBodies[0].R))); mat4 TransformB =
    Math::MulMat4(Math::Mat4Translate(Y[1].X),
                  Math::MulMat4(g_RigidBodies[1].Mat4Scale,
Math::Mat3ToMat4(g_RigidBodies[1].R)));

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
}*/
