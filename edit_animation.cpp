#include "edit_animation.h"
#include "misc.h"

void
EditAnimation::LerpTransforms(Anim::transform* Result, const Anim::transform* A, float t,
                              const Anim::transform* B)
{
  if(!(0.0f <= t && t <= 1))
  {
    printf("t: %f\n", (double)t);
    assert(0 <= t && t <= 1);
  }
  Result->Translation.X = A->Translation.X * (1 - t) + B->Translation.X * t;
  Result->Translation.Y = A->Translation.Y * (1 - t) + B->Translation.Y * t;
  Result->Translation.Z = A->Translation.Z * (1 - t) + B->Translation.Z * t;
  Result->Rotation.X    = A->Rotation.X * (1 - t) + B->Rotation.X * t;
  Result->Rotation.Y    = A->Rotation.Y * (1 - t) + B->Rotation.Y * t;
  Result->Rotation.Z    = A->Rotation.Z * (1 - t) + B->Rotation.Z * t;
  Result->Scale.X       = A->Scale.X * (1 - t) + B->Scale.X * t;
  Result->Scale.Y       = A->Scale.Y * (1 - t) + B->Scale.Y * t;
  Result->Scale.Z       = A->Scale.Z * (1 - t) + B->Scale.Z * t;
}

void
EditAnimation::LerpKeyframes(editor_keyframe* Result, const editor_keyframe* A, float t,
                             const editor_keyframe* B, int ChannelCount)
{
  for(int i = 0; i < ChannelCount; i++)
  {
    LerpTransforms(&Result->Transforms[i], &A->Transforms[i], t, &B->Transforms[i]);
  }
}

void
EditAnimation::ClampedLinearKeyframeSample(animation_editor* Editor, float Time,
                                           editor_keyframe* Result)
{
  assert(Editor->KeyframeCount > 1);

  Time = ClampFloat(Editor->SampleTimes[0], Editor->PlayHeadTime,
                    Editor->SampleTimes[Editor->KeyframeCount - 1]);
  for(int i = 0; i < Editor->KeyframeCount - 1; i++)
  {
    if(Editor->SampleTimes[i] <= Time && Time <= Editor->SampleTimes[i + 1])
    {
      float FrameDelta = (Editor->SampleTimes[i + 1] - Editor->SampleTimes[i]);
      assert(FrameDelta > 0.0f);

      float t = (Time - Editor->SampleTimes[i]) / FrameDelta;
      if(!(0 <= t && t <= 1))
      {
        printf("s1: %f, s2: %f; Time: %f, t: %f\n", (double)Editor->SampleTimes[i],
               (double)Editor->SampleTimes[i + 1], (double)Time, (double)t);
        assert(0 <= t && t <= 1);
      }

      LerpKeyframes(Result, &Editor->Keyframes[i], t, &Editor->Keyframes[i + 1],
                    Editor->Skeleton->BoneCount);
      return;
    }
  }
}

void
EditAnimation::CalculateHierarchicalmatricesAtTime(animation_editor* Editor)
{
  assert(Editor->KeyframeCount > 0);

  editor_keyframe OutputKeyframe = {};
  if(Editor->KeyframeCount == 1)
  {
    OutputKeyframe = Editor->Keyframes[0];
  }
  else
  {
    ClampedLinearKeyframeSample(Editor, Editor->PlayHeadTime, &OutputKeyframe);
  }

  Anim::ComputeBoneSpacePoses(Editor->BoneSpaceMatrices, OutputKeyframe.Transforms,
                              Editor->Skeleton->BoneCount);
  Anim::ComputeModelSpacePoses(Editor->ModelSpaceMatrices, Editor->BoneSpaceMatrices,
                               Editor->Skeleton);
  Anim::ComputeFinalHierarchicalPoses(Editor->HierarchicalModelSpaceMatrices,
                                      Editor->ModelSpaceMatrices, Editor->Skeleton);
}

