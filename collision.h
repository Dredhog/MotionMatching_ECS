#pragma once

#include <stdint.h>
#include <float.h>
#include "linear_math/vector.h"
#include "game.h"
#include "debug_drawing.h"
#include "mesh.h"

struct collider
{
  vec3    Vertices;
  int32_t VertexCount;
};

struct contact_point
{
  vec3 P;
  vec3 SupportA;
};

vec3
TransformVector(vec3 Vector, mat4 Matrix)
{
  return Math::Vec4ToVec3(Math::MulMat4Vec4(Matrix, Math::Vec4(Vector, 1.0f)));
}

vec3
Support(Render::mesh* Mesh, vec3 Direction, mat4 ModelMatrix)
{
  vec3 Transformed = TransformVector(Mesh->Vertices[0].Position, ModelMatrix);

  float   Max   = Math::Dot(Transformed, Direction);
  int32_t Index = 0;

  float DotProduct;

  for(int i = 1; i < Mesh->VerticeCount; i++)
  {
    Transformed = TransformVector(Mesh->Vertices[i].Position, ModelMatrix);
    DotProduct  = Math::Dot(Transformed, Direction);
    if(DotProduct > Max)
    {
      Max   = DotProduct;
      Index = i;
    }
  }

  return TransformVector(Mesh->Vertices[Index].Position, ModelMatrix);
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
    Simplex[2]    = Simplex[3]; // A -> 2
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
  vec3 TransformedA = TransformVector(MeshA->Vertices[0].Position, ModelAMatrix);
  vec3 TransformedB = TransformVector(MeshB->Vertices[0].Position, ModelBMatrix);

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

    ++i;
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
    Math::Normalized(Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P,
                                 Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P));

  ++(*TriangleCount);

  // ACD
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[1];
  Polytope[*TriangleCount].C = Simplex[0];

  Polytope[*TriangleCount].Normal =
    Math::Normalized(Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P,
                                 Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P));
  ++(*TriangleCount);

  // ADB
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[0];
  Polytope[*TriangleCount].C = Simplex[2];

  Polytope[*TriangleCount].Normal =
    Math::Normalized(Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P,
                                 Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P));

  ++(*TriangleCount);

  // BDC
  Polytope[*TriangleCount].A = Simplex[2];
  Polytope[*TriangleCount].B = Simplex[0];
  Polytope[*TriangleCount].C = Simplex[1];

  Polytope[*TriangleCount].Normal =
    Math::Normalized(Math::Cross(Polytope[*TriangleCount].B.P - Polytope[*TriangleCount].A.P,
                                 Polytope[*TriangleCount].C.P - Polytope[*TriangleCount].A.P));

  ++(*TriangleCount);
}

