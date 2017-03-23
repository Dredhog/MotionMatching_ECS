#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdlib.h>
#include <stdio.h>

#include "mesh.h"
#include "model.h"
#include "asset.h"

#include "stack_allocator.h"
#include "stack_allocator.cpp"

#include "file_io.h"
#include "pack.h"

Render::mesh
ProcessMesh(Memory::stack_allocator* Alloc, const aiMesh* AssimpMesh)
{
  Render::mesh Mesh = {};
  Mesh.VerticeCount = AssimpMesh->mNumVertices;

  // Write All Vertices
  Mesh.Vertices = PushArray(Alloc, Mesh.VerticeCount, Render::vertex);
  for(int i = 0; i < Mesh.VerticeCount; i++)
  {
    Render::vertex Vertex;
    Vertex.Position.X = AssimpMesh->mVertices[i].x;
    Vertex.Position.Y = AssimpMesh->mVertices[i].y;
    Vertex.Position.Z = AssimpMesh->mVertices[i].z;

    Vertex.Normal.X = AssimpMesh->mNormals[i].x;
    Vertex.Normal.Y = AssimpMesh->mNormals[i].y;
    Vertex.Normal.Z = AssimpMesh->mNormals[i].z;

    Mesh.Vertices[i] = Vertex;
  }
  if(AssimpMesh->mTextureCoords[0])
  {
    Mesh.HasUVs = true;

    for(int i = 0; i < Mesh.VerticeCount; i++)
    {
      Mesh.Vertices[i].UV.U = AssimpMesh->mTextureCoords[0][i].x;
      Mesh.Vertices[i].UV.V = AssimpMesh->mTextureCoords[0][i].y;
    }
  }

  // Write All Indices
  Mesh.IndiceCount = 3 * AssimpMesh->mNumFaces;
  Mesh.Indices     = PushArray(Alloc, Mesh.IndiceCount, uint32_t);
  for(int i = 0; i < AssimpMesh->mNumFaces; i++)
  {
    aiFace Face = AssimpMesh->mFaces[i];
    assert(Face.mNumIndices == 3);

    for(int j = 0; j < Face.mNumIndices; j++)
    {
      Mesh.Indices[3 * i + j] = Face.mIndices[j];
    }
  }

  return Mesh;
}

void
r_ProcessNode(Memory::stack_allocator* Alloc, const aiNode* Node, const aiScene* Scene,
              Render::model* Model)
{
  for(int i = 0; i < Node->mNumMeshes; i++)
  {
    Render::mesh* Mesh = PushStruct(Alloc, Render::mesh);
    *Mesh              = ProcessMesh(Alloc, Scene->mMeshes[Node->mMeshes[i]]);

    Model->Meshes[Model->MeshCount++] = Mesh;
  }
  for(int i = 0; i < Node->mNumChildren; i++)
  {
    r_ProcessNode(Alloc, Node->mChildren[i], Scene, Model);
  }
}

int32_t
CalculateTotalModelAssetSize(const aiScene* Scene)
{
  int64_t Total = 0;

  Total += sizeof(Asset::asset_file_header);
  Total += sizeof(Render::model);
  Total += sizeof(Render::mesh*) * Scene->mNumMeshes;
  for(int i = 0; i < Scene->mNumMeshes; i++)
  {
    aiMesh* AssimpMesh = Scene->mMeshes[i];

    Total += sizeof(Render::mesh);
    Total += sizeof(Render::vertex) * AssimpMesh->mNumVertices;
    Total += sizeof(uint32_t) * AssimpMesh->mNumFaces * 3;
  }

  return SafeTruncateUint64(Total);
}

int
main(int ArgCount, char** Args)
{
  if(ArgCount < 3)
  {
    printf("error: not enough command line arguments.\n");
    return 1;
  }
  char* DestName       = Args[2];
  char* ModelEntension = ".model";

  size_t ExtensionLen = strlen(ModelEntension);
  size_t DestNameLen  = strlen(DestName);

  if(DestNameLen <= strlen(".model") || strcmp(&DestName[DestNameLen-(ExtensionLen)], ModelEntension))
  {
    printf("error: output must have the extension '.model'\n");
		return 1;
  }

  Assimp::Importer Importer;
  const aiScene*   Scene = Importer.ReadFile(Args[1], aiProcess_SplitLargeMeshes | aiProcess_Triangulate | aiProcess_GenNormals);
  if(!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
  {
    printf("error::assimp: %s\n", Importer.GetErrorString());
    return 1;
  }

  // Determine file size on disk
  int32_t TotalOutputFileSize = CalculateTotalModelAssetSize(Scene);
  void*   FileMemory          = calloc(TotalOutputFileSize, 1);

  Memory::stack_allocator Allocator = {};
  Allocator.Create(FileMemory, TotalOutputFileSize);

  // Create Asset Header
  Asset::asset_file_header* AssetHeader = PushStruct(&Allocator, Asset::asset_file_header);

  // Create Model Header
  Render::model* Model = PushStruct(&Allocator, Render::model);
  Model->Meshes        = PushArray(&Allocator, Scene->mNumMeshes, Render::mesh*);

  // Create meshes
  r_ProcessNode(&Allocator, Scene->mRootNode, Scene, Model);

  AssetHeader->HeaderOffset = (uint64_t)Model - (uint64_t)FileMemory;
  AssetHeader->Checksum     = 12345;
  AssetHeader->TotalSize    = TotalOutputFileSize;

  PrintModel(Model);
  PackModel(Model);

  assert(Allocator.GetUsedSize() == TotalOutputFileSize);
  WriteEntireFile(Args[2], TotalOutputFileSize, FileMemory);
  return 0;
}
