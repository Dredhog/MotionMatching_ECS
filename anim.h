#pragma once

#include <stdint.h>

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "skeleton.h"

static const int ANIM_CONTROLLER_MAX_ANIM_COUNT = 5;

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
    float*     SampleTimes;
    int32_t    KeyframeCount;
    int32_t    ChannelCount;
  };

  struct animation_state
  {
    float StartTimeSec;
    float playbackRateSec;
    bool  IsPlaying;
    bool  Loop;
  };

  struct animation_controller
  {
    animation_state States[ANIM_CONTROLLER_MAX_ANIM_COUNT];
    animation*      Animations[ANIM_CONTROLLER_MAX_ANIM_COUNT];
    skeleton*       Skeleton;
    mat4*           BoneSpacePoses;
    mat4*           ModelSpacePoses;
    mat4*           FinalHierarchicalPoses;
    int32_t         AnimStateCount;
  };

  struct animation_group
  {
    animation** Animations;
    int32_t     AnimationCount;
  };

  void ComputeBoneSpacePoses(mat4* BoneSpaceMatrices, const Anim::transform* Transforms,
                             int32_t Count);
  void ComputeModelSpacePoses(mat4* ModelSpacePoses, const mat4* BoneSpaceMatrices,
                              const Anim::skeleton* Skeleton);
  void ComputeFinalHierarchicalPoses(mat4* FinalPoseMatrices, const mat4* ModelSpaceMatrices,
                                     const Anim::skeleton* Skeleton);

  void AddAnimation(Anim::animation_controller* AnimController, Anim::animation* Animation);
  void StartAnimationAtIndex(Anim::animation_controller* AnimController, int Index, float Time);
}
