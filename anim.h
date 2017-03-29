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

  struct animation
  {
    transform* Transforms;
    float*     SampleTimesSec;
    int32_t    KeyframeCount;
    int32_t    BonesPerPose;
  };

  struct animation_state
  {
    float StartTimeSec;
    float playbackRateSec;
  };

  struct animation_controller
  {
    animation**       Animations;
    animation_state* AnimationStates;
    skeleton*         Skeleton;
  };

  void ComputeBoneSpacePoses(mat4* BoneSpaceMatrices, const Anim::transform* Transforms,
                             int32_t Count);
  void ComputeModelSpacePoses(mat4* ModelSpacePoses, const mat4* BoneSpaceMatrices,
                              const Anim::skeleton* Skeleton);
  void ComputeFinalHierarchicalPoses(mat4* FinalPoseMatrices, const mat4* ModelSpaceMatrices,
                                     const Anim::skeleton* Skeleton);
}
