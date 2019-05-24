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

#include "stack_alloc.cpp"
#include "heap_alloc.cpp"

#include "file_io.h"
#include <float.h>

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

aiMatrix4x4t<float>
Convert_mat4_To_aiMatrix4x4(mat4 Mat)
{
  aiMatrix4x4t<float> Result;
  Result.a1 = Mat._11;
  Result.b1 = Mat._21;
  Result.c1 = Mat._31;
  Result.d1 = Mat._41;
  Result.a2 = Mat._12;
  Result.b2 = Mat._22;
  Result.c2 = Mat._32;
  Result.d2 = Mat._42;
  Result.a3 = Mat._13;
  Result.b3 = Mat._23;
  Result.c3 = Mat._33;
  Result.d3 = Mat._43;
  Result.a4 = Mat._14;
  Result.b4 = Mat._24;
  Result.c4 = Mat._34;
  Result.d4 = Mat._44;
  return Result;
}

// Debug inspection helper
void
r_PrintNodeHieararchy(const aiNode* Node, int Depth, const aiMatrix4x4t<float>& ParentMat)
{
  char IndentAndNameString[150];
  sprintf(IndentAndNameString, "%*s %s", Depth * 2, "", Node->mName.C_Str());

  aiMatrix4x4t<float> LocalMat  = Node->mTransformation;
  aiMatrix4x4t<float> GlobalMat = ParentMat * LocalMat;

  aiVector3t<float>    scaling;
  aiQuaterniont<float> rotation;
  aiVector3t<float>    position;
  LocalMat.Decompose(scaling, rotation, position);

  char TransformString[150];
  sprintf(TransformString,
          "T{ %.2f, %.2f, %.2f }, R{%.2f, %.2f, %.2f }, S{%.2f, %.2f, %.2f } GlobT{ %.2f, %.2f, "
          "%.2f }",
          (double)position.x, (double)position.y, (double)position.z, (double)rotation.x,
          (double)rotation.y, (double)rotation.z, (double)scaling.x, (double)scaling.y,
          (double)scaling.z, (double)GlobalMat.a4, (double)GlobalMat.b4, (double)GlobalMat.c4);

  printf("%-50s %s\n", IndentAndNameString, TransformString);
  for(int i = 0; i < Node->mNumChildren; i++)
  {
    r_PrintNodeHieararchy(Node->mChildren[i], Depth + 1, GlobalMat);
  }
}

#if 0
void
r_AddWithParentsUpToRoot(char** UsedBoneNames, int32_t* UsedBoneCount, const aiNode* Node,
                         const char* RootName)
{
}

void
r_AddMeshNodesWithParentsNeededBoneArray(char** UsedBoneNames, int32_t* UsedBoneCount,
                                         const aiNode* Node, const char* RootName)
{
}
#endif

#define USE_BIND_POSE

void
r_AddBindPoseBoneAndDescendantBonesToSkeleton(Anim::skeleton* Skeleton, const aiNode* Node,
                                              const aiMatrix4x4t<float>& GlobalNodeMat)
{
  assert(0 <= Skeleton->BoneCount && Skeleton->BoneCount < SKELETON_MAX_BONE_COUNT);

  assert(Node->mName.length <= BONE_NAME_LENGTH);
  Anim::bone Bone = {};
  strcpy(Bone.Name, Node->mName.C_Str());

#ifdef USE_BIND_POSE
  Bone.BindPose = Convert_aiMatrix4x4_To_mat4(GlobalNodeMat);
  // Bone.InverseBindPose = Math::InvMat4(Bone.BindPose);
#else
  Bone.BindPose = Math::Mat4Ident();
  // Bone.InverseBindPose = Math::Mat4Ident();
#endif

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
    r_AddBindPoseBoneAndDescendantBonesToSkeleton(Skeleton, Node->mChildren[i],
                                                  GlobalNodeMat *
                                                    Node->mChildren[i]->mTransformation);
  }
}

