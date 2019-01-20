#include "rigid_body.h"
#include "collision.h"
#include "basic_data_structures.h"

const int RIGID_BODY_MAX_COUNT = 20;
const int CONSTRAINT_MAX_COUNT = 200;

vec3  g_Force;
vec3  g_ForceStart;
bool  g_ApplyingForce;
bool  g_UseGravity;
bool  g_ApplyingTorque;
bool  g_VisualizeFc;
bool  g_VisualizeFcComponents;
bool  g_VisualizeFriction;
float g_Bias;
float g_Mu;
hull  g_CubeHull;

rigid_body g_RigidBodies[RIGID_BODY_MAX_COUNT]; // Indices correspond to entities
fixed_stack<constraint, CONSTRAINT_MAX_COUNT> g_Constraints;

#define DYDT_FUNC(name)                                                                            \
  void name(vec3 Fext[][2], vec3 Fc[][2], rigid_body RigidBodies[], int RBCount,                   \
            const constraint Constraints[], int ConstraintCount, float t0, float t1,               \
            int IterationCount)
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
FillEpsilonJmapJspLambdaMinMaxLambdaDependencies(float Epsilon[], int Jmap[][2], vec3 Jsp[][4],
                                                 float LambdaMinMax[][2], int32_t Dependencies[],
                                                 const rigid_body RigidBodies[], int RBCount,
                                                 const constraint Constraints[],
                                                 int              ConstraintCount)
{
  for(int i = 0; i < ConstraintCount; i++)
  {
    int IndA = Constraints[i].IndA;
    int IndB = Constraints[i].IndB;
    assert(0 <= IndA && IndA < RBCount);

    Dependencies[i] = -1;
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
      float C  = Constraints[i].Penetration;

      Epsilon[i] = -(g_Bias * C);

      Jmap[i][0] = IndA;
      Jmap[i][1] = IndB;

      Jsp[i][0] = -n;
      Jsp[i][1] = -Math::Cross(rA, n);
      Jsp[i][2] = n;
      Jsp[i][3] = Math::Cross(rB, n);
    }
    else if(Constraints[i].Type == CONSTRAINT_Friction)
    {
      LambdaMinMax[i][0] = -INFINITY; //-g_Mu;
      LambdaMinMax[i][1] = INFINITY;  // g_Mu;

      vec3 rA = Constraints[i].BodyRa;
      vec3 rB = Constraints[i].BodyRb;
      vec3 u  = Constraints[i].Tangent;

      Epsilon[i] = 0;

      // Dependencies[i] = Constraints[i].ContactIndex;

      Jmap[i][0] = IndA;
      Jmap[i][1] = IndB;

      Jsp[i][0] = -u;
      Jsp[i][1] = -Math::Cross(rA, u);
      Jsp[i][2] = u;
      Jsp[i][3] = Math::Cross(rB, u);
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

  float   Lambda[CONSTRAINT_MAX_COUNT];
  float   LambdaMinMax[CONSTRAINT_MAX_COUNT][2];
  int32_t LambdaDependencies[CONSTRAINT_MAX_COUNT];
  mat3    MDiagInv[RIGID_BODY_MAX_COUNT][2];

  float a[CONSTRAINT_MAX_COUNT][CONSTRAINT_MAX_COUNT]; // J*(M^-1)*Jt
  float b[CONSTRAINT_MAX_COUNT]; // epsilon/dt - J*V1/dt - J*(M^-1) - J*(M^-1)*Fext

  FillV1(V1, RigidBodies, RBCount);
  FillMDiagInvMatrix(MDiagInv, RigidBodies, RBCount);
  FillEpsilonJmapJspLambdaMinMaxLambdaDependencies(Epsilon, Jmap, Jsp, LambdaMinMax,
                                                   LambdaDependencies, RigidBodies, RBCount,
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

    if(0 <= IndB)
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
      bool MatchAtB = (IndB == IndBj && 0 <= IndB && 0 <= IndBj);
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

  for(int i = 0; i < ConstraintCount; i++)
  {
    Lambda[i] = 0.0f;
  }

  // Solve for lambda (PGS)
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
      if(0.00001f < a[i][i])
      {
        Lambda[i] = (b[i] - Delta) / a[i][i];
      }

      float S = 1.0f;
      if(0 <= LambdaDependencies[i])
      {
        S = Lambda[LambdaDependencies[i]];
      }

      assert(LambdaMinMax[0] < LambdaMinMax[1]);
      Lambda[i] = ClampFloat(LambdaMinMax[i][0] * S, Lambda[i], LambdaMinMax[i][1] * S);
    }
  }

  for(int i = 0; i < RBCount; i++)
  {
    Fc[i][0] = {};
    Fc[i][1] = {};
  }
  for(int i = 0; i < ConstraintCount; i++)
  {
    int IndA = Jmap[i][0];
    int IndB = Jmap[i][1];

    Fc[IndA][0] += Jsp[i][0] * Lambda[i];
    Fc[IndA][1] += Jsp[i][1] * Lambda[i];
    Fc[IndB][0] += Jsp[i][2] * Lambda[i];
    Fc[IndB][1] += Jsp[i][3] * Lambda[i];

    if(g_VisualizeFcComponents)
    {
      vec3 Pa0 = RigidBodies[IndA].X + Constraints[i].BodyRa;
      vec3 Pa1 = Pa0 + Jsp[i][0] * Lambda[i];
      switch(Constraints[i].Type)
      {
        case CONSTRAINT_Contact:
        {
          Debug::PushLine(Pa0, Pa1, { 1, 0, 0.5f, 1 });
          Debug::PushWireframeSphere(Pa1, 0.05f, { 1, 0, 0.5f, 1 });
          break;
        }
        case CONSTRAINT_Friction:
        {
          Debug::PushLine(Pa0, Pa1, { 1, 1, 0, 1 });
          Debug::PushWireframeSphere(Pa1, 0.05f, { 1, 1, 0, 1 });
          break;
        }
      }
    }
  }
}

