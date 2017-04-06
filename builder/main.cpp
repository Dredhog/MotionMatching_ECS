#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>
#include <stdlib.h>
#include <stdio.h>

#include "mesh.h"
#include "model.h"
#include "asset.h"
#include "anim.h"

#include "stack_allocator.h"
#include "stack_allocator.cpp"

#include "file_io.h"
#include "pack.h"

void InsertBoneIntoVertex(Render::vertex* Vertex, int BoneIndex, float BoneWeight);
void NormalizeVertexBoneWeights(Render::vertex* Verte);

void
InsertBoneIntoVertex(Render::vertex* Vertex, int BoneIndex, float BoneWeight)
{
  int SmallestIndex = 0;
  for(int i = 1; i < VERTEX_MAX_BONE_COUNT; i++)
  {
    if(Vertex->BoneWeights[i] < Vertex->BoneWeights[SmallestIndex])
    {
      SmallestIndex = i;
    }
  }
  if(Vertex->BoneWeights[SmallestIndex] < BoneWeight)
  {
    Vertex->BoneIndices[SmallestIndex] = BoneIndex;
    Vertex->BoneWeights[SmallestIndex] = BoneWeight;
  }
}

void
NormalizeVertexBoneWeights(Render::vertex* Vertex)
{
  float WeightSum = 0.0f;
  for(int i = 0; i < VERTEX_MAX_BONE_COUNT; i++)
  {
    WeightSum += Vertex->BoneWeights[i];
  }

  if(WeightSum > 0.0f)
  {
    for(int i = 0; i < VERTEX_MAX_BONE_COUNT; i++)
    {
      Vertex->BoneWeights[i] /= WeightSum;
    }
    float checksum = 0.0f;
    for(int i = 0; i < VERTEX_MAX_BONE_COUNT; i++)
    {
      checksum += Vertex->BoneWeights[i];
    }
    assert(0.99f < checksum && checksum < 1.01f);
  }
}

mat4
Convert_aiMatrix4x4_To_mat4(aiMatrix4x4t<float> AssimpMat)
{
  mat4 Result;
  Result._11 = AssimpMat.a1;
  Result._21 = AssimpMat.b1;
  Result._31 = AssimpMat.c1;
  Result._41 = AssimpMat.d1;
  Result._12 = AssimpMat.a2;
  Result._22 = AssimpMat.b2;
  Result._32 = AssimpMat.c2;
  Result._42 = AssimpMat.d2;
  Result._13 = AssimpMat.a3;
  Result._23 = AssimpMat.b3;
  Result._33 = AssimpMat.c3;
  Result._43 = AssimpMat.d3;
  Result._14 = AssimpMat.a4;
  Result._24 = AssimpMat.b4;
  Result._34 = AssimpMat.c4;
  Result._44 = AssimpMat.d4;
  return Result;
}

void
r_CreateSkeleton(aiNode* Node, Anim::skeleton* Skeleton)
{
  assert(Skeleton->BoneCount >= 0 && Skeleton->BoneCount < SKELETON_MAX_BONE_COUNT);

  assert(Node->mName.length <= BONE_NAME_LENGTH);
  Anim::bone Bone = {};
  strcpy(Bone.Name, Node->mName.C_Str());
  Bone.Index = Skeleton->BoneCount;

  int ParentIndex;

  if(Node->mParent &&
     ((ParentIndex = Anim::GetBoneIndexFromName(Skeleton, Node->mParent->mName.C_Str())) >= 0))
  {
    Bone.ParentIndex = ParentIndex;
  }
  else
  {
    Bone.ParentIndex = -1;
  }
  Anim::AddBone(Skeleton, Bone);

  for(int i = 0; i < Node->mNumChildren; i++)
  {
    r_CreateSkeleton(Node->mChildren[i], Skeleton);
  }
}

aiNode*
r_FindSkeletonRootInSubtre(aiNode* Node)
{
  if(strcmp(Node->mName.C_Str(), "root") == 0)
  {
    return Node;
  }
  for(int i = 0; i < Node->mNumChildren; i++)
  {
    aiNode* Result = r_FindSkeletonRootInSubtre(Node->mChildren[i]);
    if(Result)
    {
      return Result;
    }
  }
  return 0;
}

