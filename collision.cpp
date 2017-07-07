#include "collision.h"

void
Support(vec3* PointA, vec3* PointB, Render::mesh* MeshA, Render::mesh* MeshB, vec3 Direction)
{
  float   MaxA   = Math::Dot(MeshA->Vertices[0].Position, Direction);
  float   MinB   = Math::Dot(MeshB->Vertices[0].Position, Direction);
  int32_t IndexA = 0;
  int32_t IndexB = 0;

  float DotProduct;

  for(int i = 1; i < MeshA->VerticeCount; i++)
  {
    DotProduct = Math::Dot(MeshA->Vertices[i].Position, Direction);
    if(DotProduct > MaxA)
    {
      MaxA   = DotProduct;
      IndexA = i;
    }
  }

  for(int i = 1; i < MeshB->VerticeCount; i++)
  {
    DotProduct = Math::Dot(MeshB->Vertices[i].Position, Direction);
    if(DotProduct < MinB)
    {
      MinB   = DotProduct;
      IndexB = i;
    }
  }

  *PointA = MeshA->Vertices[IndexA].Position;
  *PointB = MeshB->Vertices[IndexB].Position;
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
    Simplex[0]    = Simplex[1];
    Simplex[1]    = Simplex[2];
    Simplex[2]    = Simplex[3];
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
    Simplex[2]    = Simplex[3];
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
    Simplex[1]    = Simplex[2];
    Simplex[2]    = Simplex[3];
    *SimplexOrder = 2;
    *Direction    = ADB;
    return false;
  }
  return true;
}

