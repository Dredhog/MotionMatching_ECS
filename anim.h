#pragma once

#include <stdint.h>

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "skeleton.h"

static const int ANIM_CONTROLLER_MAX_ANIM_COUNT     = 5;
static const int ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT = 1;

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
    float PlaybackRateSec;
    bool  IsPlaying;
    bool  Loop;
  };

  struct animation_controller
  {
    animation_state  States[ANIM_CONTROLLER_MAX_ANIM_COUNT];
    animation*       Animations[ANIM_CONTROLLER_MAX_ANIM_COUNT];
    skeleton*        Skeleton;
    Anim::transform* OutputTransforms;
    mat4*            BoneSpaceMatrices;
    mat4*            ModelSpaceMatrices;
    mat4*            HierarchicalModelSpaceMatrices;
    float            GlobalTimeSec;
    int32_t          AnimStateCount;
  };

  struct animation_group
  {
    animation** Animations;
    int32_t     AnimationCount;
  };

  // Keyframe blending and sampling facilities
  void LerpTransforms(const Anim::transform* InA, const Anim::transform* InB, int TransformCount,
                      float T, Anim::transform* Out);
  // Animation
  void LinearAnimationSample(const Anim::animation* Animation, float Time,
                             Anim::transform* OutputTransforms);
  void SampleAtGlobalTime(Anim::animation_controller* Controller, int AnimationIndex,
                          int OutputBlockIndex);

  // Animation controller interface
  void UpdateController(Anim::animation_controller* Controller, float dt);
  void AddAnimation(Anim::animation_controller* AnimController, Anim::animation* Animation);
  void SetAnimation(Anim::animation_controller* AnimController, Anim::animation* Animation,
                    int32_t ControllerIndex);
  void StartAnimationAtIndex(Anim::animation_controller* AnimController, int Index, float Time);
  void StartAnimationAtGlobalTime(Anim::animation_controller* AnimController, int AnimationIndex,
                                  bool Loop = true);

  // Matrix palette generation
  void ComputeBoneSpacePoses(mat4* BoneSpaceMatrices, const Anim::transform* Transforms,
                             int32_t Count);
  void ComputeModelSpacePoses(mat4* ModelSpacePoses, const mat4* BoneSpaceMatrices,
                              const Anim::skeleton* Skeleton);
  void ComputeFinalHierarchicalPoses(mat4* FinalPoseMatrices, const mat4* ModelSpaceMatrices,
                                     const Anim::skeleton* Skeleton);
}
