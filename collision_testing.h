#pragma once

#include "collision.h"

bool
TestHullvsHull(sat_contact_manifold* Manifold, Render::mesh* MeshA, Render::mesh* MeshB,
               mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount = 50)
{
  int32_t SimplexOrder;
  vec3    Direction;
  vec3    Simplex[4];
  int     IterationsToFindSimplex;

  bool CollisionFound = GJK(Simplex, &SimplexOrder, MeshA, MeshB, ModelAMatrix, ModelBMatrix,
                            IterationCount, &IterationsToFindSimplex, &Direction);

  if(CollisionFound)
  {
    vec3 CollisionPoint;
    vec3 PenetrationVector = EPA(&CollisionPoint, Simplex, MeshA, MeshB, ModelAMatrix, ModelBMatrix,
                                 IterationCount - IterationsToFindSimplex);
    Manifold->Points[0].Position    = CollisionPoint;
    Manifold->Points[0].Penetration = Math::Length(PenetrationVector);
    Manifold->PointCount            = 1;
    Manifold->Normal                = Math::Normalized(PenetrationVector);
    if(Manifold->Points[0].Penetration == 0)
    {
      CollisionFound = false;
    }
  }
  else
  {
    Manifold->PointCount = 0;
    /*
    for(int i = 0; i <= SimplexOrder; i++)
    {
      Debug::PushWireframeSphere(Simplex[i], 0.05f, { 1, 0, 1, 1 });
      for(int j = 1; j <= SimplexOrder; j++)
      {
        Debug::PushLine(Simplex[i], Simplex[j]);
      }
    }
    Debug::PushLine({ 0.0f, 0.0f, 0.0f }, Math::Normalized(Direction), { 0.0f, 0.0f, 1.0f, 1.0f });
    */
  }
  return CollisionFound;
}

