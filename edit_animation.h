#pragma once

#include <stdio.h>

#include "anim.h"
#include "skeleton.h"
#include "entity.h"

static const float KEYFRAME_MIN_TIME_DIFFERENCE_APART = 0.1f;
#define EDITOR_ANIM_MAX_KEYFRAME_COUNT 30

struct editor_keyframe
{
  Anim::transform Transforms[SKELETON_MAX_BONE_COUNT];
};

namespace EditAnimation
{
  struct animation_editor
  {
    editor_keyframe  Keyframes[EDITOR_ANIM_MAX_KEYFRAME_COUNT];
    editor_keyframe  ClipboardKeyframe;
    float            SampleTimes[EDITOR_ANIM_MAX_KEYFRAME_COUNT];
    Anim::skeleton*  Skeleton;
    Anim::transform* Transform;
    int32_t          EntityIndex;

    mat4 BoneSpaceMatrices[SKELETON_MAX_BONE_COUNT];
    mat4 ModelSpaceMatrices[SKELETON_MAX_BONE_COUNT];
    mat4 HierarchicalModelSpaceMatrices[SKELETON_MAX_BONE_COUNT];

    int32_t KeyframeCount;
    float   PlayHeadTime;
    int32_t CurrentKeyframe;
    int32_t CurrentBone;
  };

  void LerpTransforms(Anim::transform* Result, const Anim::transform* A, float t,
                      const Anim::transform* B);
  void LerpKeyframes(editor_keyframe* Result, const editor_keyframe* A, float t,
                     const editor_keyframe* B, int ChannelCount);
  void ClampedLinearKeyframeSample(animation_editor* Editor, float Time, editor_keyframe* Result);
  void CalculateHierarchicalmatricesAtTime(animation_editor* Editor);
  void InsertKeyframeAtTime(animation_editor* Editor, const editor_keyframe* NewKeyframe,
                            float Time);

  void InsertIdleKeyframeAtTime(animation_editor* Editor, float Time);
  void InsertBlendedKeyframeAtTime(animation_editor* Editor, float Time);
  void DeleteCurrentKeyframe(animation_editor* Editor);
  void MoveKeyframeToPlayHead(animation_editor* Editor, int index);
  void CopyKeyframeToClipboard(animation_editor* Editor, int Index);
  void InsertKeyframeFromClipboardAtTime(animation_editor* Editor, float Time);

  void EditNextBone(animation_editor* Editor);
  void EditPreviousBone(animation_editor* Editor);
  void EditBoneAtIndex(animation_editor* Editor, int BoneIndex);
  void JumpToNextKeyframe(animation_editor* Editor);
  void JumpToPreviousKeyframe(animation_editor* Editor);

  void AdvancePlayHead(animation_editor* Editor, float dt);
  void PlayAnimation(animation_editor* Editor, float dt);
  void PrintAnimEditorState(const animation_editor* Editor);
  float GetTimelinePercentage(const animation_editor* Editor, float Time);

}