float
PointToPlaneDistance(vec3 Point, vec3 PlaneNormal)
{
  return Math::Dot(Point, PlaneNormal);
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

#define DEBUG_COLLISION 0
#define DEBUG_SHOW_RESULT (1 || DEBUG_COLLISION)

vec3
EPA(vec3* CollisionPoint, contact_point* Simplex, Render::mesh* MeshA, Render::mesh* MeshB,
    mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount)
{
  vec3 Result;

  const float MinThreshold = 0.00001f;

  float PrevMinDistance = FLT_MAX;

  triangle Polytope[100];
  int32_t  TriangleCount = 0;

  GeneratePolytopeFrom3Simplex(Polytope, &TriangleCount, Simplex);

  for(int Iteration = 0; Iteration < IterationCount; Iteration++)
  {
    if(Iteration == IterationCount - 1)
    {
#if DEBUG_COLLISION
      for(int i = 0; i < TriangleCount; i++)
      {
        Debug::PushLine(Polytope[i].A.P, Polytope[i].B.P, { 0, 0, 1, 1 });
        Debug::PushLine(Polytope[i].B.P, Polytope[i].C.P, { 0, 0, 1, 1 });
        Debug::PushLine(Polytope[i].C.P, Polytope[i].A.P, { 0, 0, 1, 1 });
        vec3 NormalStart =
          0.33f * Polytope[i].A.P + 0.33f * Polytope[i].B.P + 0.33f * Polytope[i].C.P;
        vec3 NormalEnd = NormalStart + Polytope[i].Normal;
        Debug::PushLine(NormalStart, NormalEnd, { 1, 0, 1, 1 });
        Debug::PushWireframeSphere(NormalEnd, 0.05f);
      }
#endif
    }
    int32_t TriangleIndex = 0;
    float   MinDistance   = PointToPlaneDistance(Polytope[0].A.P, Polytope[0].Normal);

    for(int i = 1; i < TriangleCount; i++)
    {
      float CurrentDistance = PointToPlaneDistance(Polytope[i].A.P, Polytope[i].Normal);
      if(CurrentDistance < MinDistance)
      {
        MinDistance   = CurrentDistance;
        TriangleIndex = i;
      }
    }

    vec3 SupportA = Support(MeshA, Polytope[TriangleIndex].Normal, ModelAMatrix);
    vec3 SupportB = Support(MeshB, -Polytope[TriangleIndex].Normal, ModelBMatrix);
    vec3 NewPoint = SupportA - SupportB;

    Result = Polytope[TriangleIndex].Normal *
             PointToPlaneDistance(NewPoint, Polytope[TriangleIndex].Normal);
#if DEBUG_COLLISION
    if(Iteration == IterationCount - 1)
    {
      printf("NewPoint Distance = %.16f\n",
             PointToPlaneDistance(NewPoint, Polytope[TriangleIndex].Normal));
      printf("Result Length = %.16f\n", Math::Length(Result));
      printf("MinDistance = %.16f\n", MinDistance);
      printf("PrevMinDistance = %.16f\n", PrevMinDistance);
      printf("================\n");
      Debug::PushLine({}, MinDistance * Polytope[TriangleIndex].Normal, { 1, 1, 0, 1 });
      Debug::PushWireframeSphere(NewPoint, 0.05f, { 0, 1, 0, 1 });
    }
#endif
    if(fabs(PrevMinDistance - MinDistance) < MinThreshold)
    {
      float U, V, W;

      BarycentricCoordinates(&U, &V, &W, NewPoint, Polytope[TriangleIndex].A.P,
                             Polytope[TriangleIndex].B.P, Polytope[TriangleIndex].C.P);

      *CollisionPoint  = U * SupportA + V * SupportA + W * SupportA;
      vec3 SecondPoint = U * SupportB + V * SupportB + W * SupportB;

#if DEBUG_SHOW_RESULT
      Debug::PushLine(SecondPoint, SecondPoint + Result, { 1, 1, 0, 1 });
      Debug::PushWireframeSphere(SecondPoint + Result, 0.05f, { 1, 1, 0, 1 });
      Debug::PushLine(*CollisionPoint, *CollisionPoint - Result, { 0, 1, 0, 1 });
      Debug::PushWireframeSphere(*CollisionPoint - Result, 0.05f, { 0, 1, 0, 1 });
#endif

      return -Result;
    }

    edge    Edges[100];
    int32_t EdgeCount = 0;

    Iteration++;

    if(Iteration < IterationCount)
    {
      for(int i = 0; i < TriangleCount; i++)
      {
#if 0
        float CurrentDistance = PointToPlaneDistance(Polytope[i].A.P, Polytope[i].Normal);
        if(CurrentDistance < PointToPlaneDistance(NewPoint, Polytope[i].Normal))
#else
        if(Math::Dot(Polytope[i].Normal, NewPoint - Polytope[i].A.P) > MinThreshold)
#endif
        {
#if DEBUG_COLLISION
          if(Iteration == IterationCount - 1)
          {
            vec3 NormalStart =
              0.33f * Polytope[i].A.P + 0.33f * Polytope[i].B.P + 0.33f * Polytope[i].C.P;
            vec3 NormalEnd = NormalStart + Polytope[i].Normal;
            Debug::PushLine(NormalStart, NormalEnd, { 0, 1, 0, 1 });
            Debug::PushWireframeSphere(NormalEnd, 0.05f);
          }
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
        Math::Normalized(Math::Cross(Polytope[TriangleCount].B.P - Polytope[TriangleCount].A.P,
                                     Polytope[TriangleCount].C.P - Polytope[TriangleCount].A.P));

      ++TriangleCount;
    }
    PrevMinDistance = MinDistance;
  }
  return {};
}

// SAT Implementation

struct sat_contact_point
{
  vec3  Position;
  float Penetration;
};

struct sat_contact_manifold
{
  int32_t           PointCount;
  sat_contact_point Points[4];
  vec3              Normal;
};

struct face;

struct half_edge
{
  vec3* Tail;

  half_edge* Next;
  half_edge* Twin;

  face* Face;
};

struct face
{
  half_edge* Edge;

  int32_t ConflictingVertexCount;
  vec3    ConflictList[10];

  vec3 Normal;
};

struct hull
{
  vec3 Centroid;

  int32_t VertexCount;
  vec3    Vertices[30];

  int32_t   EdgeCount;
  half_edge Edges[60];

  int32_t FaceCount;
  face    Faces[10];
};

vec3
HullSupport(hull* Hull, vec3 Direction, mat4 ModelMatrix)
{
  vec3 Transformed = TransformVector(Hull->Vertices[0], ModelMatrix);

  float   Max   = Math::Dot(Transformed, Direction);
  int32_t Index = 0;

  float DotProduct;

  for(int i = 1; i < Hull->VertexCount; i++)
  {
    Transformed = TransformVector(Hull->Vertices[i], ModelMatrix);
    DotProduct  = Math::Dot(Transformed, Direction);
    if(DotProduct > Max)
    {
      Max   = DotProduct;
      Index = i;
    }
  }

  return TransformVector(Hull->Vertices[Index], ModelMatrix);
}

void
CalculateFaceNormal(face* Face)
{
  vec3 A = *Face->Edge->Tail;
  vec3 B = *Face->Edge->Next->Tail;
  vec3 C = *Face->Edge->Next->Next->Tail;

  Face->Normal = Math::Normalized(Math::Cross(B - A, C - A));
}

struct face_query
{
  int32_t Index;
  float   Separation;
};

struct edge_query
{
  int32_t IndexA;
  int32_t IndexB;
  float   Separation;
};

void
QueryFaceDirections(face_query* Result, const mat4 TransformA, hull* HullA, const mat4 TransformB,
                    hull* HullB)
{
  mat4 HullBLocalT = Math::MulMat4(TransformB, TransformA);

  int32_t MaxIndex = 0;
  float   MaxSeparation =
    PointToPlaneDistance(HullSupport(HullB, -HullA->Faces[0].Normal, HullBLocalT),
                         HullA->Faces[0].Normal);

  for(int i = 1; i < HullA->FaceCount; ++i)
  {
    float Separation =
      PointToPlaneDistance(HullSupport(HullB, -HullA->Faces[i].Normal, HullBLocalT),
                           HullA->Faces[i].Normal);
    if(Separation > MaxSeparation)
    {
      MaxIndex      = i;
      MaxSeparation = Separation;
    }
  }

  Result->Index      = MaxIndex;
  Result->Separation = MaxSeparation;
}

void
QueryEdgeDirections(edge_query* Result, const mat4 TransformA, hull* HullA, const mat4 TransformB,
                    hull* HullB)
{
  int32_t MaxIndexA     = -1;
  int32_t MaxIndexB     = -1;
  float   MaxSeparation = -FLT_MAX;

  mat4 HullBLocalT = Math::MulMat4(TransformB, TransformA);

  vec3 TransformedCenterA = TransformVector(HullA->Centroid, HullBLocalT);

  for(int i = 0; i < HullA->EdgeCount; ++i)
  {
    half_edge* EdgeA     = &HullA->Edges[i];
    vec3       EdgeATail = TransformVector(*EdgeA->Tail, HullBLocalT);
    vec3       EdgeAHead = TransformVector(*EdgeA->Next->Tail, HullBLocalT);
    for(int j = 0; j < HullB->EdgeCount; ++j)
    {
      half_edge* EdgeB = &HullB->Edges[i];
      vec3       Axis =
        Math::Normalized(Math::Cross(EdgeAHead - EdgeATail, *EdgeB->Next->Tail - *EdgeB->Tail));

      float Separation = PointToPlaneDistance(*EdgeB->Tail - EdgeATail, Axis);
      if(Separation > MaxSeparation)
      {
        MaxIndexA     = i;
        MaxIndexB     = j;
        MaxSeparation = Separation;
      }
    }
  }

  Result->IndexA     = MaxIndexA;
  Result->IndexB     = MaxIndexB;
  Result->Separation = MaxSeparation;
}

bool
IntersectEdgeFace(vec3* IntersectionPoint, vec3 PointA, vec3 PointB, vec3 FacePoint,
                  vec3 FaceNormal)
{
  vec3  Edge = PointB - PointA;
  float d    = Math::Dot(FaceNormal, FacePoint);
  float t    = (d - Math::Dot(FaceNormal, PointA)) / Math::Dot(FaceNormal, Edge);

  if(t >= 0.0f && t <= 1.0f)
  {
    *IntersectionPoint = PointA + t * Edge;
  }

  return false;
}

void
CreateFaceContact(sat_contact_manifold* Manifold, face_query QueryA, const mat4 TransformA,
                  hull* HullA, face_query QueryB, const mat4 TransformB, hull* HullB)
{
  int32_t           ContactPointCount = 0;
  sat_contact_point ContactPoints[20];

  half_edge* ReferenceFaceEdge = HullA->Faces[QueryA.Index].Edge;

  int32_t Index         = 0;
  float   MinDotProduct = Math::Dot(HullA->Faces[QueryA.Index].Normal, HullB->Faces[0].Normal);
  for(int i = 1; i < HullB->FaceCount; ++i)
  {
    float DotProduct = Math::Dot(HullA->Faces[QueryA.Index].Normal, HullB->Faces[0].Normal);
    if(DotProduct < MinDotProduct)
    {
      Index         = i;
      MinDotProduct = DotProduct;
    }
  }

  half_edge* IncidentFaceEdge = HullB->Faces[Index].Edge;

  for(half_edge* r = ReferenceFaceEdge; r->Next != ReferenceFaceEdge; r = r->Next)
  {
    for(half_edge* i = IncidentFaceEdge; i->Next != IncidentFaceEdge; i = i->Next)
    {
      vec3 NewPoint;
      if(IntersectEdgeFace(&NewPoint, *r->Tail, *r->Next->Tail, *i->Tail, i->Face->Normal))
      {
        ContactPoints[ContactPointCount].Position    = NewPoint;
        ContactPoints[ContactPointCount].Penetration = QueryA.Separation;
      }
    }
  }
}

bool
SAT(sat_contact_manifold* Manifold, const mat4 TransformA, hull* HullA, const mat4 TransformB,
    hull* HullB)
{
  face_query QueryA;

  QueryFaceDirections(&QueryA, TransformA, HullA, TransformB, HullB);
  if(QueryA.Separation > 0.0f)
  {
    return false;
  }

  face_query QueryB;

  QueryFaceDirections(&QueryB, TransformB, HullB, TransformA, HullA);
  if(QueryB.Separation > 0.0f)
  {
    return false;
  }

  edge_query EdgeQuery;

  QueryEdgeDirections(&EdgeQuery, TransformA, HullA, TransformB, HullB);
  if(EdgeQuery.Separation > 0.0f)
  {
    return false;
  }

  if(QueryA.Separation > EdgeQuery.Separation && QueryB.Separation > EdgeQuery.Separation)
  {
    CreateFaceContact(Manifold, QueryA, TransformA, HullA, QueryB, TransformB, HullB);
  }
  else
  {
    // CreateEdgeContact(Manifold, EdgeQuery, TransformA, HullA, TransformB, HullB);
  }

  return true;
}

#if 0
bool
BuildInitialHull(hull* Hull, vec3* Vertices, int32_t VertexCount)
{
  if(VertexCount < 4)
  {
    printf("Not enough vertices to build initial hull!\n");
    return false;
  }

  Hull->VertexCount = 0;
  Hull->EdgeCount   = 0;
  Hull->FaceCount   = 0;

  vec3 MinX = Vertices[0];
  vec3 MaxX = Vertices[0];
  vec3 MinY = Vertices[0];
  vec3 MaxY = Vertices[0];
  vec3 MinZ = Vertices[0];
  vec3 MaxZ = Vertices[0];

  for(int i = 1; i < VertexCount; ++i)
  {
    if(Vertices[i].X < MinX.X)
    {
      MinX = Vertices[i];
    }
    else if(Vertices[i].X > MaxX.X)
    {
      MaxX = Vertices[i];
    }

    if(Vertices[i].Y < MinY.Y)
    {
      MinY = Vertices[i];
    }
    else if(Vertices[i].Y > MaxY.Y)
    {
      MaxY = Vertices[i];
    }

    if(Vertices[i].Z < MinZ.Z)
    {
      MinZ = Vertices[i];
    }
    else if(Vertices[i].Z > MaxZ.Z)
    {
      MaxZ = Vertices[i];
    }
  }

  float LengthX = Math::Length(MaxX - MinX);
  float LengthY = Math::Length(MaxY - MinY);
  float LengthZ = Math::Length(MaxZ - MinZ);

  if(LengthX > LengthY && LengthX > LengthZ)
  {
    Hull->Vertices[VertexCount++] = MinX;
    Hull->Vertices[VertexCount++] = MaxX;
  }
  else if(LengthY > LengthX && LengthY > LengthZ)
  {
    Hull->Vertices[VertexCount++] = MinY;
    Hull->Vertices[VertexCount++] = MaxY;
  }
  else
  {
    Hull->Vertices[VertexCount++] = MinZ;
    Hull->Vertices[VertexCount++] = MaxZ;
  }

  Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[0];
  Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[1];
  Hull->Edges[0].Twin           = &Hull->Edges[1];
  Hull->Edges[1].Twin           = &Hull->Edges[0];

  vec3 Edge = Hull->Edges[1]->Tail - Hull->Edges[0]->Tail;

  vec3 Normal = Math::Normalized(Math::Cross(Math::Cross(Edge, Hull->Vertices[0]), Edge));

  int32_t Index = 0;

  float MaxDistance = PointToPlaneDistance(Vertices[0], Normal);

  for(int i = 1; i < VertexCount; ++i)
  {
    Distance = PointToPlaneDistance(Vertices[i], Normal);
    if(fabs(Distance) > fabs(MaxDistance))
    {
      MaxDistance = Distance;
      Index       = i;
    }
  }

  Hull->Vertices[VertexCount++] = Vertices[Index];

  if(MaxDistance > 0.0f)
  {
    Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[1];
    Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[2];
    Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[2];
    Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[0];
  }
  else if(MaxDistance < 0.0f)
  {
    Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[2];
    Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[1];
    Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[1];
    Hull->Edges[EdgeCount++].Tail = &Hull->Vertices[0];
  }
  else
  {
    return false;
  }

  Hull->Edges[0].Next     = &Hull->Edges[2];
  Hull->Edges[0].Previous = &Hull->Edges[4];
  Hull->Edges[2].Next     = &Hull->Edges[4];
  Hull->Edges[2].Previous = &Hull->Edges[0];
  Hull->Edges[4].Next     = &Hull->Edges[0];
  Hull->Edges[4].Previous = &Hull->Edges[2];

  Hull->Edges[2].Twin = Hull->Edges[3].Twin;
  Hull->Edges[3].Twin = Hull->Edges[2].Twin;
  Hull->Edges[4].Twin = Hull->Edges[5].Twin;
  Hull->Edges[5].Twin = Hull->Edges[4].Twin;
}

void
QuickHull(hull* Hull, vec3* Vertices, int32_t VertexCount)
{
}
#endif
