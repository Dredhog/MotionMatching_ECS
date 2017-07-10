#include "collision.h"

vec3
Support(Render::mesh* Mesh, vec3 Direction, mat4 ModelMatrix)
{
  vec3 Transformed =
    Math::Vec4ToVec3(Math::MulMat4Vec4(ModelMatrix, Math::Vec4(Mesh->Vertices[0].Position, 1.0f)));

  float   Max   = Math::Dot(Transformed, Direction);
  int32_t Index = 0;

  float DotProduct;

  for(int i = 1; i < Mesh->VerticeCount; i++)
  {
    Transformed = Math::Vec4ToVec3(
      Math::MulMat4Vec4(ModelMatrix, Math::Vec4(Mesh->Vertices[i].Position, 1.0f)));
    DotProduct = Math::Dot(Transformed, Direction);
    if(DotProduct > Max)
    {
      Max   = DotProduct;
      Index = i;
    }
  }

  return Math::Vec4ToVec3(
    Math::MulMat4Vec4(ModelMatrix, Math::Vec4(Mesh->Vertices[Index].Position, 1.0f)));
}

void
DoSimplex1(contact_point* Simplex, vec3* Direction)
{
  vec3 AO = -Simplex[1].P;
  vec3 AB = Simplex[0].P - Simplex[1].P;

  *Direction = Math::Cross(Math::Cross(AB, AO), AB);
}

void
DoSimplex2(contact_point* Simplex, int32_t* SimplexOrder, vec3* Direction)
{
  vec3 AO  = -Simplex[2].P;
  vec3 AB  = Simplex[1].P - Simplex[2].P;
  vec3 AC  = Simplex[0].P - Simplex[2].P;
  vec3 ABC = Math::Cross(AB, AC);

  if(Math::Dot(Math::Cross(ABC, AC), AO) > 0)
  {
    Simplex[1]    = Simplex[2];
    *SimplexOrder = 1;
    *Direction    = Math::Cross(Math::Cross(AC, AO), AC);
  }
  else if(Math::Dot(Math::Cross(AB, ABC), AO) > 0)
  {
    Simplex[0]    = Simplex[1];
    Simplex[1]    = Simplex[2];
    *SimplexOrder = 1;
    *Direction    = Math::Cross(Math::Cross(AB, AO), AB);
  }
  else
  {
    if(Math::Dot(ABC, AO) > 0)
    {
      *Direction = ABC;
    }
    else
    {
      contact_point Temp = Simplex[0];
      Simplex[0]         = Simplex[1];
      Simplex[1]         = Temp;
      *Direction         = -ABC;
    }
  }
}

bool
DoSimplex3(contact_point* Simplex, int32_t* SimplexOrder, vec3* Direction)
{
  vec3 AO  = -Simplex[3].P;
  vec3 AB  = Simplex[2].P - Simplex[3].P;
  vec3 AC  = Simplex[1].P - Simplex[3].P;
  vec3 AD  = Simplex[0].P - Simplex[3].P;
  vec3 ABC = Math::Cross(AB, AC);
  vec3 ADB = Math::Cross(AD, AB);
  vec3 ACD = Math::Cross(AC, AD);

  // Counter-clockwise
  if(Math::Dot(ABC, AO) > 0)
  {
    if(Math::Dot(Math::Cross(ABC, AC), AO) > 0)
    {
      // AC region
      Simplex[0]    = Simplex[1];
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AC, AO), AC);
      return false;
    }
    else if(Math::Dot(Math::Cross(AB, ABC), AO) > 0)
    {
      // AB region
      Simplex[0]    = Simplex[2];
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AB, AO), AB);
      return false;
    }
    // ABC region
    Simplex[0]    = Simplex[1]; // C -> 0
    Simplex[1]    = Simplex[2]; // B -> 1
    Simplex[2]    = Simplex[3]; // A -> 2
    *SimplexOrder = 2;
    *Direction    = ABC;
    return false;
  }
  else if(Math::Dot(ACD, AO) > 0)
  {
    if(Math::Dot(Math::Cross(ACD, AD), AO) > 0)
    {
      // AD region
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AD, AO), AD);
      return false;
    }
    else if(Math::Dot(Math::Cross(AC, ACD), AO) > 0)
    {
      // AC region
      Simplex[0]    = Simplex[1];
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AC, AO), AC);
      return false;
    }
    // ACD region
    Simplex[0]    = Simplex[0]; // D -> 0
    Simplex[1]    = Simplex[1]; // C -> 1
    Simplex[2]    = Simplex[3]; // A ->2
    *SimplexOrder = 2;
    *Direction    = ACD;
    return false;
  }
  else if(Math::Dot(ADB, AO) > 0)
  {
    if(Math::Dot(Math::Cross(ADB, AB), AO) > 0)
    {
      // AB region
      Simplex[0]    = Simplex[2];
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AB, AO), AB);
      return false;
    }
    else if(Math::Dot(Math::Cross(AD, ADB), AO) > 0)
    {
      // AD region
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AD, AO), AD);
      return false;
    }
    // ADB region
    Simplex[1]    = Simplex[0]; // D -> 1
    Simplex[0]    = Simplex[2]; // B -> 0
    Simplex[2]    = Simplex[3]; // A -> 2
    *SimplexOrder = 2;
    *Direction    = ADB;
    return false;
  }
  return true;
}

