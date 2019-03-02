#pragma once

#include <stdint.h>
#include "rid.h"
#include "linear_math/matrix.h"
#include "linear_math/vector.h"
#include "linear_math/quaternion.h"

#include "skeleton.h"

static const int ANIM_CONTROLLER_MAX_ANIM_COUNT     = 5;
static const int ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT = 3;

namespace Anim
{
  struct transform
  {
    quat Rotation;
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

	//Note(Lukas): Move this to the asset/serialization system
  struct animation_group
  {
    animation** Animations;
    int32_t     AnimationCount;
  };

  struct animation_controller
  {
    skeleton*       Skeleton;
    animation_state States[ANIM_CONTROLLER_MAX_ANIM_COUNT];
    rid             AnimationIDs[ANIM_CONTROLLER_MAX_ANIM_COUNT];
    animation*      Animations[ANIM_CONTROLLER_MAX_ANIM_COUNT];

    Anim::transform* OutputTransforms;
    mat4*            BoneSpaceMatrices;
    mat4*            ModelSpaceMatrices;
    mat4*            HierarchicalModelSpaceMatrices;
    float            GlobalTimeSec;
    int32_t          AnimStateCount;

    void (*BlendFunc)(animation_controller*);
  };

  // Sampling / Blending
  void SampleAtGlobalTime(animation_controller*, int AnimationIndex, int OutputBlockIndex);
  void LinearBlend(animation_controller*, int AnimA, int AnimB, float t, int ResultInd);
  void AdditiveBlend(animation_controller*, int AnimBase, int AnimAdd, float t, int ResultInd);
  void UpdateController(animation_controller*, float dt,
                        void BlendFunction(animation_controller*) = NULL);

  // Animation controller interface
  void AppendAnimation(animation_controller*, rid AnimationID);
  void SetAnimation(animation_controller*, rid AnimationID, int32_t AnimationIndex);

  void StopAnimation(animation_controller*, int AnimationIndex);
  void StartAnimationAtGlobalTime(animation_controller*, int AnimationIndex, bool Loop = true,
                                  float LocalStartTime = 0.0f);
  void StartAnimationAtGlobalTime(Anim::animation_controller* Controller, int AnimationIndex,
                                   bool Loop, float LocalStartTime);
    /*void SetLooping(animation_controller*, Anim::animation_controller* AnimController,
                    int AnimationIndex, bool Loop = true);*/
    // void SetPlaybackRate(animation_controller*, int32_t Index, float Rate);

    // Blending and sampling facilities
    void LerpTransforms(const Anim::transform* InA, const Anim::transform* InB, int TransformCount,
                        float T, Anim::transform* Out);
  void AddTransforms(const Anim::transform* InA, const Anim::transform* InB, int TransformCount,
                     float T, Anim::transform* Out);
  void LinearAnimationSample(animation_controller*, int AnimAInd, float Time, int ResultIndex);

  // Matrix palette generation
  void ComputeBoneSpacePoses(mat4* BoneSpaceMatrices, const Anim::transform* Transforms,
                             int32_t Count);
  void ComputeModelSpacePoses(mat4* ModelSpacePoses, const mat4* BoneSpaceMatrices,
                              const Anim::skeleton* Skeleton);
  void ComputeFinalHierarchicalPoses(mat4* FinalPoseMatrices, const mat4* ModelSpaceMatrices,
                                     const Anim::skeleton* Skeleton);

  inline mat4
  TransformToMat4(const Anim::transform* Transform)
  {
    mat4 Result = Math::MulMat4(Math::Mat4Translate(Transform->Translation),
                                Math::MulMat4(Math::Mat4Rotate(Transform->Rotation),
                                              Math::Mat4Scale(Transform->Scale)));
    return Result;
  }
}