bool
r_GetNodeAndGlobalMatrixByName(aiNode** OutNode, aiMatrix4x4t<float>* OutMatrix,
                               const aiNode* CurrentNode, const aiMatrix4x4t<float>& GlobalNodeMat,
                               const char* DesiredNodeName)
{
  if(strcmp(CurrentNode->mName.C_Str(), DesiredNodeName) == 0)
  {
    *OutNode   = (aiNode*)CurrentNode;
    *OutMatrix = GlobalNodeMat;
    return true;
  }
  for(int i = 0; i < CurrentNode->mNumChildren; i++)
  {
    if(r_GetNodeAndGlobalMatrixByName(OutNode, OutMatrix, CurrentNode->mChildren[i],
                                      GlobalNodeMat * CurrentNode->mChildren[i]->mTransformation,
                                      DesiredNodeName))
    {
      return true;
    }
  }
  return false;
}

Render::mesh
ProcessMesh(Memory::stack_allocator* Alloc, const aiMesh* AssimpMesh, Anim::skeleton* Skeleton,
            float SpaceScale)
{
  Render::mesh Mesh = {};
  Mesh.VerticeCount = AssimpMesh->mNumVertices;

  // Write All Vertices
  Mesh.Vertices = PushArray(Alloc, Mesh.VerticeCount, Render::vertex);
  if(AssimpMesh->mTextureCoords[0])
  {
    Mesh.HasUVs      = true;
    Mesh.HasTangents = true;
  }
  for(int i = 0; i < Mesh.VerticeCount; i++)
  {
    Render::vertex Vertex = {};
    Vertex.Position.X     = SpaceScale * AssimpMesh->mVertices[i].x;
    Vertex.Position.Y     = SpaceScale * AssimpMesh->mVertices[i].y;
    Vertex.Position.Z     = SpaceScale * AssimpMesh->mVertices[i].z;

    Vertex.Normal.X = AssimpMesh->mNormals[i].x;
    Vertex.Normal.Y = AssimpMesh->mNormals[i].y;
    Vertex.Normal.Z = AssimpMesh->mNormals[i].z;

    if(Mesh.HasTangents)
    {
      Vertex.Tangent.X = AssimpMesh->mTangents[i].x;
      Vertex.Tangent.Y = AssimpMesh->mTangents[i].y;
      Vertex.Tangent.Z = AssimpMesh->mTangents[i].z;
    }

    Mesh.Vertices[i] = Vertex;
  }
  if(Mesh.HasUVs)
  {
    for(int i = 0; i < Mesh.VerticeCount; i++)
    {
      Mesh.Vertices[i].UV.U = AssimpMesh->mTextureCoords[0][i].x;
      Mesh.Vertices[i].UV.V = AssimpMesh->mTextureCoords[0][i].y;
    }
  }
  if(AssimpMesh->HasBones() && Skeleton->BoneCount > 0)
  {
    Mesh.HasBones  = true;
    Mesh.BoneCount = AssimpMesh->mNumBones;
    for(int i = 0; i < AssimpMesh->mNumBones; i++)
    {
      aiBone* AssimpBone = AssimpMesh->mBones[i];
      int32_t BoneIndex  = Anim::GetBoneIndexFromName(Skeleton, AssimpBone->mName.C_Str());

      // TODO(Lukas) reactivate this
#if 0
      assert(Convert_mat4_To_aiMatrix4x4(Skeleton->Bones[BoneIndex].InverseBindPose)
               .Equal(AssimpBone->mOffsetMatrix, 0.01f));
#endif

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
              Render::model* Model, Anim::skeleton* Skeleton, float SpaceScale,
              bool ExtractBindPose = false)
{
  for(int i = 0; i < Node->mNumMeshes; i++)
  {
    Render::mesh* Mesh = PushStruct(Alloc, Render::mesh);
    *Mesh              = ProcessMesh(Alloc, Scene->mMeshes[Node->mMeshes[i]], Skeleton, SpaceScale);

    Model->Meshes[Model->MeshCount++] = Mesh;
  }
  for(int i = 0; i < Node->mNumChildren; i++)
  {
    r_ProcessNode(Alloc, Node->mChildren[i], Scene, Model, Skeleton, SpaceScale);
  }
}

void
SimpleRemoveBoneFromSkeleton(Anim::skeleton* Skeleton, int RemoveIndex)
{
  assert(0 <= RemoveIndex && RemoveIndex < Skeleton->BoneCount);

  // Update child bone parent
  for(int b = RemoveIndex + 1; b < Skeleton->BoneCount; b++)
  {
    if(Skeleton->Bones[b].ParentIndex == RemoveIndex)
    {
      Skeleton->Bones[b].ParentIndex = Skeleton->Bones[RemoveIndex].ParentIndex;
    }
    else if(RemoveIndex < Skeleton->Bones[b].ParentIndex)
    {
      --Skeleton->Bones[b].ParentIndex;
    }
  }

  // Shift bones to fill vancancy
  for(int b = RemoveIndex + 1; b < Skeleton->BoneCount; b++)
  {
    Skeleton->Bones[b - 1] = Skeleton->Bones[b];
  }

  --Skeleton->BoneCount;
}

float
Min(float A, float B)
{
  return (A < B) ? A : B;
}

float
Max(float A, float B)
{
  return (A > B) ? A : B;
}

float
Min(float A, float B, float C)
{
  return (A < B) ? ((A < C) ? A : C) : ((B < C) ? B : C);
}

float
Max(float A, float B, float C)
{
  return (A > B) ? ((A > C) ? A : C) : ((B > C) ? B : C);
}

float
Clamp(float t, float A, float B)
{
  assert(A <= B);
  return (t <= A) ? A : ((t < B) ? t : B);
}

int32_t
GetAnimationKeyframeCount(const aiAnimation* Animation, float SamplingFrequency,
                          float* OutAnimStartTick = NULL, float* OutAnimEndTick = NULL)
{
  float AnimStartTick = FLT_MAX;
  float AnimEndTick   = -FLT_MAX;
  for(int i = 0; i < Animation->mNumChannels; i++)
  {
    const aiNodeAnim* BoneAnim = Animation->mChannels[i];

    float BoneAnimStartTick =
      Min(BoneAnim->mRotationKeys[0].mTime, BoneAnim->mPositionKeys[0].mTime,
          BoneAnim->mScalingKeys[0].mTime);

    float BoneAnimEndTick = Max(BoneAnim->mRotationKeys[BoneAnim->mNumRotationKeys - 1].mTime,
                                BoneAnim->mPositionKeys[BoneAnim->mNumPositionKeys - 1].mTime,
                                BoneAnim->mScalingKeys[BoneAnim->mNumScalingKeys - 1].mTime);

    AnimStartTick = Min(BoneAnimStartTick, AnimStartTick);
    AnimEndTick   = Max(BoneAnimEndTick, AnimEndTick);
  }
  float DuratinInTicks = AnimEndTick - AnimStartTick;
  assert(DuratinInTicks > 0);
  assert(Animation->mTicksPerSecond > 0);

  if(OutAnimStartTick)
  {
    *OutAnimStartTick = AnimStartTick;
  }
  if(OutAnimEndTick)
  {
    *OutAnimEndTick = AnimEndTick;
  }
  return (int32_t)Max(1, ((DuratinInTicks / Animation->mTicksPerSecond) * SamplingFrequency));
}

int32_t
CalculateTotalModelSize(const aiScene* Scene)
{
  int64_t Total = 0;

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
GetSubsampledKeyframeCount(int OriginalKeyframeCount, int UndersamplingPeriod)
{
  assert(OriginalKeyframeCount > 0);
  return ((OriginalKeyframeCount - 1) / UndersamplingPeriod) + 1;
}

int32_t
GetBoneIndexInChannelArray(int32_t BoneIndex, const aiNodeAnim* const* Channels,
                           int32_t ChannelCount, const Anim::skeleton* Skeleton)
{
  int ResultChannelIndex = -1;
  for(int c = 0; c < ChannelCount; c++)
  {
    if(strcmp(Skeleton->Bones[BoneIndex].Name, Channels[c]->mNodeName.C_Str()) == 0)
    {
      ResultChannelIndex = c;
      break;
    }
  }
  return ResultChannelIndex;
}

int32_t
CalculateTotalAnimationGroupSize(const aiScene* Scene, const Anim::skeleton* Skeleton,
                                 float SamplingFrequency)
{
  int64_t Total = 0;

  Total += sizeof(Anim::animation_group);
  Total += sizeof(Anim::animation*) * Scene->mNumAnimations;

  for(int i = 0; i < Scene->mNumAnimations; i++)
  {
    aiAnimation* Animation = Scene->mAnimations[i];

    int KeyframeCount = KeyframeCount = GetAnimationKeyframeCount(Animation, SamplingFrequency);

    Total += sizeof(Anim::animation);
    Total += sizeof(transform) * Skeleton->BoneCount * KeyframeCount;
    Total += sizeof(float) * KeyframeCount;
  }

  return SafeTruncateUint64(Total);
}

void
PrintUsage()
{
  printf(
    "usage:\nbuilder input_file output_file_wo_ext [--root_bone name] [--scale value] "
    "[--print_scene] [--print_skeleton]"
    "[--sampling_frequency freq] [--target_actor actor_file] --model | --actor | --animation\n");
}

int
main(int ArgCount, char** Args)
{
  char* ModelName;
  char* ActorName;
  char* AnimationName;

  bool BuildModel             = false;
  bool BuildActor             = false;
  bool BuildAnimation         = false;
  bool PrintScene             = false;
  bool PrintSkeletonHierarchy = false;

  float RescaleCoefficient = 1.0f;
  float SamplingFrequency  = 120.0f;

  const char* RootBoneName    = "root";
  const char* TargetActorName = NULL;
  // Process command line arguments
  {
    // printf("%s\n", Args[1]);
    if(ArgCount < 3)
    {
      PrintUsage();
      return 1;
    }

    const char* ModelExtension     = ".model";
    const char* ActorExtension     = ".actor";
    const char* AnimationExtension = ".anim";

    size_t LenWithoutExtension = strlen(Args[2]);
    ModelName                  = (char*)malloc(LenWithoutExtension + strlen(ModelExtension) + 1);
    ActorName                  = (char*)malloc(LenWithoutExtension + strlen(ActorExtension) + 1);
    AnimationName = (char*)malloc(LenWithoutExtension + strlen(AnimationExtension) + 1);

    strcpy(ModelName, Args[2]);
    strcpy(ActorName, Args[2]);
    strcpy(AnimationName, Args[2]);

    strcpy(ModelName + LenWithoutExtension, ModelExtension);
    strcpy(ActorName + LenWithoutExtension, ActorExtension);
    strcpy(AnimationName + LenWithoutExtension, AnimationExtension);

    for(int ArgIndex = 3; ArgIndex < ArgCount; ArgIndex++)
    {
      if(strcmp(Args[ArgIndex], "--model") == 0)
      {
        BuildModel = true;
      }
      else if(strcmp(Args[ArgIndex], "--actor") == 0)
      {
        BuildActor = true;
      }
      else if(strcmp(Args[ArgIndex], "--animation") == 0)
      {
        BuildAnimation = true;
      }
      else if(strcmp(Args[ArgIndex], "--print_scene") == 0)
      {
        PrintScene = true;
      }
      else if(strcmp(Args[ArgIndex], "--print_skeleton") == 0)
      {
        PrintSkeletonHierarchy = true;
      }
      else if(strcmp(Args[ArgIndex], "--sampling_frequency") == 0)
      {
        if(ArgIndex + 1 < ArgCount)
        {
          SamplingFrequency = atoi(Args[ArgIndex + 1]);
          ++ArgIndex;
        }
        else
        {
          PrintUsage();
          return 1;
        }
      }
      else if(strcmp(Args[ArgIndex], "--root_bone") == 0)
      {
        if(ArgIndex + 1 < ArgCount)
        {
          RootBoneName = Args[ArgIndex + 1];
          ++ArgIndex;
        }
        else
        {
          PrintUsage();
          return 1;
        }
      }
      else if(strcmp(Args[ArgIndex], "--target_actor") == 0)
      {
        if(ArgIndex + 1 < ArgCount)
        {
          TargetActorName = Args[ArgIndex + 1];
          ++ArgIndex;
        }
        else
        {
          PrintUsage();
          return 1;
        }
      }
      else if(strcmp(Args[ArgIndex], "--scale") == 0)
      {
        if(ArgIndex + 1 < ArgCount)
        {
          RescaleCoefficient = (float)atof(Args[ArgIndex + 1]);
          ++ArgIndex;
        }
        else
        {
          PrintUsage();
          return 1;
        }
      }
      else
      {
        PrintUsage();
        return 1;
      }
    }
    if(!(BuildModel || BuildActor || BuildAnimation))
    {
      PrintUsage();
      return 1;
    }
  }

  if(TargetActorName)
  {
    bool QuitBuild = false;
    if(!BuildAnimation)
    {
      printf(
        "error: cannot have target_actor option without --animation option - quitting build\n");
      QuitBuild = true;
    }

    if(BuildActor)
    {
      printf("error: cannot have both target_actor and --actor options - quitting build\n");
      QuitBuild = true;
    }

    if(BuildModel)
    {
      printf("error: cannot have target_actor and --model option - quitting build\n");
      QuitBuild = true;
    }

    if(QuitBuild)
    {
      return 1;
    }
  }

  Assimp::Importer Importer;
  const aiScene*   Scene =
    Importer.ReadFile(Args[1], aiProcess_SplitLargeMeshes | aiProcess_Triangulate |
                                 aiProcess_GenNormals | aiProcess_FlipUVs |
                                 aiProcess_CalcTangentSpace);
  if(!Scene || !Scene->mRootNode)
  {
    printf("error::assimp: %s\n", Importer.GetErrorString());
    return 1;
  }
  if((!TargetActorName || (TargetActorName && Scene->mNumAnimations == 0)) &&
     (Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
  {
    printf("error: assimp scene is incomplete\n \"%s\"\n", Importer.GetErrorString());
  }

  if(PrintScene)
  {
    r_PrintNodeHieararchy(Scene->mRootNode, 0, aiMatrix4x4t<float>());
  }

  // Create Skeleton
  Anim::skeleton Skeleton = {};
  if(TargetActorName)
  {
    // TODO(Lukas) would better determine file size dynamically and only read in the fixed skeleton
    // and scale data
    const uint64_t MaximumActorSize = 1024 * 1024 * 10;
    void*          ActorMemory      = malloc(MaximumActorSize);

    Memory::stack_allocator TempActorAllocator = {};
    TempActorAllocator.Create(ActorMemory, MaximumActorSize);

    {
      debug_read_file_result ReadFile =
        Platform::ReadEntireFile(&TempActorAllocator, TargetActorName);
      if(ReadFile.Contents == NULL)
      {
        printf("error: target actor \"%s\" not found\n", TargetActorName);
        free(ActorMemory);
        return 1;
      }

      Render::model* ActorModel = (Render::model*)ReadFile.Contents;
      Asset::UnpackModel(ActorModel);
      if(!ActorModel->Skeleton)
      {
        printf("error: target actor \"%s\" has no skeleton\n", TargetActorName);
        free(ActorMemory);
        return 1;
      }

      Skeleton = *ActorModel->Skeleton;

      if(RescaleCoefficient != 1 && RescaleCoefficient != ActorModel->ScaleOnBuild)
      {
        printf(
          "Warning: --scale %.2f option will override scale %.2f found in --target_actor \"%s\"\n",
          (double)RescaleCoefficient, (double)ActorModel->ScaleOnBuild, TargetActorName);
      }

      if(RescaleCoefficient == 1)
      {
        RescaleCoefficient = ActorModel->ScaleOnBuild;
      }
    }

    free(ActorMemory);
  }
  else if(BuildActor || BuildAnimation)
  {
    aiMatrix4x4t<float> RootNodeTransform = {};
    aiNode*             RootBoneNode      = {};

    if(r_GetNodeAndGlobalMatrixByName(&RootBoneNode, &RootNodeTransform, Scene->mRootNode,
                                      aiMatrix4x4t<float>(), RootBoneName))
    {
      r_AddBindPoseBoneAndDescendantBonesToSkeleton(&Skeleton, RootBoneNode, RootNodeTransform);
      // Rescale skeleton
      for(int b = 0; b < Skeleton.BoneCount; b++)
      {
        Skeleton.Bones[b].BindPose.T *= RescaleCoefficient;
        Skeleton.Bones[b].InverseBindPose = Math::InvMat4(Skeleton.Bones[b].BindPose);
      }
    }
    else
    {
      printf("error: cannot build actor - no root bone named \"%s\" in file\n", RootBoneName);
      BuildActor = false;
    }
  }

  // Determine file size on disk
  int32_t TotalModelFileSize = CalculateTotalModelSize(Scene);
  int32_t TotalActorFileSize = TotalModelFileSize + SafeTruncateUint64(sizeof(Skeleton));
  int32_t TotalAnimFileSize = CalculateTotalAnimationGroupSize(Scene, &Skeleton, SamplingFrequency);

  int32_t MaximumAssetSize =
    (TotalActorFileSize > TotalAnimFileSize) ? TotalActorFileSize : TotalAnimFileSize;
  void* FileMemory = calloc(MaximumAssetSize, 1);

  Memory::stack_allocator Allocator = {};
  Allocator.Create(FileMemory, MaximumAssetSize);

  if(BuildModel || BuildActor)
  {
    // Reserve Space For Model
    Render::model* Model = PushStruct(&Allocator, Render::model);
    Model->ScaleOnBuild  = RescaleCoefficient;
    {
      // Reserve Space For Mesh Pointer Array
      Model->Meshes = PushArray(&Allocator, Scene->mNumMeshes, Render::mesh*);

      // Create The Actual Meshes
      r_ProcessNode(&Allocator, Scene->mRootNode, Scene, Model, &Skeleton, RescaleCoefficient,
                    false);
    }

    if(BuildModel)
    {
      // write '.model'
      assert(Allocator.GetUsedSize() == TotalModelFileSize);

      Model->Skeleton = 0;
      printf("writing: %s\n", ModelName);
      PrintModelHeader(Model);
      Asset::PackModel(Model);
      Platform::WriteEntireFile(ModelName, TotalModelFileSize, FileMemory);
      Asset::UnpackModel(Model);
    }

    if(BuildActor)
    {
      // write '.actor'
      Anim::skeleton* SkeletonPtr = PushStruct(&Allocator, Anim::skeleton);
      *SkeletonPtr                = Skeleton;
      Model->Skeleton             = SkeletonPtr;

      assert(Allocator.GetUsedSize() == TotalActorFileSize);

      printf("writing: %s\n", ActorName);
      if(PrintSkeletonHierarchy)
      {
        PrintSkeleton(&Skeleton);
      }
      Asset::PackModel(Model);
      Platform::WriteEntireFile(ActorName, TotalActorFileSize, FileMemory);
    }
  }

  if(BuildAnimation && 0 < Scene->mNumAnimations)
  {
    Allocator.Clear();

    Anim::animation_group* AnimGroup = PushStruct(&Allocator, Anim::animation_group);
    AnimGroup->AnimationCount        = Scene->mNumAnimations;

    AnimGroup->Animations = PushArray(&Allocator, AnimGroup->AnimationCount, Anim::animation*);

    for(int a = 0; a < Scene->mNumAnimations; a++)
    {
      const aiAnimation* AssimpAnimation = Scene->mAnimations[a];

      Anim::animation* Animation = PushStruct(&Allocator, Anim::animation);
      AnimGroup->Animations[a]   = Animation;

      float AnimStartTicks;
      float AnimEndTicks;
      Animation->KeyframeCount = GetAnimationKeyframeCount(AssimpAnimation, SamplingFrequency,
                                                           &AnimStartTicks, &AnimEndTicks);
      // printf("Keyframe Count: %d, Total Anim Size: %d, Total Allocated Asset Size: %d",
      // Animation->KeyframeCount, TotalAnimFileSize, MaximumAssetSize);
      float TicksPerKeyframe = (AnimEndTicks - AnimStartTicks) / Animation->KeyframeCount;

      Animation->Transforms =
        PushArray(&Allocator, Animation->KeyframeCount * Skeleton.BoneCount, transform);
      Animation->SampleTimes = PushArray(&Allocator, Animation->KeyframeCount, float);

      Animation->ChannelCount = Skeleton.BoneCount;

      for(int b = 0; b < Skeleton.BoneCount; b++)
      {
        bool BoneAnimFound = false;
        for(int c = 0; c < AssimpAnimation->mNumChannels; c++)
        {
          const aiNodeAnim* Channel = AssimpAnimation->mChannels[c];
          if(strcmp(Skeleton.Bones[b].Name, Channel->mNodeName.C_Str()) == 0)
          {
            BoneAnimFound = true;
            break;
          }
        }
        if(!BoneAnimFound)
        {
          transform IdentityTransform = {};
          IdentityTransform.R         = Math::QuatIdent();
          IdentityTransform.S         = { 1, 1, 1 };
          for(int i = 0; i < Animation->KeyframeCount; i++)
          {
            Animation->Transforms[i * Skeleton.BoneCount + b] = IdentityTransform;
          }
        }
      }

      // Process animation data
      for(int c = 0; c < AssimpAnimation->mNumChannels; c++)
      {
        const aiNodeAnim* Channel = AssimpAnimation->mChannels[c];

        int BoneIndex = GetBoneIndexFromName(&Skeleton, Channel->mNodeName.C_Str());
        if(BoneIndex == -1)
        {
          printf("Skipping anim channel: \"%s\", not found in skeleton\n",
                 Channel->mNodeName.C_Str());
          continue;
        }

#ifdef USE_BIND_POSE
        int ParentBoneIndex = Skeleton.Bones[BoneIndex].ParentIndex;

        mat4 ParentInverseBindPose = (ParentBoneIndex == -1)
                                       ? Math::Mat4Ident()
                                       : Skeleton.Bones[ParentBoneIndex].InverseBindPose;
        mat4 LocalBoneInvBindPose =
          Math::InvMat4(Math::MulMat4(ParentInverseBindPose, Skeleton.Bones[BoneIndex].BindPose));

        aiMatrix4x4t<float> AssimpLocalBoneInvBindPose =
          Convert_mat4_To_aiMatrix4x4(LocalBoneInvBindPose);
#endif

        // assert(Channel->mNumRotationKeys == Channel->mNumPositionKeys);
        assert(Channel->mNumScalingKeys >= 1);
        int32_t CurrT = 0;
        int32_t NextT = (int32_t)Max(0, Min(Channel->mNumPositionKeys - 1, 1));

        int32_t CurrR = 0;
        int32_t NextR = (int32_t)Max(0, Min(Channel->mNumRotationKeys - 1, 1));

        int32_t CurrS = 0;
        int32_t NextS = (int32_t)Max(0, Min(Channel->mNumScalingKeys - 1, 1));
        // assert(Channel->mNumRotationKeys == Animation->KeyframeCount);
        for(int i = 0; (float)i < Animation->KeyframeCount; i++)
        {
          float CurrentTick = AnimStartTicks + ((float)i) * TicksPerKeyframe;
          {
            while(Channel->mPositionKeys[NextT].mTime < CurrentTick &&
                  NextT + 1 < Channel->mNumPositionKeys)
            {
              ++CurrT;
              ++NextT;
            }

            while(Channel->mRotationKeys[NextR].mTime < CurrentTick &&
                  NextR + 1 < Channel->mNumRotationKeys)
            {
              ++CurrR;
              ++NextR;
            }

            while(Channel->mScalingKeys[NextS].mTime < CurrentTick &&
                  NextS + 1 < Channel->mNumScalingKeys)
            {
              ++CurrS;
              ++NextS;
            }
          }

          transform LocalBoneKeyTransform = {};

          {
            float tT;
            {
              float T0 = Channel->mPositionKeys[CurrT].mTime;
              float T1 = Channel->mPositionKeys[NextT].mTime;
              tT       = Clamp((CurrentTick - T0) / (T1 - T0), 0, 1);
              assert(0 <= tT && tT <= 1);
            }
            vec3 FirstT             = { Channel->mPositionKeys[CurrT].mValue.x,
                            Channel->mPositionKeys[CurrT].mValue.y,
                            Channel->mPositionKeys[CurrT].mValue.z };
            vec3 SecondT            = { Channel->mPositionKeys[NextT].mValue.x,
                             Channel->mPositionKeys[NextT].mValue.y,
                             Channel->mPositionKeys[NextT].mValue.z };
            LocalBoneKeyTransform.T = RescaleCoefficient * (FirstT * (1 - tT) + SecondT * tT);
          }

          {
            float tR;
            {
              float R0 = Channel->mRotationKeys[CurrR].mTime;
              float R1 = Channel->mRotationKeys[NextR].mTime;
              tR       = Clamp((CurrentTick - R0) / (R1 - R0), 0, 1);
              assert(0 <= tR && tR <= 1);
            }
            quat FirstR = {};
            FirstR.S = Channel->mRotationKeys[CurrR].mValue.w;
            FirstR.V = { Channel->mRotationKeys[CurrR].mValue.x,
                         Channel->mRotationKeys[CurrR].mValue.y,
                         Channel->mRotationKeys[CurrR].mValue.z };

            quat SecondR = {};
            SecondR.S = Channel->mRotationKeys[NextR].mValue.w;
            SecondR.V = { Channel->mRotationKeys[NextR].mValue.x,
                          Channel->mRotationKeys[NextR].mValue.y,
                          Channel->mRotationKeys[NextR].mValue.z };

            LocalBoneKeyTransform.R = Math::QuatLerp(FirstR, SecondR, tR);
          }

          // TODO(Lukas) make scaling also use proper sampling
          {
            float tS;
            {
              float S0 = Channel->mScalingKeys[CurrS].mTime;
              float S1 = Channel->mScalingKeys[NextS].mTime;
              tS       = Clamp((CurrentTick - S0) / (S1 - S0), 0, 1);
              assert(0 <= tS && tS <= 1);
            }

            LocalBoneKeyTransform.S.X = Channel->mScalingKeys[0].mValue.x;
            LocalBoneKeyTransform.S.Y = Channel->mScalingKeys[0].mValue.y;
            LocalBoneKeyTransform.S.Z = Channel->mScalingKeys[0].mValue.z;
            // assert(LocalBoneKeyTransform.S.X == 1.0f && LocalBoneKeyTransform.S.Y == 1.0f &&
            // LocalBoneKeyTransform.S.Z == 1.0f);
          }

#ifdef USE_BIND_POSE

          mat4 LocalBoneKeyMat      = TransformToMat4(LocalBoneKeyTransform);
          mat4 BoundLocalBoneKeyMat = Math::MulMat4(LocalBoneInvBindPose, LocalBoneKeyMat);

          transform BindRelativeLocalBoneKeyTransform;
          {
            // Extract bone space translation
            BindRelativeLocalBoneKeyTransform.R = Math::Mat4ToQuat(BoundLocalBoneKeyMat);
            BindRelativeLocalBoneKeyTransform.T = BoundLocalBoneKeyMat.T;
            BindRelativeLocalBoneKeyTransform.S = { 1, 1, 1 };
          }

          aiMatrix4x4t<float> AssimpLocalBoneKeyMat = Convert_mat4_To_aiMatrix4x4(LocalBoneKeyMat);
          aiMatrix4x4t<float> AssimpBoundLocalBoneKeyMat =
            AssimpLocalBoneInvBindPose * AssimpLocalBoneKeyMat;
          {
            aiVector3t<float>    scaling;
            aiQuaterniont<float> rotation;
            aiVector3t<float>    position;
            AssimpBoundLocalBoneKeyMat.Decompose(scaling, rotation, position);

            {
              BindRelativeLocalBoneKeyTransform.R.S   = rotation.w;
              BindRelativeLocalBoneKeyTransform.R.V.X = rotation.x;
              BindRelativeLocalBoneKeyTransform.R.V.Y = rotation.y;
              BindRelativeLocalBoneKeyTransform.R.V.Z = rotation.z;
            }
            {
              BindRelativeLocalBoneKeyTransform.T.X = position.x;
              BindRelativeLocalBoneKeyTransform.T.Y = position.y;
              BindRelativeLocalBoneKeyTransform.T.Z = position.z;
            }
            {
              BindRelativeLocalBoneKeyTransform.S.X = scaling.x;
              BindRelativeLocalBoneKeyTransform.S.Y = scaling.y;
              BindRelativeLocalBoneKeyTransform.S.Z = scaling.z;
            }
          }

#else  // USE_BIND_POSE
          transform BindRelativeLocalBoneKeyTransform = LocalBoneKeyTransform;
#endif // USE_BIND_POSE

          Animation->Transforms[i * Skeleton.BoneCount + BoneIndex] =
            BindRelativeLocalBoneKeyTransform;

          // Note: continuous overwriting with the same data
          Animation->SampleTimes[i] = CurrentTick / AssimpAnimation->mTicksPerSecond;
        }
      }
    }

    assert(Allocator.GetUsedSize() == TotalAnimFileSize);
    printf("writing: %s\n", AnimationName);
    Asset::PackAnimationGroup(AnimGroup);
    Platform::WriteEntireFile(AnimationName, TotalAnimFileSize, FileMemory);
  }

  free(ModelName);
  free(ActorName);
  free(AnimationName);
  free(FileMemory);
  return 0;
}