void
EditAnimation::InsertKeyframeAtTime(animation_editor* Editor, const editor_keyframe* NewKeyframe,
                                    float Time)
{
  assert(0 <= Editor->KeyframeCount && Editor->KeyframeCount <= EDITOR_ANIM_MAX_KEYFRAME_COUNT);

  int InsertIndex  = 0;
  int SizeIncrease = 1;
  if(Editor->KeyframeCount > 0)
  {
    if(FloatGreaterByThreshold(Time, Editor->SampleTimes[Editor->KeyframeCount - 1],
                               KEYFRAME_MIN_TIME_DIFFERENCE_APART))
    {
      SizeIncrease = 1;
      InsertIndex  = Editor->KeyframeCount;
    }
    else
    {
      for(InsertIndex = 0; InsertIndex < Editor->KeyframeCount; InsertIndex++)
      {
        if(FloatsEqualByThreshold(Time, Editor->SampleTimes[InsertIndex],
                                  KEYFRAME_MIN_TIME_DIFFERENCE_APART))
        {
          SizeIncrease = 0;
          break;
        }
        else if(Time < Editor->SampleTimes[InsertIndex])
        {
          assert(Editor->KeyframeCount < EDITOR_ANIM_MAX_KEYFRAME_COUNT);
          for(int i = Editor->KeyframeCount; i > InsertIndex; i--)
          {
            Editor->Keyframes[i]   = Editor->Keyframes[i - 1];
            Editor->SampleTimes[i] = Editor->SampleTimes[i - 1];
          }
          SizeIncrease = 1;
          break;
        }
      }
    }
  }

  Editor->Keyframes[InsertIndex]   = *NewKeyframe;
  Editor->SampleTimes[InsertIndex] = Time;
  Editor->CurrentKeyframe          = InsertIndex;
  Editor->KeyframeCount += SizeIncrease;
}

void
DeleteKeyframeAtIndex(EditAnimation::animation_editor* Editor, int32_t Index)
{
  assert(0 <= Index && Index <= Editor->KeyframeCount);
  assert(0 < Editor->KeyframeCount);
  for(int i = Index; i < Editor->KeyframeCount - 1; i++)
  {
    Editor->Keyframes[i]   = Editor->Keyframes[i + 1];
    Editor->SampleTimes[i] = Editor->SampleTimes[i + 1];
  }
  --Editor->KeyframeCount;
}

static int32_t
FindCurrentKeyframeIndex(const EditAnimation::animation_editor* Editor)
{
  int32_t CurrentKeyframeIndex = 0;
  for(int i = 0; i < Editor->KeyframeCount; i++)
  {
    if(Editor->SampleTimes[i] <= Editor->PlayHeadTime)
    {
      CurrentKeyframeIndex = i;
    }
  }
  return CurrentKeyframeIndex;
}

void
EditAnimation::DeleteCurrentKeyframe(animation_editor* Editor)
{
  if(Editor->KeyframeCount > 0)
  {
    DeleteKeyframeAtIndex(Editor, Editor->CurrentKeyframe);
    Editor->CurrentKeyframe = FindCurrentKeyframeIndex(Editor);
  }
}

void
EditAnimation::InsertIdleKeyframeAtTime(animation_editor* Editor, float Time)
{
  editor_keyframe IdleKeyframe = {};
  for(int i = 0; i < Editor->Skeleton->BoneCount; i++)
  {
    IdleKeyframe.Transforms[i].Scale = { 1, 1, 1 };
  }
  InsertKeyframeAtTime(Editor, &IdleKeyframe, Time);
}

void
EditAnimation::InsertBlendedKeyframeAtTime(animation_editor* Editor, float Time)
{
  assert(Editor->KeyframeCount >= 0);
  if(Editor->KeyframeCount <= 1)
  {
    InsertIdleKeyframeAtTime(Editor, Time);
  }
  else
  {
    editor_keyframe BlendedKeyframe = {};
    ClampedLinearKeyframeSample(Editor, Time, &BlendedKeyframe);
    InsertKeyframeAtTime(Editor, &BlendedKeyframe, Time);
  }
}

