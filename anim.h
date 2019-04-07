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

  // TODO(Lukas): Move this to the asset/serialization system
  struct animation_group
  {
    animation** Animations;
    int32_t     AnimationCount;
  };

  struct int32_pair
  {
    int32_t a;
    int32_t b;
  };

  struct skeleton_mirror_info
  {
    int32_pair BoneMirrorIndices[SKELETON_MAX_BONE_COUNT];
    int32_t    BoneCount;
    vec3       MirrorBasisScales;
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
  void SampleAtGlobalTime(Anim::animation_controller* Controller, int AnimationIndex,
                          int OutputBlockIndex, const skeleton_mirror_info* MirrorInfo = NULL);
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

  // Blending and sampling facilities
  void LerpTransforms(const Anim::transform* InA, const Anim::transform* InB, int TransformCount,
                      float T, Anim::transform* Out);
  void AddTransforms(const Anim::transform* InA, const Anim::transform* InB, int TransformCount,
                     float T, Anim::transform* Out);
  void LinearAnimationSample(Anim::transform* OutputTransforms, const Anim::animation* Animation,
                             float Time);
  void LinearAnimationSample(animation_controller*, int AnimAInd, float Time, int ResultIndex);
  void LinearMirroredAnimationSample(Anim::transform* OutputTransforms, mat4* TempMatrices,
                                     const skeleton* Skeleton, const Anim::animation* Animation,
                                     float Time, const Anim::skeleton_mirror_info* MirrorInfo);
  void LinearMirroredAnimationSample(Anim::animation_controller* Controller, int AnimIndex,
                                     float Time, int ResultIndex,
                                     const skeleton_mirror_info* MirrorInfo);
  Anim::transform LinearAnimationBoneSample(const Anim::animation* Animation, int BoneIndex,
                                            float Time);

  // Matrix palette generation
  void ComputeBoneSpacePoses(mat4* BoneSpaceMatrices, const Anim::transform* Transforms,
                             int32_t Count);
  void ComputeModelSpacePoses(mat4* ModelSpacePoses, const mat4* BoneSpaceMatrices,
                              const Anim::skeleton* Skeleton);
  void ComputeFinalHierarchicalPoses(mat4* FinalPoseMatrices, const mat4* ModelSpaceMatrices,
                                     const Anim::skeleton* Skeleton);
  void InverseComputeBoneSpacePoses(Anim::transform* Transforms, const mat4* BoneSpaceMatrices,
                                    int Count);
  void InverseComputeModelSpacePoses(mat4* BoneSpaceMatrices, const mat4* ModelSpaceMatrices,
                                     const Anim::skeleton* Skeleton);

  // Helper functions
  float GetLocalSampleTime(const Anim::animation_controller* Controller, int AnimationIndex,
                           float GlobalTimeSec);
  void  GetRootAndInvRootMatrices(mat4* OutRootMatrix, mat4* OutInvRoot, mat4 HipMatrix);
  float GetAnimDuration(const Anim::animation* Animation);

  inline mat4
  TransformToMat4(const Anim::transform& Transform)
  {
    mat4 Result = Math::MulMat4(Math::Mat4Translate(Transform.Translation),
                                Math::MulMat4(Math::Mat4Rotate(Transform.Rotation),
                                              Math::Mat4Scale(Transform.Scale)));
    return Result;
  }
}
