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

#include "stack_alloc.h"
#include "stack_alloc.cpp"

#include "heap_alloc.h"
#include "heap_alloc.cpp"

#include "file_io.h"

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
r_AddBoneAndDescendantBonesToSkeleton(aiNode* Node, Anim::skeleton* Skeleton)
{
  assert(Skeleton->BoneCount >= 0 && Skeleton->BoneCount < SKELETON_MAX_BONE_COUNT);

  assert(Node->mName.length <= BONE_NAME_LENGTH);
  Anim::bone Bone = {};
  strcpy(Bone.Name, Node->mName.C_Str());
  Bone.Index = Skeleton->BoneCount;

#ifndef USE_BIND_POSE
  Bone.BindPose        = Math::Mat4Ident();
  Bone.InverseBindPose = Math::Mat4Ident();
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
    r_AddBoneAndDescendantBonesToSkeleton(Node->mChildren[i], Skeleton);
  }
}

aiNode*
r_FindSkeletonRootInSubtre(aiNode* Node, const char* RootBoneName)
{
  if(strcmp(Node->mName.C_Str(), RootBoneName) == 0)
  {
    return Node;
  }
  for(int i = 0; i < Node->mNumChildren; i++)
  {
    aiNode* Result = r_FindSkeletonRootInSubtre(Node->mChildren[i], RootBoneName);
    if(Result)
    {
      return Result;
    }
  }
  return 0;
}