Render::mesh
ProcessMesh(Memory::stack_allocator* Alloc, const aiMesh* AssimpMesh, Anim::skeleton* Skeleton)
{
  Render::mesh Mesh = {};
  Mesh.VerticeCount = AssimpMesh->mNumVertices;

  // Write All Vertices
  Mesh.Vertices = PushArray(Alloc, Mesh.VerticeCount, Render::vertex);
  for(int i = 0; i < Mesh.VerticeCount; i++)
  {
    Render::vertex Vertex = {};
    Vertex.Position.X     = AssimpMesh->mVertices[i].x;
    Vertex.Position.Y     = AssimpMesh->mVertices[i].y;
    Vertex.Position.Z     = AssimpMesh->mVertices[i].z;

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
  if(AssimpMesh->HasBones() && Skeleton->BoneCount > 0)
  {
    Mesh.HasBones = true;
    // CURRENTLY ONLY ONE BONE
    Mesh.BoneCount = AssimpMesh->mNumBones;
    for(int i = 0; i < AssimpMesh->mNumBones; i++)
    {
      aiBone* AssimpBone = AssimpMesh->mBones[i];
      int32_t BoneIndex  = Anim::GetBoneIndexFromName(Skeleton, AssimpBone->mName.C_Str());

      Skeleton->Bones[BoneIndex].InverseBindPose =
        Convert_aiMatrix4x4_To_mat4(AssimpBone->mOffsetMatrix);

      // Assimp's Inverse inverts in place...
      AssimpBone->mOffsetMatrix.Inverse();

      Skeleton->Bones[BoneIndex].BindPose = Convert_aiMatrix4x4_To_mat4(AssimpBone->mOffsetMatrix);
      for(int j = 0; j < AssimpBone->mNumWeights; j++)
      {
        InsertBoneIntoVertex(&Mesh.Vertices[AssimpBone->mWeights[j].mVertexId], BoneIndex,
                             AssimpBone->mWeights[j].mWeight);
      }
    }
    for(int i = 0; i < Mesh.VerticeCount; i++)
    {
      NormalizeVertexBoneWeights(&Mesh.Vertices[i]);
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
              Render::model* Model, Anim::skeleton* Skeleton)
{
  for(int i = 0; i < Node->mNumMeshes; i++)
  {
    Render::mesh* Mesh = PushStruct(Alloc, Render::mesh);
    *Mesh              = ProcessMesh(Alloc, Scene->mMeshes[Node->mMeshes[i]], Skeleton);

    Model->Meshes[Model->MeshCount++] = Mesh;
  }
  for(int i = 0; i < Node->mNumChildren; i++)
  {
    r_ProcessNode(Alloc, Node->mChildren[i], Scene, Model, Skeleton);
  }
}

int32_t
CalculateTotalAssetSize(const aiScene* Scene, bool IsActor)
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

  if(IsActor)
  {
    Total += sizeof(Anim::skeleton);
  }

  return SafeTruncateUint64(Total);
}

int
main(int ArgCount, char** Args)
{
  char* ModelName;
  char* ActorName;
  // Process comman line arguments
  {
    if(ArgCount != 3)
    {
      printf("error: not enough command line arguments.\n");
      return 1;
    }
    const char* ModelExtension = ".model";
    const char* ActorExtension = ".actor";

    size_t LenWithoutExtension = strlen(Args[2]);
    ModelName                  = (char*)malloc(LenWithoutExtension + strlen(ModelExtension));
    ActorName                  = (char*)malloc(LenWithoutExtension + strlen(ActorExtension));

    strcpy(ModelName, Args[2]);
    strcpy(ActorName, Args[2]);
    strcpy(ModelName + LenWithoutExtension, ModelExtension);
    strcpy(ActorName + LenWithoutExtension, ActorExtension);

#if 0
    char* DestName       = Args[2];
    char* ModelExtension = ".model";
    char* ActorExtension = ".actor";

    size_t DestNameLen       = strlen(DestName);
    size_t ModelExtensionLen = strlen(ModelExtension);
    size_t ActorExtensionLen = strlen(ActorExtension);

    if(DestNameLen > ModelExtensionLen &&
       strcmp(&DestName[DestNameLen - ModelExtensionLen], ModelExtension) == 0)
    {
      printf("This is an model\n");
    }
    else if(DestNameLen > ActorExtensionLen &&
            strcmp(&DestName[DestNameLen - ActorExtensionLen], ActorExtension) == 0)
    {
      IsActor = true;
      printf("This is an actor\n");
    }
    else
    {
      printf("error: output must have either '.model' or '.actor' extension'\n");
      return 1;
    }
#endif
  }

  Assimp::Importer Importer;
  const aiScene*   Scene = Importer.ReadFile(Args[1], aiProcess_SplitLargeMeshes |
                                                      aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);
  if(!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
  {
    printf("error::assimp: %s\n", Importer.GetErrorString());
    return 1;
  }

  // Create Skeleton
  bool           IsActor  = false;
  Anim::skeleton Skeleton = {};
  {
    aiNode* SkeletonRootNode = r_FindSkeletonRootInSubtre(Scene->mRootNode);

    if(SkeletonRootNode)
    {
      r_CreateSkeleton(SkeletonRootNode, &Skeleton);
      IsActor = true;
    }
  }

  // Determine file size on disk
  int32_t TotalOutputFileSize = CalculateTotalAssetSize(Scene, IsActor);
  void*   FileMemory          = calloc(TotalOutputFileSize, 1);

  Memory::stack_allocator Allocator = {};
  Allocator.Create(FileMemory, TotalOutputFileSize);

  // Create Asset Header
  Asset::asset_file_header* AssetHeader = PushStruct(&Allocator, Asset::asset_file_header);

  // Create Model
  {
    // Reserve Space For Model
    Render::model* Model = PushStruct(&Allocator, Render::model);
    // Reserve Space For Mesh Pointer Array
    Model->Meshes = PushArray(&Allocator, Scene->mNumMeshes, Render::mesh*);

    // Create The Actal Meshesmeshes
    r_ProcessNode(&Allocator, Scene->mRootNode, Scene, Model, &Skeleton);

    AssetHeader->Model = (uint64_t)Model;
  }

  if(IsActor)
  {
    // write '.model'
    assert(Allocator.GetUsedSize() == TotalOutputFileSize - sizeof(Anim::skeleton));
    AssetHeader->AssetType = (uint32_t)Asset::ASSET_Model;

    printf("writing: %s\n", ModelName);
    PrintModelHeader((Render::model*)AssetHeader->Model);
    PackAsset(AssetHeader, TotalOutputFileSize - sizeof(Anim::skeleton));
    WriteEntireFile(ModelName, TotalOutputFileSize - sizeof(Anim::skeleton), FileMemory);

    UnpackAsset(AssetHeader);

    // write '.actor'
    Anim::skeleton* SkeletonPtr = PushStruct(&Allocator, Anim::skeleton);
    *SkeletonPtr                = Skeleton;
    AssetHeader->Skeleton       = (uint64_t)SkeletonPtr;
    AssetHeader->AssetType      = (uint32_t)Asset::ASSET_Actor;
    assert(Allocator.GetUsedSize() == TotalOutputFileSize);

    printf("writing: %s\n", ActorName);
    PrintSkeleton((Anim::skeleton*)AssetHeader->Skeleton);
    printf("\n");
    PackAsset(AssetHeader, TotalOutputFileSize);
    WriteEntireFile(ActorName, TotalOutputFileSize, FileMemory);
  }
  else
  {
    // write '.model'
    assert(Allocator.GetUsedSize() == TotalOutputFileSize);
    AssetHeader->AssetType = (uint32_t)Asset::ASSET_Model;
    AssetHeader->TotalSize = TotalOutputFileSize;

    printf("writing: %s\n", ModelName);
    Render::PrintModelHeader((Render::model*)AssetHeader->Model);
    printf("\n");
    PackAsset(AssetHeader, TotalOutputFileSize);
    WriteEntireFile(ModelName, TotalOutputFileSize, FileMemory);
  }
  return 0;
}
