#pragma once

#include <stdint.h>
#include <stdio.h>

#include "linear_math/matrix.h"

#define BONE_NAME_LENGTH 20
#define SKELETON_MAX_BONE_COUNT 20

namespace Anim
{
  struct bone
  {
    char Name[BONE_NAME_LENGTH];
    mat4 BindPose;
    mat4 InverseBindPose;

    int32_t ParentIndex;
    int32_t Index;
  };

  struct skeleton // root is always the 0'th bone
  {
    int32_t BoneCount;
    bone    Bones[SKELETON_MAX_BONE_COUNT];
  };

  void
  AddBone(Anim::skeleton* Skeleton, Anim::bone Bone)
  {
    assert(Skeleton->BoneCount < SKELETON_MAX_BONE_COUNT);
    assert(Skeleton->BoneCount >= 0);

    Skeleton->Bones[Skeleton->BoneCount++] = Bone;
  }

  int
  r_GetBoneDepth(const skeleton* Skeleton, int BoneIndex)
  {
    assert(BoneIndex >= 0 && BoneIndex < Skeleton->BoneCount);

    int ParentIndex = Skeleton->Bones[BoneIndex].ParentIndex;
    if(ParentIndex < 0)
    {
      return 0;
    }
    return 1 + r_GetBoneDepth(Skeleton, ParentIndex);
  }

  void
  PrintSkeleton(const skeleton* Skeleton)
  {
    for(int i = 0; i < Skeleton->BoneCount; i++)
    {
      const bone* Bone = &Skeleton->Bones[i];

      int BoneDepth = r_GetBoneDepth(Skeleton, i);
      for(int d = 0; d < BoneDepth; d++)
      {
        printf("  ");
      }
      printf("%d: %s #%d, child of #%d\n", i, Bone->Name, Bone->Index, Bone->ParentIndex);
    }
  }

  int32_t
  GetBoneIndexFromName(const skeleton* Skeleton, const char* Name)
  {
    for(int i = 0; i < Skeleton->BoneCount; i++)
    {
      if(strcmp(Skeleton->Bones[i].Name, Name) == 0)
      {
        return i;
      }
    }
    return -1;
  }
}