Render::mesh
ProcessMesh(Memory::stack_allocator* Alloc, const aiMesh* AssimpMesh, Anim::skeleton* Skeleton,
            float SpaceScale, bool ExtractBindPose)
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
    // TODO(Lukas) Remove the ExtractBindPose it should not be here, no verts get bone weights right
    // now
    for(int i = 0; i < AssimpMesh->mNumBones && ExtractBindPose; i++)
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
              Render::model* Model, Anim::skeleton* Skeleton, float SpaceScale,
              bool ExtractBindPose = false)
{
  for(int i = 0; i < Node->mNumMeshes; i++)
  {
    Render::mesh* Mesh = PushStruct(Alloc, Render::mesh);
    *Mesh =
      ProcessMesh(Alloc, Scene->mMeshes[Node->mMeshes[i]], Skeleton, SpaceScale, ExtractBindPose);

    Model->Meshes[Model->MeshCount++] = Mesh;
  }
  for(int i = 0; i < Node->mNumChildren; i++)
  {
    r_ProcessNode(Alloc, Node->mChildren[i], Scene, Model, Skeleton, SpaceScale, ExtractBindPose);
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

int32_t
CalculateTotalAnimationGroupSize(const aiScene* Scene, const Anim::skeleton* Skeleton)
{
  int64_t Total = 0;

  Total += sizeof(Anim::animation_group);
  Total += sizeof(Anim::animation*) * Scene->mNumAnimations;
  for(int i = 0; i < Scene->mNumAnimations; i++)
  {
    aiAnimation* Animation = Scene->mAnimations[i];

    Total += sizeof(Anim::animation);
    Total +=
      sizeof(Anim::transform) * Skeleton->BoneCount * Animation->mChannels[0]->mNumRotationKeys;
    Total += sizeof(float) * Animation->mChannels[0]->mNumRotationKeys;
  }

  return SafeTruncateUint64(Total);
}

void
PrintUsage()
{
  printf(
    "usage:\nbuilder input_file output_name [--root_bone name] [--scale value] --model | --actor | --animation\n");
}

int
main(int ArgCount, char** Args)
{
  char* ModelName;
  char* ActorName;
  char* AnimationName;

  bool BuildModel     = false;
  bool BuildActor     = false;
  bool BuildAnimation = false;

  float RescaleCoefficient = 1.0f;

  const char* RootBoneName = "root";
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

  Assimp::Importer Importer;
  const aiScene*   Scene =
    Importer.ReadFile(Args[1], aiProcess_SplitLargeMeshes | aiProcess_Triangulate |
                                 aiProcess_GenNormals | aiProcess_FlipUVs |
                                 aiProcess_CalcTangentSpace);
  if(!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
  {
    printf("error::assimp: %s\n", Importer.GetErrorString());
    return 1;
  }

  // Create Skeleton
  Anim::skeleton Skeleton = {};
  if(BuildActor || BuildAnimation)
  {
    aiNode* RootBoneNode = r_FindSkeletonRootInSubtre(Scene->mRootNode, RootBoneName);

    if(RootBoneNode)
    {
      r_AddBoneAndDescendantBonesToSkeleton(RootBoneNode, &Skeleton);
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
  int32_t TotalAnimFileSize  = CalculateTotalAnimationGroupSize(Scene, &Skeleton);

  int32_t MaximumAssetSize =
    (TotalActorFileSize > TotalAnimFileSize) ? TotalActorFileSize : TotalAnimFileSize;
  void* FileMemory = calloc(MaximumAssetSize, 1);

  Memory::stack_allocator Allocator = {};
  Allocator.Create(FileMemory, MaximumAssetSize);

  if(BuildModel || BuildActor)
  {
    // Reserve Space For Model
    Render::model* Model = PushStruct(&Allocator, Render::model);
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
      PrintSkeleton(&Skeleton);
      printf("\n");
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
      assert(1 < AssimpAnimation->mTicksPerSecond);

      Anim::animation* Animation = PushStruct(&Allocator, Anim::animation);
      AnimGroup->Animations[a]   = Animation;
      Animation->KeyframeCount   = AssimpAnimation->mChannels[0]->mNumRotationKeys;

      Animation->Transforms =
        PushArray(&Allocator, Animation->KeyframeCount * Skeleton.BoneCount, Anim::transform);
      Animation->SampleTimes = PushArray(&Allocator, Animation->KeyframeCount, float);

      // Build the skeleton bind pose
      for(int BoneIndex = 0; BoneIndex < Skeleton.BoneCount; BoneIndex++)
      {
        bool FoundBone = false;
        for(int c = 0; c < AssimpAnimation->mNumChannels; c++)
        {
          const aiNodeAnim* Channel = AssimpAnimation->mChannels[c];

          if(strcmp(Skeleton.Bones[BoneIndex].Name, Channel->mNodeName.C_Str()) == 0)
          {
            FoundBone                    = true;
            aiQuaternion BoneQuat        = Channel->mRotationKeys[0].mValue;
            aiVector3D   BoneScale       = Channel->mScalingKeys[0].mValue;
            aiVector3D   BoneTranslation = Channel->mPositionKeys[0].mValue;

            mat4 LocalBindPose = {};
            {
              Anim::transform LocalTransform = {};
              LocalTransform.Rotation        = { BoneQuat.w, BoneQuat.x, BoneQuat.y, BoneQuat.z };
              LocalTransform.Scale           = { BoneScale.x, BoneScale.y, BoneScale.z };
              LocalTransform.Translation =
                RescaleCoefficient * vec3{ BoneTranslation.x, BoneTranslation.y, BoneTranslation.z };

              LocalBindPose = Anim::TransformToMat4(&LocalTransform);
            }

            int  ParentIndex = Skeleton.Bones[BoneIndex].ParentIndex;
            mat4 ParentBindPose =
              (ParentIndex == -1) ? Math::Mat4Ident() : Skeleton.Bones[ParentIndex].BindPose;
            Skeleton.Bones[BoneIndex].BindPose = Math::MulMat4(ParentBindPose, LocalBindPose);
            Skeleton.Bones[BoneIndex].InverseBindPose =
              Math::InvMat4(Skeleton.Bones[BoneIndex].BindPose);
          }
        }

        if(!FoundBone)
        {

          // Skeleton.Bones[BoneIndex].BindPose        = Math::Mat4Ident();
          // Skeleton.Bones[BoneIndex].InverseBindPose = Math::Mat4Ident();
          printf("No animation for bone \"%s\" found\n", Skeleton.Bones[BoneIndex].Name);

#ifdef USE_BIND_POSE
          {
            printf("Removing \"%s\" from animation hierarchy\n", Skeleton.Bones[BoneIndex].Name);
            SimpleRemoveBoneFromSkeleton(&Skeleton, BoneIndex);
            --BoneIndex;
          }
#endif
        }
      }

      Animation->ChannelCount = Skeleton.BoneCount;

      // Process animation data
      for(int b = 0; b < AssimpAnimation->mNumChannels; b++)
      {
        const aiNodeAnim* Channel = AssimpAnimation->mChannels[b];

        int BoneIndex = GetBoneIndexFromName(&Skeleton, Channel->mNodeName.C_Str());
        assert(BoneIndex != -1);

#ifdef USE_BIND_POSE
        int ParentBoneIndex = Skeleton.Bones[BoneIndex].ParentIndex;

        mat4 ParentInverseBindPose = (ParentBoneIndex == -1)
                                       ? Math::Mat4Ident()
                                       : Skeleton.Bones[ParentBoneIndex].InverseBindPose;
        mat4 InverseLocalBindPose =
          Math::InvMat4(Math::MulMat4(ParentInverseBindPose, Skeleton.Bones[BoneIndex].BindPose));
#endif

        for(int i = 0; i < Channel->mNumRotationKeys; i++)
        {
          Anim::transform BoneParentSpaceAbsTransform = {};
          int             TranslationKeyIndex         = (b == 0) ? i : 0;
          BoneParentSpaceAbsTransform.Translation.X =
            RescaleCoefficient * Channel->mPositionKeys[TranslationKeyIndex].mValue.x;
          BoneParentSpaceAbsTransform.Translation.Y =
            RescaleCoefficient * Channel->mPositionKeys[TranslationKeyIndex].mValue.y;
          BoneParentSpaceAbsTransform.Translation.Z =
            RescaleCoefficient * Channel->mPositionKeys[TranslationKeyIndex].mValue.z;

          BoneParentSpaceAbsTransform.Scale.X = Channel->mScalingKeys[0].mValue.x;
          BoneParentSpaceAbsTransform.Scale.Y = Channel->mScalingKeys[0].mValue.y;
          BoneParentSpaceAbsTransform.Scale.Z = Channel->mScalingKeys[0].mValue.z;

          BoneParentSpaceAbsTransform.Rotation = { Channel->mRotationKeys[i].mValue.w,
                                                   Channel->mRotationKeys[i].mValue.x,
                                                   Channel->mRotationKeys[i].mValue.y,
                                                   Channel->mRotationKeys[i].mValue.z };

#ifdef USE_BIND_POSE
          mat4 BoneParentSpaceAbsPose = Anim::TransformToMat4(&BoneParentSpaceAbsTransform);
          mat4 BindRelativeLocalBonePose =
            Math::MulMat4(InverseLocalBindPose, BoneParentSpaceAbsPose);
          Anim::transform BindRelativeLocalBoneTransform = {};
          {
            BindRelativeLocalBoneTransform.Translation =
              Math::GetMat4Translation(BindRelativeLocalBonePose);
            BindRelativeLocalBoneTransform.Scale =
              Math::Mat4GetScaleAndNormalize(&BindRelativeLocalBonePose);
            {
              const mat4& R = BindRelativeLocalBonePose;
              // assert(isRotationMatrix(R));

              float sy = sqrt(R._11 * R._11 + R._21 * R._21);

              bool singular = sy < 1e-6f; // If

              float x, y, z;
              if(!singular)
              {
                x = atan2f(R._32, R._33);
                y = atan2f(-R._31, sy);
                z = atan2f(R._21, R._11);
              }
              else
              {
                x = atan2f(-R._23, R._22);
                y = atan2f(-R._31, sy);
                z = 0;
              }
              BindRelativeLocalBoneTransform.Rotation = { x, y, z };
            }
          }
#else  // USE_BIND_POSE
          Anim::transform BindRelativeLocalBoneTransform = BoneParentSpaceAbsTransform;
#endif // USE_BIND_POSE

          Animation->Transforms[i * Skeleton.BoneCount + BoneIndex] =
            BindRelativeLocalBoneTransform;

          // Note: continuous overwriting with the same data
          Animation->SampleTimes[i] =
            (float)(Channel->mRotationKeys[i].mTime / AssimpAnimation->mTicksPerSecond);
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
