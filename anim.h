#pragma once

#include <stdint.h>
#include "rid.h"
#include "linear_math/matrix.h"
#include "linear_math/vector.h"
#include "linear_math/quaternion.h"

#include "skeleton.h"
#include "transform.h"

static const int ANIM_PLAYER_MAX_ANIM_COUNT     = 5;
static const int ANIM_PLAYER_OUTPUT_BLOCK_COUNT = 3;

namespace Anim
{
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
    bool  Loop;
    bool  Mirror;
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
    int32_t    PairCount;
    vec3       MirrorBasisScales;
  };

  struct animation_player
  {
    skeleton*       Skeleton;
    animation_state States[ANIM_PLAYER_MAX_ANIM_COUNT];
    rid             AnimationIDs[ANIM_PLAYER_MAX_ANIM_COUNT];
    animation*      Animations[ANIM_PLAYER_MAX_ANIM_COUNT];

    transform* OutputTransforms;
    mat4*      BoneSpaceMatrices;
    mat4*      ModelSpaceMatrices;
    mat4*      HierarchicalModelSpaceMatrices;
    float      GlobalTimeSec;
    int32_t    AnimStateCount;

    void (*BlendFunc)(animation_player*, void* UserData);
  };

  // Sampling / Blending
  void SampleAtGlobalTime(Anim::animation_player* Player, int AnimationIndex,
                          int OutputBlockIndex, const skeleton_mirror_info* MirrorInfo = NULL);
  void LinearBlend(animation_player*, int AnimA, int AnimB, float t, int ResultInd);
  void AdditiveBlend(animation_player*, int AnimBase, int AnimAdd, float t, int ResultInd);
  void UpdatePlayer(animation_player*, float dt,
                    void  BlendFunction(animation_player*, void* UserData) = NULL,
                    void* UserData                                             = NULL);

  // Animation controller interface
  void AppendAnimation(animation_player*, rid AnimationID);
  void SetAnimation(animation_player*, rid AnimationID, int32_t AnimationIndex);

  void StopAnimation(animation_player*, int AnimationIndex);
  void StartAnimationAtGlobalTime(animation_player*, int AnimationIndex, bool Loop = true,
                                  float LocalStartTime = 0.0f);
  void StartAnimationAtGlobalTime(Anim::animation_player* Player, int AnimationIndex,
                                  bool Loop, float LocalStartTime);

  // Blending and sampling facilities
  void      LerpTransforms(const transform* InA, const transform* InB, int TransformCount, float T,
                           transform* Out);
  void      AddTransforms(const transform* InA, const transform* InB, int TransformCount, float T,
                          transform* Out);
  void      LinearAnimationSample(transform* OutputTransforms, const Anim::animation* Animation,
                                  float Time);
  void      LinearAnimationSample(animation_player*, int AnimAInd, float Time, int ResultIndex);
  void      LinearMirroredAnimationSample(transform* OutputTransforms, mat4* TempMatrices,
                                          const skeleton* Skeleton, const Anim::animation* Animation,
                                          float Time, const Anim::skeleton_mirror_info* MirrorInfo);
  void      LinearMirroredAnimationSample(Anim::animation_player* Player, int AnimIndex,
                                          float Time, int ResultIndex,
                                          const skeleton_mirror_info* MirrorInfo);
  transform LinearAnimationBoneSample(const Anim::animation* Animation, int BoneIndex, float Time);

  // Matrix palette generation
  void ComputeBoneSpacePoses(mat4* BoneSpaceMatrices, const transform* Transforms, int32_t Count);
  void ComputeModelSpacePoses(mat4* ModelSpacePoses, const mat4* BoneSpaceMatrices,
                              const Anim::skeleton* Skeleton);
  void ComputeFinalHierarchicalPoses(mat4* FinalPoseMatrices, const mat4* ModelSpaceMatrices,
                                     const Anim::skeleton* Skeleton);
  void InverseComputeBoneSpacePoses(transform* Transforms, const mat4* BoneSpaceMatrices,
                                    int Count);
  void InverseComputeModelSpacePoses(mat4* BoneSpaceMatrices, const mat4* ModelSpaceMatrices,
                                     const Anim::skeleton* Skeleton);

  // Helper functions
  float GetLocalSampleTime(const Anim::animation* Animation, float SampleTime, float StartTime = 0,
                           bool Loop = false, float PlaybackRate = 1.0f);
  float GetLocalSampleTime(const Anim::animation_player* Player, int AnimationIndex,
                           float GlobalTimeSec);
  float GetLocalSampleTime(const Anim::animation* Animation, const Anim::animation_state* AnimState,
                           float GlobalSampleTime);
  void  GetRootAndInvRootMatrices(mat4* OutRootMatrix, mat4* OutInvRoot, mat4 HipMatrix);
  float GetAnimDuration(const Anim::animation* Animation);
  void  GenerateSkeletonMirroringInfo(Anim::skeleton_mirror_info* OutMirrorInfo,
                                      const Anim::skeleton*       Skeleton);

  void PreviewBlendFunc(animation_player* AnimPlayer, void* UserData);
}
