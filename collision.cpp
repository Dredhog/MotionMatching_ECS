#include "collision.h"

vec3
Support(Render::mesh* MeshA, Render::mesh* MeshB, vec3 Direction)
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

  return MeshA->Vertices[IndexA].Position - MeshB->Vertices[IndexB].Position;
}

void
DoSimplex1(vec3* Simplex, vec3* Direction)
{
  vec3 AO = -Simplex[1];
  vec3 AB = Simplex[0] - Simplex[1];

  *Direction = Math::Cross(Math::Cross(AB, AO), AB);
}

void
DoSimplex2(vec3* Simplex, int32_t* SimplexOrder, vec3* Direction)
{
  vec3 AO  = -Simplex[2];
  vec3 AB  = Simplex[1] - Simplex[2];
  vec3 AC  = Simplex[0] - Simplex[2];
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
      vec3 Temp  = Simplex[0];
      Simplex[0] = Simplex[1];
      Simplex[1] = Temp;
      *Direction = -ABC;
    }
  }
}

bool
DoSimplex3(vec3* Simplex, int32_t* SimplexOrder, vec3* Direction)
{
  vec3 AO  = -Simplex[3];
  vec3 AB  = Simplex[2] - Simplex[3];
  vec3 AC  = Simplex[1] - Simplex[3];
  vec3 AD  = Simplex[0] - Simplex[3];
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
GJK(vec3* Simplex, Render::mesh* MeshA, Render::mesh* MeshB)
{
  Simplex[0]           = MeshA->Vertices[0].Position - MeshB->Vertices[0].Position;
  vec3    Direction    = -Simplex[0];
  int32_t SimplexOrder = 0;

  for(;;)
  {
    vec3 A = Support(MeshA, MeshB, Direction);
    if(Math::Dot(A, Direction) < 0)
    {
      return false;
    }

    ++SimplexOrder;
    Simplex[SimplexOrder] = A;

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
  vec3 A;
  vec3 B;
};

struct triangle
{
  vec3 A;
  vec3 B;
  vec3 C;
  vec3 Normal;
  edge Edges[3];
};

float
OriginDistanceToTriangle(triangle Triangle)
{
  vec3 AB  = Triangle.A - Triangle.B;
  vec3 AC  = Triangle.A - Triangle.C;
  vec3 ABC = Math::Cross(AB, AC);

  return -Math::Dot(Math::Normalized(ABC), -Triangle.A);
}

void
GeneratePolytope(triangle* Polytope, int32_t* TriangleCount, vec3* Vertices, int32_t VertexCount)
{
  for(int i = 0; i < VertexCount - 2; i++)
  {
    for(int j = i + 1; j < VertexCount - 1; j++)
    {
      for(int k = j + 1; k < VertexCount; k++)
      {
        Polytope[*TriangleCount].A = Vertices[i];
        Polytope[*TriangleCount].B = Vertices[j];
        Polytope[*TriangleCount].C = Vertices[k];
        if(OriginDistanceToTriangle(Polytope[*TriangleCount]) < 0)
        {
          Polytope[*TriangleCount].B = Vertices[k];
          Polytope[*TriangleCount].C = Vertices[j];
        }
        Polytope[*TriangleCount].Normal = Math::Cross(Polytope[*TriangleCount].A - Polytope[*TriangleCount].B, Polytope[*TriangleCount].A - Polytope[*TriangleCount].C);

        Polytope[*TriangleCount].Edges[0].A = Polytope[*TriangleCount].A;
        Polytope[*TriangleCount].Edges[0].B = Polytope[*TriangleCount].B;
        Polytope[*TriangleCount].Edges[1].A = Polytope[*TriangleCount].B;
        Polytope[*TriangleCount].Edges[1].B = Polytope[*TriangleCount].C;
        Polytope[*TriangleCount].Edges[2].A = Polytope[*TriangleCount].C;
        Polytope[*TriangleCount].Edges[2].B = Polytope[*TriangleCount].A;

        ++(*TriangleCount);
      }
    }
  }
}

int32_t
FindEdge(edge* Edges, int32_t EdgeCount, edge TargetEdge)
{
  for(int i = 0; i < EdgeCount; i++)
  {
// TODO(rytis): Figure out why two non-reverse edge scenario is possible
// Comparing with reversed TargetEdge
#if 0
    if((TargetEdge.B == Edges[i].A) && (TargetEdge.A == Edges[i].B))
    {
      return i;
    }
#else
    if(((TargetEdge.A == Edges[i].A) && (TargetEdge.B == Edges[i].B)) || ((TargetEdge.B == Edges[i].A) && (TargetEdge.A == Edges[i].B)))
    {
      return i;
    }
#endif
  }
  return -1;
}

#define PRINT_EDGES 0
#define DEBUG_COLLISION 0

#if DEBUG_COLLISION
#include <stdio.h>
#endif

vec3
EPA(vec3* Simplex, int32_t SimplexLength, Render::mesh* MeshA, Render::mesh* MeshB)
{
  vec3 Result;

  float MinThreshold = 0.0001f;

  triangle Polytope[300];
  int32_t  TriangleCount = 0;

  GeneratePolytope(Polytope, &TriangleCount, Simplex, SimplexLength);

  for(;;)
  {
#if PRINT_EDGES
    printf("=====================\n");
    for(int i = 0; i < 100; i++)
    {
      for(int j = 0; j < 3; j++)
      {
        printf("Triangle[%d].Edges[%d].A = { %f, %f, %f }\n", i, j, Polytope[i].Edges[j].A.X, Polytope[i].Edges[j].A.Y, Polytope[i].Edges[j].A.Z);
        printf("Triangle[%d].Edges[%d].B = { %f, %f, %f }\n", i, j, Polytope[i].Edges[j].B.X, Polytope[i].Edges[j].B.Y, Polytope[i].Edges[j].B.Z);
      }
    }
    printf("=====================\n");
#endif
    int32_t TriangleIndex = 0;
    float   MinDistance   = OriginDistanceToTriangle(Polytope[0]);
#if DEBUG_COLLISION
    printf("=====================\n");
    printf("InitialMinDistance = %f\n", MinDistance);
#endif

    for(int i = 1; i < TriangleCount; i++)
    {
      float CurrentDistance = OriginDistanceToTriangle(Polytope[i]);
      if(CurrentDistance < MinDistance)
      {
        MinDistance   = CurrentDistance;
        TriangleIndex = i;
#if DEBUG_COLLISION
        printf("IterationMinDistance = %f\n", MinDistance);
#endif
      }
    }
#if DEBUG_COLLISION
    printf("Polytope[0].A = { %f, %f, %f }\n", Polytope[0].A.X, Polytope[0].A.Y, Polytope[0].A.Z);
    printf("Polytope[0].B = { %f, %f, %f }\n", Polytope[0].B.X, Polytope[0].B.Y, Polytope[0].B.Z);
    printf("Polytope[0].C = { %f, %f, %f }\n", Polytope[0].C.X, Polytope[0].C.Y, Polytope[0].C.Z);
    printf("TriangleIndex = %d\n", TriangleIndex);
    printf("Polytope[TriangleIndex].A = { %f, %f, %f }\n", Polytope[TriangleIndex].A.X, Polytope[TriangleIndex].A.Y, Polytope[TriangleIndex].A.Z);
    printf("Polytope[TriangleIndex].B = { %f, %f, %f }\n", Polytope[TriangleIndex].B.X, Polytope[TriangleIndex].B.Y, Polytope[TriangleIndex].B.Z);
    printf("Polytope[TriangleIndex].C = { %f, %f, %f }\n", Polytope[TriangleIndex].C.X, Polytope[TriangleIndex].C.Y, Polytope[TriangleIndex].C.Z);
    printf("Polytope[TriangleIndex].Normal = { %f, %f, %f }\n", Polytope[TriangleIndex].Normal.X, Polytope[TriangleIndex].Normal.Y, Polytope[TriangleIndex].Normal.Z);
    printf("FinalMinDistance = %f\n", MinDistance);
#endif

    vec3 NewPoint = Support(MeshA, MeshB, Polytope[TriangleIndex].Normal);

    Result = Math::Normalized(Polytope[TriangleIndex].Normal) * Math::Dot(NewPoint, Math::Normalized(Polytope[TriangleIndex].Normal));
#if DEBUG_COLLISION
    printf("NewPoint = { %f, %f, %f }\n", NewPoint.X, NewPoint.Y, NewPoint.Z);
    printf("NewPointLength = %f\n", Math::Length(NewPoint));
    printf("ResultLength = %f\n", Math::Length(Result));
#endif
    if(Math::Length(Result) <= MinDistance + MinThreshold)
    {
      break;
    }

    edge    Edges[300];
    int32_t EdgeCount = 0;

    for(int i = 0; i < TriangleCount; i++)
    {
      if(Math::Dot(Polytope[i].Normal, NewPoint) > 0)
      {
        for(int j = 0; j < 3; j++)
        {
          int32_t EdgeIndex = FindEdge(Edges, EdgeCount, Polytope[i].Edges[j]);
          if(EdgeIndex != -1)
          {
            // Remove edge at EdgeIndex
            for(int k = EdgeIndex; k < EdgeCount - 1; k++)
            {
              Edges[k] = Edges[k + 1];
            }
            --EdgeCount;
          }
          else
          {
            Edges[EdgeCount++] = Polytope[i].Edges[j];
          }
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
      Polytope[TriangleCount].A = Edges[i].A;
      Polytope[TriangleCount].B = Edges[i].B;
      Polytope[TriangleCount].C = NewPoint;
#if 1
      if(OriginDistanceToTriangle(Polytope[TriangleCount]) < 0)
      {
        Polytope[TriangleCount].B = NewPoint;
        Polytope[TriangleCount].C = Edges[i].B;
      }
#endif
      Polytope[TriangleCount].Normal = Math::Cross(Polytope[TriangleCount].A - Polytope[TriangleCount].B, Polytope[TriangleCount].A - Polytope[TriangleCount].C);

      Polytope[TriangleCount].Edges[0].A = Polytope[TriangleCount].A;
      Polytope[TriangleCount].Edges[0].B = Polytope[TriangleCount].B;
      Polytope[TriangleCount].Edges[1].A = Polytope[TriangleCount].B;
      Polytope[TriangleCount].Edges[1].B = Polytope[TriangleCount].C;
      Polytope[TriangleCount].Edges[2].A = Polytope[TriangleCount].C;
      Polytope[TriangleCount].Edges[2].B = Polytope[TriangleCount].A;

      ++TriangleCount;
    }
#if DEBUG_COLLISION
    printf("TriangleCount = %d\n", TriangleCount);
#endif
  }

  return Result;
}