void
ODE(rigid_body RigidBodies[], int RBCount, const constraint Constraints[], int ConstraintCount,
    float t0, float t1, dydt_func dydt, int32_t IterationCount, bool UpdateState)
{
  vec3 Fext[RIGID_BODY_MAX_COUNT][2];
  vec3 Fc[RIGID_BODY_MAX_COUNT][2];
  dydt(Fext, Fc, RigidBodies, RBCount, Constraints, ConstraintCount, t0, t1, IterationCount);

  const float dt = t1 - t0;
  // Euler step
  for(int i = 0; i < RBCount; i++)
  {
    if(g_VisualizeFc)
    {
      Debug::PushLine(RigidBodies[i].X, RigidBodies[i].X + Fext[i][0], { 0, 0, 1, 1 });
      Debug::PushWireframeSphere(RigidBodies[i].X + Fext[i][0], 0.05f, { 0, 0, 1, 1 });
    }

    if(g_VisualizeFc)
    {
      Debug::PushLine(RigidBodies[i].X, RigidBodies[i].X + Fc[i][0]);
      Debug::PushWireframeSphere(RigidBodies[i].X + Fc[i][0], 0.05f);
    }

    if(UpdateState)
    {
      // update v and w first
      RigidBodies[i].v += dt * (RigidBodies[i].MassInv * (Fext[i][0] + Fc[i][0]));
      RigidBodies[i].w += dt * Math::MulMat3Vec3(RigidBodies[i].InertiaInv, Fext[i][1] + Fc[i][1]);

      // update X and q after
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
}

void
SimulateDynamics(game_state* GameState)
{
  if(1 <= GameState->EntityCount)
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

      /*constraint TestConstraint = {};
      TestConstraint.Type       = CONSTRAINT_Distance;
      TestConstraint.IndA       = 5;
      TestConstraint.IndB       = 6;
      TestConstraint.L          = 0;
      TestConstraint.BodyRa     = { 1, -1, 1 };
      TestConstraint.BodyRb     = { -1, -1, 1 };
      g_Constraints.Push(TestConstraint);
      g_Constraints.Push(TestConstraint);
      TestConstraint.BodyRa = { 1, -1, -1 };
      TestConstraint.BodyRb = { -1, -1, -1 };
      g_Constraints.Push(TestConstraint);
      TestConstraint.IndA   = 6;
      TestConstraint.IndB   = 7;
      TestConstraint.BodyRa = { -1, 1, 1 };
      TestConstraint.BodyRb = { -1, -1, -1 };
      g_Constraints.Push(TestConstraint);
      TestConstraint.IndA   = 6;
      TestConstraint.IndB   = 7;
      TestConstraint.BodyRa = { 1, 1, 1 };
      TestConstraint.BodyRb = { -1, 1, -1 };
      g_Constraints.Push(TestConstraint);
      TestConstraint.Type   = CONSTRAINT_Point;
      TestConstraint.P      = {};
      TestConstraint.L      = 0;
      TestConstraint.IndA   = 0;
      g_Constraints.Push(TestConstraint);*/

      for(int i = 0; i < GameState->EntityCount; i++)
      {
        for(int j = 0; j < i; j++)
        {

          Anim::transform ATransform = GameState->Entities[i].Transform;
          mat4            TransformA = Math::MulMat4(Math::Mat4Translate(g_RigidBodies[i].X),
                                          Math::MulMat4(Math::Mat3ToMat4(g_RigidBodies[i].R),
                                                        g_RigidBodies[i].Mat4Scale));
          Anim::transform BTransform = GameState->Entities[j].Transform;
          mat4            TransformB = Math::MulMat4(Math::Mat4Translate(g_RigidBodies[j].X),
                                          Math::MulMat4(Math::Mat3ToMat4(g_RigidBodies[j].R),
                                                        g_RigidBodies[j].Mat4Scale));
          /*Math::PrintMat4(Math::MulMat4(Math::Mat4Translate(g_RigidBodies[1].X),
                                        Math::MulMat4(Math::Mat3ToMat4(g_RigidBodies[1].R),
                                                      g_RigidBodies[1].Mat4Scale)));
          Math::PrintMat4(Math::MulMat4(Math::Mat4Translate(g_RigidBodies[1].X),
                                        Math::MulMat4(g_RigidBodies[1].Mat4Scale,
                                                      Math::Mat3ToMat4(g_RigidBodies[1].R))));
          */
          /*
          mat4 RotationMat = Math::Mat4Rotate(ATransform.Rotation);
          printf("\n1. Euler\n");
          Math::PrintMat4(RotationMat);
          printf("\n2. Quat\n");
          Math::PrintMat4(Math::Mat3ToMat4(Math::Mat4ToMat3(RotationMat)));

          printf("\n1. T * R * S\n");
          Math::PrintMat4(Math::MulMat4(Math::Mat4Translate(ATransform.Translation),
                                        Math::MulMat4(Math::Mat4Rotate(ATransform.Rotation),
                                                      Math::Mat4Scale(ATransform.Scale))));

          printf("\n2. T * S * R\n");
          Math::PrintMat4(Math::MulMat4(Math::Mat4Translate(ATransform.Translation),
                                        Math::MulMat4(Math::Mat4Scale(ATransform.Scale),
                                                      Math::Mat4Rotate(ATransform.Rotation))));
          */
          sat_contact_manifold Manifold;
          if(SAT(&Manifold, TransformA, &g_CubeHull, TransformB, &g_CubeHull))
          {
            constraint Constraint;
            Constraint.Type = CONSTRAINT_Contact;
            for(int c = 0; c < Manifold.PointCount; ++c)
            {
              // Constraint
              assert(Manifold.Points[c].Penetration < 0);
              vec3 P                 = Manifold.Points[c].Position;
              Constraint.Penetration = Manifold.Points[c].Penetration;
              Constraint.n           = Manifold.Normal;

              if(Manifold.NormalFromA)
              {
                Constraint.IndA = i;
                Constraint.IndB = j;
              }
              else
              {

                Constraint.IndA = j;
                Constraint.IndB = i;
              }

              Constraint.BodyRa = P - g_RigidBodies[Constraint.IndA].X;
              Constraint.BodyRb = (P - Manifold.Points[c].Penetration * Constraint.n) -
                                  g_RigidBodies[Constraint.IndB].X;

              int32_t ContactIndex = g_Constraints.Count;
              g_Constraints.Push(Constraint);

              // Friction
              if(GameState->SimulateFriction)
              {
                vec3 U1 =
                  Math::Normalized(Math::Cross(Manifold.Normal, Manifold.Normal + vec3{ 1, 1, 1 }));
                vec3 U2 = Math::Normalized(Math::Cross(Manifold.Normal, U1));

                Constraint.Type         = CONSTRAINT_Friction;
                Constraint.ContactIndex = ContactIndex;

                Constraint.Tangent = U1;
                g_Constraints.Push(Constraint);

                if(g_VisualizeFriction)
                {
                  Debug::PushLine(P, P + Constraint.Tangent, { 0, 0.7f, 0, 1 });
                  Debug::PushWireframeSphere(P + Constraint.Tangent, 0.05f, { 0, 0.7f, 0, 1 });
                }

                Constraint.Tangent = U2;
                g_Constraints.Push(Constraint);

                if(g_VisualizeFriction)
                {
                  Debug::PushLine(P, P + Constraint.Tangent, { 0, 7, 0, 1 });
                  Debug::PushWireframeSphere(P + Constraint.Tangent, 0.05f, { 0, 0.7f, 0, 1 });
                }
              }
            }
          }
        }
      }
    }

    bool UpdateState = (GameState->SimulateDynamics ||
                        (!GameState->SimulateDynamics && GameState->PerformDynamicsStep));
    ODE(g_RigidBodies, BodyCount, g_Constraints.Elements, g_Constraints.Count, 0.0f,
        (FRAME_TIME_MS / 1000.0f), DYDT_PGS, GameState->PGSIterationCount, UpdateState);

    for(int i = 0; i < BodyCount; i++)
    {
      GameState->Entities[i].RigidBody             = g_RigidBodies[i];
      GameState->Entities[i].Transform.Rotation    = Math::QuatToEuler(g_RigidBodies[i].q);
      GameState->Entities[i].Transform.Translation = g_RigidBodies[i].X;
    }
  }
}