void
SetUpCubeHull(hull* Cube)
{
  Cube->Centroid = { 0.0f, 0.0f, 0.0f };

  Cube->VertexCount          = 8;
  Cube->Vertices[0].Position = { 1.0f, 1.0f, 1.0f };
  Cube->Vertices[1].Position = { 1.0f, -1.0f, 1.0f };
  Cube->Vertices[2].Position = { 1.0f, -1.0f, -1.0f };
  Cube->Vertices[3].Position = { 1.0f, 1.0f, -1.0f };
  Cube->Vertices[4].Position = { -1.0f, 1.0f, 1.0f };
  Cube->Vertices[5].Position = { -1.0f, 1.0f, -1.0f };
  Cube->Vertices[6].Position = { -1.0f, -1.0f, -1.0f };
  Cube->Vertices[7].Position = { -1.0f, -1.0f, 1.0f };

  Cube->EdgeCount = 24;

  // Face 0
  Cube->Edges[0].Tail = &Cube->Vertices[0];
  Cube->Edges[1].Tail = &Cube->Vertices[1];
  Cube->Edges[2].Tail = &Cube->Vertices[2];
  Cube->Edges[3].Tail = &Cube->Vertices[3];

  Cube->Edges[0].Next = &Cube->Edges[1];
  Cube->Edges[1].Next = &Cube->Edges[2];
  Cube->Edges[2].Next = &Cube->Edges[3];
  Cube->Edges[3].Next = &Cube->Edges[0];

  Cube->Edges[0].Previous = &Cube->Edges[3];
  Cube->Edges[1].Previous = &Cube->Edges[0];
  Cube->Edges[2].Previous = &Cube->Edges[1];
  Cube->Edges[3].Previous = &Cube->Edges[2];

  // Face 1
  Cube->Edges[4].Tail = &Cube->Vertices[3];
  Cube->Edges[5].Tail = &Cube->Vertices[2];
  Cube->Edges[6].Tail = &Cube->Vertices[6];
  Cube->Edges[7].Tail = &Cube->Vertices[5];

  Cube->Edges[4].Next = &Cube->Edges[5];
  Cube->Edges[5].Next = &Cube->Edges[6];
  Cube->Edges[6].Next = &Cube->Edges[7];
  Cube->Edges[7].Next = &Cube->Edges[4];

  Cube->Edges[4].Previous = &Cube->Edges[7];
  Cube->Edges[5].Previous = &Cube->Edges[4];
  Cube->Edges[6].Previous = &Cube->Edges[5];
  Cube->Edges[7].Previous = &Cube->Edges[6];

  // Face 2
  Cube->Edges[8].Tail  = &Cube->Vertices[5];
  Cube->Edges[9].Tail  = &Cube->Vertices[6];
  Cube->Edges[10].Tail = &Cube->Vertices[7];
  Cube->Edges[11].Tail = &Cube->Vertices[4];

  Cube->Edges[8].Next  = &Cube->Edges[9];
  Cube->Edges[9].Next  = &Cube->Edges[10];
  Cube->Edges[10].Next = &Cube->Edges[11];
  Cube->Edges[11].Next = &Cube->Edges[8];

  Cube->Edges[8].Previous  = &Cube->Edges[11];
  Cube->Edges[9].Previous  = &Cube->Edges[8];
  Cube->Edges[10].Previous = &Cube->Edges[9];
  Cube->Edges[11].Previous = &Cube->Edges[10];

  // Face 3
  Cube->Edges[12].Tail = &Cube->Vertices[4];
  Cube->Edges[13].Tail = &Cube->Vertices[7];
  Cube->Edges[14].Tail = &Cube->Vertices[1];
  Cube->Edges[15].Tail = &Cube->Vertices[0];

  Cube->Edges[12].Next = &Cube->Edges[13];
  Cube->Edges[13].Next = &Cube->Edges[14];
  Cube->Edges[14].Next = &Cube->Edges[15];
  Cube->Edges[15].Next = &Cube->Edges[12];

  Cube->Edges[12].Previous = &Cube->Edges[15];
  Cube->Edges[13].Previous = &Cube->Edges[12];
  Cube->Edges[14].Previous = &Cube->Edges[13];
  Cube->Edges[15].Previous = &Cube->Edges[14];

  // Face 4
  Cube->Edges[16].Tail = &Cube->Vertices[5];
  Cube->Edges[17].Tail = &Cube->Vertices[4];
  Cube->Edges[18].Tail = &Cube->Vertices[0];
  Cube->Edges[19].Tail = &Cube->Vertices[3];

  Cube->Edges[16].Next = &Cube->Edges[17];
  Cube->Edges[17].Next = &Cube->Edges[18];
  Cube->Edges[18].Next = &Cube->Edges[19];
  Cube->Edges[19].Next = &Cube->Edges[16];

  Cube->Edges[16].Previous = &Cube->Edges[19];
  Cube->Edges[17].Previous = &Cube->Edges[16];
  Cube->Edges[18].Previous = &Cube->Edges[17];
  Cube->Edges[19].Previous = &Cube->Edges[18];

  // Face 5
  Cube->Edges[20].Tail = &Cube->Vertices[7];
  Cube->Edges[21].Tail = &Cube->Vertices[6];
  Cube->Edges[22].Tail = &Cube->Vertices[2];
  Cube->Edges[23].Tail = &Cube->Vertices[1];

  Cube->Edges[20].Next = &Cube->Edges[21];
  Cube->Edges[21].Next = &Cube->Edges[22];
  Cube->Edges[22].Next = &Cube->Edges[23];
  Cube->Edges[23].Next = &Cube->Edges[20];

  Cube->Edges[20].Previous = &Cube->Edges[23];
  Cube->Edges[21].Previous = &Cube->Edges[20];
  Cube->Edges[22].Previous = &Cube->Edges[21];
  Cube->Edges[23].Previous = &Cube->Edges[22];

  // Set up twins
  Cube->Edges[0].Twin  = &Cube->Edges[14];
  Cube->Edges[1].Twin  = &Cube->Edges[22];
  Cube->Edges[2].Twin  = &Cube->Edges[4];
  Cube->Edges[3].Twin  = &Cube->Edges[18];
  Cube->Edges[4].Twin  = &Cube->Edges[2];
  Cube->Edges[5].Twin  = &Cube->Edges[21];
  Cube->Edges[6].Twin  = &Cube->Edges[8];
  Cube->Edges[7].Twin  = &Cube->Edges[19];
  Cube->Edges[8].Twin  = &Cube->Edges[6];
  Cube->Edges[9].Twin  = &Cube->Edges[20];
  Cube->Edges[10].Twin = &Cube->Edges[12];
  Cube->Edges[11].Twin = &Cube->Edges[16];
  Cube->Edges[12].Twin = &Cube->Edges[10];
  Cube->Edges[13].Twin = &Cube->Edges[23];
  Cube->Edges[14].Twin = &Cube->Edges[0];
  Cube->Edges[15].Twin = &Cube->Edges[17];
  Cube->Edges[16].Twin = &Cube->Edges[11];
  Cube->Edges[17].Twin = &Cube->Edges[15];
  Cube->Edges[18].Twin = &Cube->Edges[3];
  Cube->Edges[19].Twin = &Cube->Edges[7];
  Cube->Edges[20].Twin = &Cube->Edges[9];
  Cube->Edges[21].Twin = &Cube->Edges[5];
  Cube->Edges[22].Twin = &Cube->Edges[1];
  Cube->Edges[23].Twin = &Cube->Edges[13];

  // Faces
  Cube->FaceCount = 6;

  Cube->Faces[0].Edge = &Cube->Edges[0];
  Cube->Faces[1].Edge = &Cube->Edges[4];
  Cube->Faces[2].Edge = &Cube->Edges[8];
  Cube->Faces[3].Edge = &Cube->Edges[12];
  Cube->Faces[4].Edge = &Cube->Edges[16];
  Cube->Faces[5].Edge = &Cube->Edges[20];

  for(int i = 0; i < Cube->FaceCount; i++)
  {
    CalculateFaceCentroid(&Cube->Faces[i]);
    CalculateFaceNormal(&Cube->Faces[i]);
    half_edge* Edge = Cube->Faces[i].Edge;
    do
    {
      Edge->Face = &Cube->Faces[i];
      Edge       = Edge->Next;
    } while(Edge != Cube->Faces[i].Edge);
  }
}

bool
TestSAT(const mat4 TransformA, const mat4 TransformB)
{
  hull Cube = {};

  SetUpCubeHull(&Cube);

  sat_contact_manifold Manifold = {};

  if(SAT(&Manifold, TransformA, &Cube, TransformB, &Cube))
  {
    return true;
  }
  return false;
}