bool
GJK(contact_point* Simplex, int32_t* SimplexOrder, Render::mesh* MeshA, Render::mesh* MeshB,
    mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount, int32_t* FoundInIterations,
    vec3* Direction)
{
  vec3 TransformedA = Math::Vec4ToVec3(
    Math::MulMat4Vec4(ModelAMatrix, Math::Vec4(MeshA->Vertices[0].Position, 1.0f)));
  vec3 TransformedB = Math::Vec4ToVec3(
    Math::MulMat4Vec4(ModelBMatrix, Math::Vec4(MeshB->Vertices[0].Position, 1.0f)));
  Simplex[0].P        = TransformedA - TransformedB;
  Simplex[0].SupportA = TransformedA;
  *Direction          = -Simplex[0].P;
  *SimplexOrder       = 0;

  for(int i = 0; i < IterationCount; i++)
  {
    vec3 SupportA = Support(MeshA, *Direction, ModelAMatrix);
    vec3 SupportB = Support(MeshB, -*Direction, ModelBMatrix);
    vec3 A        = SupportA - SupportB;
    if(Math::Dot(A, *Direction) < 0)
    {
      return false;
    }

    ++(*SimplexOrder);
    Simplex[*SimplexOrder].P        = A;
    Simplex[*SimplexOrder].SupportA = SupportA;

    i++;
    if(IterationCount <= i)
    {
      break;
    }

    switch(*SimplexOrder)
    {
      case 1:
      {
        DoSimplex1(Simplex, Direction);
      }
      break;
      case 2:
      {
        DoSimplex2(Simplex, SimplexOrder, Direction);
      }
      break;
      case 3:
      {
        if(DoSimplex3(Simplex, SimplexOrder, Direction))
        {
          *FoundInIterations = i;
          return true;
        }
      }
      break;
    }
  }
  return false;
}

struct edge
{
  contact_point A;
  contact_point B;
};

struct triangle
{
  contact_point A;
  contact_point B;
  contact_point C;

  vec3 Normal;
};

void
GeneratePolytopeFrom3Simplex(triangle* Polytope, int32_t* TriangleCount, contact_point* Simplex)
{
  // ABC
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[2];
  Polytope[*TriangleCount].C = Simplex[1];

  Polytope[*TriangleCount].Normal =
    Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P,
                Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P);

  ++(*TriangleCount);

  // ACD
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[1];
  Polytope[*TriangleCount].C = Simplex[0];

  Polytope[*TriangleCount].Normal =
    Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P,
                Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P);
  ++(*TriangleCount);

  // ADB
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[0];
  Polytope[*TriangleCount].C = Simplex[2];

  Polytope[*TriangleCount].Normal =
    Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P,
                Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P);

  ++(*TriangleCount);

  // BDC
  Polytope[*TriangleCount].A = Simplex[2];
  Polytope[*TriangleCount].B = Simplex[0];
  Polytope[*TriangleCount].C = Simplex[1];

  Polytope[*TriangleCount].Normal =
    Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P,
                Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P);

  ++(*TriangleCount);
}