bool
GJK(contact_point* Simplex, Render::mesh* MeshA, Render::mesh* MeshB)
{
  Simplex[0].P         = MeshA->Vertices[0].Position - MeshB->Vertices[0].Position;
  Simplex[0].SupportA  = MeshA->Vertices[0].Position;
  vec3    Direction    = -Simplex[0].P;
  int32_t SimplexOrder = 0;

  for(;;)
  {
    vec3 SupportA, SupportB;
    Support(&SupportA, &SupportB, MeshA, MeshB, Direction);
    vec3 A = SupportA - SupportB;
    if(Math::Dot(A, Direction) < 0)
    {
      return false;
    }

    ++SimplexOrder;
    Simplex[SimplexOrder].P        = A;
    Simplex[SimplexOrder].SupportA = SupportA;

    switch(SimplexOrder)
    {
      case 1:
      {
        DoSimplex1(Simplex, &Direction);
      }
      break;
      case 2:
      {
        DoSimplex2(Simplex, &SimplexOrder, &Direction);
      }
      break;
      case 3:
      {
        if(DoSimplex3(Simplex, &SimplexOrder, &Direction))
        {
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

  Polytope[*TriangleCount].Normal = Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P, Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P);

  ++(*TriangleCount);

  // ACD
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[1];
  Polytope[*TriangleCount].C = Simplex[0];

  Polytope[*TriangleCount].Normal = Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P, Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P);
  ++(*TriangleCount);

  // ADB
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[0];
  Polytope[*TriangleCount].C = Simplex[2];

  Polytope[*TriangleCount].Normal = Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P, Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P);

  ++(*TriangleCount);

  // BDC
  Polytope[*TriangleCount].A = Simplex[2];
  Polytope[*TriangleCount].B = Simplex[0];
  Polytope[*TriangleCount].C = Simplex[1];

  Polytope[*TriangleCount].Normal = Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P, Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P);

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

#define DEBUG_COLLISION 1

void
EPA(vec3* SolutionVector, vec3* CollisionPoint, contact_point* Simplex, Render::mesh* MeshA, Render::mesh* MeshB)
{
  vec3 Result;

  float MinThreshold = 0.0001f;

  triangle Polytope[100];
  int32_t  TriangleCount = 0;

  GeneratePolytopeFrom3Simplex(Polytope, &TriangleCount, Simplex);

  for(int temp = 0; temp < 5; temp++)
  {
    int32_t TriangleIndex = 0;
    float   MinDistance   = -Math::Dot(-Polytope[0].A.P, Math::Normalized(Polytope[0].Normal));
#if DEBUG_COLLISION
    printf("Initial MinDistance = %f\n", MinDistance);
    printf("======================\n");
#endif

    for(int i = 1; i < TriangleCount; i++)
    {
      float CurrentDistance = -Math::Dot(-Polytope[i].A.P, Math::Normalized(Polytope[i].Normal));
      if(CurrentDistance < MinDistance)
      {
        MinDistance   = CurrentDistance;
        TriangleIndex = i;
      }
#if DEBUG_COLLISION
      printf("Iteration %d CurrentDistance = %f\n", i, CurrentDistance);
#endif
    }
#if DEBUG_COLLISION
    printf("======================\n");
    printf("MinDistance TriangleIndex = %d\n", TriangleIndex);
    printf("Polytope[TriangleIndex].A = { %f, %f, %f }\n", Polytope[TriangleIndex].A.P.X, Polytope[TriangleIndex].A.P.Y, Polytope[TriangleIndex].A.P.Z);
    printf("Polytope[TriangleIndex].B = { %f, %f, %f }\n", Polytope[TriangleIndex].B.P.X, Polytope[TriangleIndex].B.P.Y, Polytope[TriangleIndex].B.P.Z);
    printf("Polytope[TriangleIndex].C = { %f, %f, %f }\n", Polytope[TriangleIndex].C.P.X, Polytope[TriangleIndex].C.P.Y, Polytope[TriangleIndex].C.P.Z);
    printf("Polytope[TriangleIndex].Normal = { %f, %f, %f }\n", Polytope[TriangleIndex].Normal.X, Polytope[TriangleIndex].Normal.Y, Polytope[TriangleIndex].Normal.Z);
    printf("MinDistance = %f\n", MinDistance);
    printf("======================\n");
#endif
#if DEBUG_COLLISION
    for(int i = 0; i < TriangleCount; i++)
    {
      printf("Polytope[%d].A = { %f, %f, %f }\n", i, Polytope[i].A.P.X, Polytope[i].A.P.Y, Polytope[i].A.P.Z);
      printf("Polytope[%d].B = { %f, %f, %f }\n", i, Polytope[i].B.P.X, Polytope[i].B.P.Y, Polytope[i].B.P.Z);
      printf("Polytope[%d].C = { %f, %f, %f }\n", i, Polytope[i].C.P.X, Polytope[i].C.P.Y, Polytope[i].C.P.Z);
      printf("Polytope[%d].Normal = { %f, %f, %f }\n", i, Polytope[i].Normal.X, Polytope[i].Normal.Y, Polytope[i].Normal.Z);
      printf("-----------------------\n");
    }
#endif

    vec3 SupportA, SupportB;
    Support(&SupportA, &SupportB, MeshA, MeshB, Polytope[TriangleIndex].Normal);
    vec3 NewPoint = SupportA - SupportB;

    Result = Math::Normalized(Polytope[TriangleIndex].Normal) * Math::Dot(NewPoint, Math::Normalized(Polytope[TriangleIndex].Normal));
#if DEBUG_COLLISION
    printf("NewPoint = { %f, %f, %f }\n", NewPoint.X, NewPoint.Y, NewPoint.Z);
    for(int i = 0; i < TriangleCount; i++)
    {
      if((Polytope[i].A.P == NewPoint) || (Polytope[i].B.P == NewPoint) || (Polytope[i].C.P == NewPoint))
      {
        printf("NewPoint in triangle %d!\n", i);
        break;
      }
    }
    printf("NewPointLength = %f\n", Math::Length(NewPoint));
    printf("NewPointLength = %f\n", -Math::Dot(-NewPoint, Math::Normalized(Polytope[TriangleIndex].Normal)));
    printf("ResultLength = %f\n", Math::Length(Result));
    printf("DIFF = %f\n", Math::Length(Result) - MinDistance);
    printf("======================\n");
#endif
    if(Math::Length(Result) <= MinDistance + MinThreshold)
    {
      *SolutionVector = -Result;

      float U, V, W;

      BarycentricCoordinates(&U, &V, &W, NewPoint, Polytope[TriangleIndex].A.P, Polytope[TriangleIndex].B.P, Polytope[TriangleIndex].C.P);

      *CollisionPoint = U * Polytope[TriangleIndex].A.SupportA + V * Polytope[TriangleIndex].B.SupportA + W * Polytope[TriangleIndex].C.SupportA;
      break;
    }

    edge    Edges[100];
    int32_t EdgeCount = 0;

    for(int i = 0; i < TriangleCount; i++)
    {
#if DEBUG_COLLISION
      if(Math::Dot(Polytope[i].Normal, -Polytope[i].A.P) > 0.0f)
      {
        printf("Triangle %d/%d points in wrong direction!\n", i, TriangleCount);
      }
#endif
      if(Math::Dot(Polytope[i].Normal, NewPoint) > 0.0f)
      {
#if DEBUG_COLLISION
        printf("Removed %d/%d triangle.\n", i, TriangleCount);
#endif
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

    for(int i = 0; i < EdgeCount; i++)
    {
#if DEBUG_COLLISION
#if 1
      printf("%d/%d\n", i, EdgeCount);
      printf("Edges[%d].A.P = { %f, %f, %f }\n", i, Edges[i].A.P.X, Edges[i].A.P.Y, Edges[i].A.P.Z);
      printf("Edges[%d].B.P = { %f, %f, %f }\n", i, Edges[i].B.P.X, Edges[i].B.P.Y, Edges[i].B.P.Z);
      printf("Edges[%d].A.SupportA = { %f, %f, %f }\n", i, Edges[i].A.SupportA.X, Edges[i].A.SupportA.Y, Edges[i].A.SupportA.Z);
      printf("Edges[%d].B.SupportA = { %f, %f, %f }\n", i, Edges[i].B.SupportA.X, Edges[i].B.SupportA.Y, Edges[i].B.SupportA.Z);
#endif
#endif
      Polytope[TriangleCount].A          = Edges[i].A;
      Polytope[TriangleCount].B          = Edges[i].B;
      Polytope[TriangleCount].C.P        = NewPoint;
      Polytope[TriangleCount].C.SupportA = SupportA;
      Polytope[TriangleCount].Normal     = Math::Cross(Polytope[TriangleCount].B.P - Polytope[TriangleCount].A.P, Polytope[TriangleCount].C.P - Polytope[TriangleCount].A.P);

      ++TriangleCount;
    }
#if DEBUG_COLLISION
    printf("======================\n");
    printf("TriangleCount = %d\n", TriangleCount);
#endif
  }
}