void
EditAnimation::EditNextBone(animation_editor* Editor)
{
  if(Editor->Skeleton->BoneCount > 0)
  {
    Editor->CurrentBone = (Editor->CurrentBone + 1) % Editor->Skeleton->BoneCount;
  }
}

void
EditAnimation::EditPreviousBone(animation_editor* Editor)
{
  if(Editor->Skeleton->BoneCount > 0)
  {
    Editor->CurrentBone =
      (Editor->CurrentBone + Editor->Skeleton->BoneCount - 1) % Editor->Skeleton->BoneCount;
  }
}

void
EditAnimation::EditBoneAtIndex(animation_editor* Editor, int BoneIndex)
{
  assert(0 <= BoneIndex && BoneIndex < Editor->Skeleton->BoneCount);
  Editor->CurrentBone = BoneIndex;
}

void
EditAnimation::AdvancePlayHead(animation_editor* Editor, float dt)
{
  Editor->PlayHeadTime += dt;
  Editor->CurrentKeyframe = FindCurrentKeyframeIndex(Editor);
}

void
EditAnimation::JumpToNextKeyframe(animation_editor* Editor)
{
  if(Editor->KeyframeCount > 0)
  {
    Editor->CurrentKeyframe =
      ClampInt32InIn(0, Editor->CurrentKeyframe + 1, Editor->KeyframeCount - 1);
    Editor->PlayHeadTime = Editor->SampleTimes[Editor->CurrentKeyframe];
  }
}

void
EditAnimation::JumpToPreviousKeyframe(animation_editor* Editor)
{
  if(Editor->KeyframeCount > 0)
  {
    Editor->CurrentKeyframe =
      ClampInt32InIn(0, Editor->CurrentKeyframe - 1, Editor->KeyframeCount - 1);
    Editor->PlayHeadTime = Editor->SampleTimes[Editor->CurrentKeyframe];
  }
}

void
EditAnimation::CopyKeyframeToClipboard(animation_editor* Editor, int Index)
{
  assert(0 <= Index && Index <= Editor->KeyframeCount);
  assert(0 < Editor->KeyframeCount);
  Editor->ClipboardKeyframe = Editor->Keyframes[Index];
}

void
EditAnimation::InsertKeyframeFromClipboardAtTime(animation_editor* Editor, float Time)
{
  EditAnimation::InsertKeyframeAtTime(Editor, &Editor->ClipboardKeyframe, Editor->PlayHeadTime);
}

void
EditAnimation::PlayAnimation(animation_editor* Editor, float dt)
{
  if(Editor->KeyframeCount > 1)
  {
    EditAnimation::AdvancePlayHead(Editor, 1 * dt);
    if(Editor->PlayHeadTime < Editor->SampleTimes[0] ||
       Editor->SampleTimes[Editor->KeyframeCount - 1] < Editor->PlayHeadTime)
    {
      Editor->PlayHeadTime = Editor->SampleTimes[0];
      EditAnimation::AdvancePlayHead(Editor, 0);
    }
  }
}

void
EditAnimation::PrintAnimEditorState(const animation_editor* Editor)
{
  printf("ANIMATION EDITOR\n\n");
  printf("Playhead position: %f\n", (double)Editor->PlayHeadTime);
  printf("keyframe: %d out of %d\n", Editor->CurrentKeyframe, Editor->KeyframeCount);
  printf("bone: %s %d\n", Editor->Skeleton->Bones[Editor->CurrentBone].Name, Editor->CurrentBone);
  for(int i = 0; i < Editor->KeyframeCount; i++)
  {
    printf("%d, %fs\n", i, (double)Editor->SampleTimes[i]);
  }
}