int32_t
FindEdge(edge* Edges, int32_t EdgeCount, contact_point A, contact_point B)
{
  for(int i = 0; i < EdgeCount; i++)
  {
    if((A.P == Edges[i].B.P) && (B.P == Edges[i].A.P))
    {
      return i;
    }
  }
  return -1;
}

void
BarycentricCoordinates(float* U, float* V, float* W, vec3 P, vec3 A, vec3 B, vec3 C)
{
  vec3 V0 = B - A;
  vec3 V1 = C - A;
  vec3 V2 = P - A;

  float D00 = Math::Dot(V0, V0);
  float D01 = Math::Dot(V0, V1);
  float D11 = Math::Dot(V1, V1);
  float D20 = Math::Dot(V2, V0);
  float D21 = Math::Dot(V2, V1);

  float Denominator = D00 * D11 - D01 * D01;

  *V = (D11 * D20 - D01 * D21) / Denominator;
  *W = (D00 * D21 - D01 * D20) / Denominator;
  *U = 1.0f - *V - *W;
}

vec3
EPA(vec3* CollisionPoint, contact_point* Simplex, Render::mesh* MeshA, Render::mesh* MeshB,
    mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount)
{
  vec3 Result;

  float MinThreshold = 0.0001f;

  triangle Polytope[100];
  int32_t  TriangleCount = 0;

  GeneratePolytopeFrom3Simplex(Polytope, &TriangleCount, Simplex);

  for(int Iteration = 0; Iteration < IterationCount; Iteration++)
  {
    if(Iteration == IterationCount - 1)
    {
      for(int i = 0; i < TriangleCount; i++)
      {
        Debug::PushLine(Polytope[i].A.P, Polytope[i].B.P, { 0, 0, 1, 1 });
        Debug::PushLine(Polytope[i].B.P, Polytope[i].C.P, { 0, 0, 1, 1 });
        Debug::PushLine(Polytope[i].C.P, Polytope[i].A.P, { 0, 0, 1, 1 });
        vec3 NormalStart =
          0.33f * Polytope[i].A.P + 0.33f * Polytope[i].B.P + 0.33f * Polytope[i].C.P;
        vec3 NormalEnd = NormalStart + Math::Normalized(Polytope[i].Normal);
        Debug::PushLine(NormalStart, NormalEnd, { 1, 0, 1, 1 });
        Debug::PushWireframeSphere(NormalEnd, 0.05f);
      }
    }
    int32_t TriangleIndex = 0;
    float   MinDistance   = -Math::Dot(-Polytope[0].A.P, Math::Normalized(Polytope[0].Normal));

    for(int i = 1; i < TriangleCount; i++)
    {
      float CurrentDistance = -Math::Dot(-Polytope[i].A.P, Math::Normalized(Polytope[i].Normal));
      if(CurrentDistance < MinDistance)
      {
        MinDistance   = CurrentDistance;
        TriangleIndex = i;
      }
    }

    vec3 SupportA = Support(MeshA, Polytope[TriangleIndex].Normal, ModelAMatrix);
    vec3 SupportB = Support(MeshB, -Polytope[TriangleIndex].Normal, ModelBMatrix);
    vec3 NewPoint = SupportA - SupportB;

    Result = Math::Normalized(Polytope[TriangleIndex].Normal) *
             Math::Dot(NewPoint, Math::Normalized(Polytope[TriangleIndex].Normal));
    if(Iteration == IterationCount - 1)
    {
      Debug::PushLine({}, MinDistance * Math::Normalized(Polytope[TriangleIndex].Normal),
                      { 1, 1, 0, 1 });
      Debug::PushWireframeSphere(NewPoint, 0.05f, { 0, 1, 0, 1 });
    }
    if(Math::Length(Result) <= MinDistance + MinThreshold)
    {
      float U, V, W;

      BarycentricCoordinates(&U, &V, &W, NewPoint, Polytope[TriangleIndex].A.P,
                             Polytope[TriangleIndex].B.P, Polytope[TriangleIndex].C.P);

      *CollisionPoint = U * Polytope[TriangleIndex].A.SupportA +
                        V * Polytope[TriangleIndex].B.SupportA +
                        W * Polytope[TriangleIndex].C.SupportA;

      Debug::PushLine(*CollisionPoint, *CollisionPoint - Result, { 0, 1, 0, 1 });
      Debug::PushWireframeSphere(*CollisionPoint - Result, 0.05f, { 0, 1, 0, 1 });

      return -Result;
    }

    edge    Edges[100];
    int32_t EdgeCount = 0;

    Iteration++;

    if(Iteration < IterationCount)
    {
      for(int i = 0; i < TriangleCount; i++)
      {
        float CurrentDistance = Math::Dot(Polytope[i].A.P, Math::Normalized(Polytope[i].Normal));
        if(CurrentDistance <= Math::Dot(Math::Normalized(Polytope[i].Normal), NewPoint))
        {
          if(Iteration == IterationCount - 1)
          {
            vec3 NormalStart =
              0.33f * Polytope[i].A.P + 0.33f * Polytope[i].B.P + 0.33f * Polytope[i].C.P;
            vec3 NormalEnd = NormalStart + Math::Normalized(Polytope[i].Normal);
            Debug::PushLine(NormalStart, NormalEnd, { 0, 1, 0, 1 });
            Debug::PushWireframeSphere(NormalEnd, 0.05f);
          }
          int32_t EdgeIndex = FindEdge(Edges, EdgeCount, Polytope[i].A, Polytope[i].B);
          if(EdgeIndex != -1)
          {
            for(int k = EdgeIndex; k < EdgeCount - 1; k++)
            {
              Edges[k] = Edges[k + 1];
            }
            --EdgeCount;
          }
          else
          {
            Edges[EdgeCount].A = Polytope[i].A;
            Edges[EdgeCount].B = Polytope[i].B;
            ++EdgeCount;
          }

          EdgeIndex = FindEdge(Edges, EdgeCount, Polytope[i].B, Polytope[i].C);
          if(EdgeIndex != -1)
          {
            for(int k = EdgeIndex; k < EdgeCount - 1; k++)
            {
              Edges[k] = Edges[k + 1];
            }
            --EdgeCount;
          }
          else
          {
            Edges[EdgeCount].A = Polytope[i].B;
            Edges[EdgeCount].B = Polytope[i].C;
            ++EdgeCount;
          }

          EdgeIndex = FindEdge(Edges, EdgeCount, Polytope[i].C, Polytope[i].A);
          if(EdgeIndex != -1)
          {
            for(int k = EdgeIndex; k < EdgeCount - 1; k++)
            {
              Edges[k] = Edges[k + 1];
            }
            --EdgeCount;
          }
          else
          {
            Edges[EdgeCount].A = Polytope[i].C;
            Edges[EdgeCount].B = Polytope[i].A;
            ++EdgeCount;
          }

          // Remove face at i
          for(int j = i; j < TriangleCount - 1; j++)
          {
            Polytope[j] = Polytope[j + 1];
          }
          --i;
          --TriangleCount;
        }
      }
    }

    for(int i = 0; i < EdgeCount; i++)
    {
      Polytope[TriangleCount].A          = Edges[i].A;
      Polytope[TriangleCount].B          = Edges[i].B;
      Polytope[TriangleCount].C.P        = NewPoint;
      Polytope[TriangleCount].C.SupportA = SupportA;
      Polytope[TriangleCount].Normal =
        Math::Cross(Polytope[TriangleCount].B.P - Polytope[TriangleCount].A.P,
                    Polytope[TriangleCount].C.P - Polytope[TriangleCount].A.P);

      ++TriangleCount;
    }
  }
  return {};
}
