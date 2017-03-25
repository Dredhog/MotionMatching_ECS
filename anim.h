#pragma once

#include <stdint.h>

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "skeleton.h"

namespace Anim
{
  struct transform
  {
    vec3 Rotation;
    vec3 Translation;
    vec3 Scale;
  };

  struct keyframe
  {
    transform Transforms[SKELETON_MAX_BONE_COUNT];
    int32_t   TransformCout;
  };

  void ComputeBoneSpacePoses(mat4* BoneSpaceMatrices, const Anim::transform* Transforms,
                             int32_t Count);
  void ComputeModelSpacePoses(mat4* ModelSpacePoses, const mat4* BoneSpaceMatrices,
                              const Anim::skeleton* Skeleton);
  void ComputeFinalHierarchicalPoses(mat4* FinalPoseMatrices, const mat4* ModelSpaceMatrices,
                                     const Anim::skeleton* Skeleton);
}
