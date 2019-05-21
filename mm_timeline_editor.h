#pragma once

#include "motion_matching.h"
#include "entity_animation_control.h"
#include "ui.h"
#include "common.h"

#define ORDER_BY(DO_FUNC)                                                                          \
  DO_FUNC(OrderInSet, Order In Set)                                                                \
  DO_FUNC(Duration, Duration)                                                                      \
  DO_FUNC(Name, Name)                                                                              \
  DO_FUNC(UsageTime, Usage Time(Sec))                                                              \
  DO_FUNC(UsageFrequency, Usage Frequency)                                                         \
  DO_FUNC(LastUsed, Last Used)

#define RANGE_SCALE(DO_FUNC)                                                                       \
  DO_FUNC(Relative, Relative)                                                                      \
  DO_FUNC(Even, Even)

#define EDITOR_MODE(DO_FUNC)                                                                       \
  DO_FUNC(RuntimeAnalysis, Runtime Analysis)                                                       \
  DO_FUNC(RangeDefinition, Range Definition)

#define GENERATE_TIMELINE_EDITOR_MODE_ENUM(Name, String) TIMELINE_EDITOR_MODE_##Name,
enum timeline_editor_mode
{
  EDITOR_MODE(GENERATE_TIMELINE_EDITOR_MODE_ENUM) TIMELINE_EDITOR_MODE_EnumCount
};
#undef GENERATE_TIMELINE_EDITOR_MODE_ENUM

#define GENERATE_ORDER_BY_ENUM(Name, String) ORDER_BY_##Name,
enum order_by_option
{
  ORDER_BY(GENERATE_ORDER_BY_ENUM) ORDER_BY_EnumCount,
};
#undef GENERATE_ORDER_BY_ENUM

#define GENERATE_RANGE_SCALE_ENUM(Name, String) RANGE_SCALE_##Name,
enum anim_scale_option
{
  RANGE_SCALE(GENERATE_RANGE_SCALE_ENUM) RANGE_SCALE_EnumCount,
};
#undef GENERATE_RANGE_SCALE_ENUM

#define GENERATE_STRING(Name, String) #String,
static const char* g_OrderByOptions[ORDER_BY_EnumCount]       = { ORDER_BY(GENERATE_STRING) };
static const char* g_RangeScaleOptions[RANGE_SCALE_EnumCount] = { RANGE_SCALE(GENERATE_STRING) };
static const char* g_TimelineEditorModeOptions[TIMELINE_EDITOR_MODE_EnumCount] = { EDITOR_MODE(
  GENERATE_STRING) };
#undef GENERATE_STRING

#undef RANGE_SCALE
#undef ORDER_BY
#undef EDITOR_MODE

#define COMBO_ARRAY_ARG(Array) Array, ArrayCount(Array)

#define ANIM_DESCRIPTION_LENGTH
#define ANIM_DESCRIPTION_NEXT_RANGE
#define ANIM_DESCRIPTION_

bool
AreBlendStackTopsEqual(const blend_stack& A, const blend_stack& B)
{
  if(A.Count < 1 || B.Count < 1)
  {
    return false;
  }
  blend_in_info InfoA = A.Peek();
  blend_in_info InfoB = B.Peek();
  if(memcmp(&InfoA, &InfoB, sizeof(blend_in_info)) != 0)
  {
    return false;
  }
  return true;
}

void
OverwriteSelectedMMEntity(blend_stack* BlendStacks, float* AnimPlayerTimes,
                          mm_timeline_state* MMTimelineState, entity* Entities,
                          mm_entity_data* MMEntityData, int32_t SelectedEntityIndex)
{
  int MMEntityIndex = GetEntityMMDataIndex(SelectedEntityIndex, MMEntityData);
  if(MMEntityIndex == -1)
  {
    MMTimelineState->Paused    = false;
    MMTimelineState->Scrubbing = false;
  }
  else
  {
    bool HasNewMatchOccured =
      !AreBlendStackTopsEqual(BlendStacks[MMEntityIndex], MMTimelineState->SavedBlendStack);
    if(HasNewMatchOccured && MMTimelineState->BreakOnMatch && !MMTimelineState->Paused &&
       !MMTimelineState->Scrubbing)
    {
      MMTimelineState->SavedBlendStack     = BlendStacks[MMEntityIndex];
      MMTimelineState->SavedAnimPlayerTime = AnimPlayerTimes[MMEntityIndex];
      MMTimelineState->SavedTransform      = Entities[SelectedEntityIndex].Transform;
      MMTimelineState->Paused              = true;
    }

    if(MMTimelineState->Scrubbing || MMTimelineState->Paused)
    {
      BlendStacks[MMEntityIndex]              = MMTimelineState->SavedBlendStack;
      Entities[SelectedEntityIndex].Transform = MMTimelineState->SavedTransform;
      AnimPlayerTimes[MMEntityIndex]          = MMTimelineState->SavedAnimPlayerTime;
    }
  }
}

void
MMTimelineWindow(mm_timeline_state* TimelineState, blend_stack BlendStack,
                 const float AnimPlayerTime, mm_frame_info AnimGoal,
                 mm_controller_data* MMController, transform Transform, const game_input* Input,
                 const Text::font* UIFont)
{
  assert(MMController);
  if(TimelineState->SavedControllerHash != (uintptr_t)MMController)
  {
    TimelineState->SavedControllerHash = (uintptr_t)MMController;
    TimelineState->SavedBlendStack     = (TimelineState->Paused) ? blend_stack{} : BlendStack;
    TimelineState->Scrubbing           = false;
    TimelineState->SavedAnimPlayerTime = AnimPlayerTime;
    TimelineState->SavedTransform      = Transform;
  }

  // Defining what will be used from mm_controller_data
  const array_handle<Anim::animation*>    Animations = MMController->Animations.GetArrayHandle();
  const array_handle<mm_frame_info_range> AnimInfoRanges =
    MMController->AnimFrameInfoRanges.GetArrayHandle();
  const array_handle<path> AnimPaths = MMController->Params.AnimPaths.GetArrayHandle();
  float InfoSamplingFrequency        = MMController->Params.FixedParams.MetadataSamplingFrequency;

  assert(Animations.Count == AnimInfoRanges.Count);
  for(int i = 0; i < Animations.Count; i++)
  {
    assert(Animations[i]);
    assert(AnimInfoRanges[i].StartTimeInAnim >= 0);
    // assert(AnimInfoRanges[i].Start < AnimInfoRanges[i].End);
  }

  static int OrderByOption = 0;
  static int RangeScaleOption;
  // static bool ShowAvoidedRegions  = true;
  static bool ShowAnimationNames  = false;
  static bool ShowUnusableRegions = true;
  static int  TimelineEditorMode  = 0;

  float AnimDurations[MM_ANIM_CAPACITY];
  // TOP LINE VISUALIZATION PARAMETER UI
  {
    UI::PushWidth(200);
    // UI::Combo("Editor Mode", &TimelineEditorMode, COMBO_ARRAY_ARG(g_TimelineEditorModeOptions));
    // TimelineEditorMode = ClampInt32InIn(0, TimelineEditorMode, TIMELINE_EDITOR_MODE_EnumCount -
    // 1);  UI::SameLine();
    // UI::Combo("Order By", &OrderByOption, COMBO_ARRAY_ARG(g_OrderByOptions));
    // UI::SameLine(300);
    RangeScaleOption = ClampInt32InIn(0, RangeScaleOption, RANGE_SCALE_EnumCount - 1);
    UI::Combo("Animation Ranges", &RangeScaleOption, COMBO_ARRAY_ARG(g_RangeScaleOptions));
    UI::PopWidth();
    RangeScaleOption = ClampInt32InIn(0, RangeScaleOption, RANGE_SCALE_EnumCount - 1);
    UI::SameLine();
    // UI::Checkbox("Show avoided regions", &ShowAvoidedRegions);
    // UI::SameLine();
    UI::Checkbox("Show animation names", &ShowAnimationNames);
    UI::SameLine();
    UI::Checkbox("Show unusable regions", &ShowUnusableRegions);
    UI::SameLine();
    UI::Checkbox("Break On Match", &TimelineState->BreakOnMatch);
  }

  bool Scrubbing = false;
  // TIMELINE CHILD WINDOW
  UI::BeginChildWindow("Controller Animation Timeline", { UI::GetAvailableWidth(), 150 });
  {
    const float FullWidth = UI::GetAvailableWidth();
    if(TimelineEditorMode == TIMELINE_EDITOR_MODE_RuntimeAnalysis)
    {
      // Computing lengths and frame info counts and finding max length
      float MaxAnimDuration   = 0;
      int   MaxAnimNameLength = 0;
      for(int a = 0; a < Animations.Count; a++)
      {
        AnimDurations[a] = Anim::GetAnimDuration(Animations[a]);
        MaxAnimDuration  = MaxFloat(MaxAnimDuration, AnimDurations[a]);
        MaxAnimNameLength =
          MaxInt32(MaxAnimNameLength, int32_t(strlen(strrchr(AnimPaths[a].Name, '/') + 1)));
      }
      // NOTE(Lukas) the 2 here is arbitrary
      float AnimNameWidth = UIFont->AverageSymbolWidth * (MaxAnimNameLength + 1);

      // Drawing the ranges
      UI::PushColor(UI::COLOR_SliderDragPressed, { 1, 1, 0, 1 });
      float MinDragSize = 4;
      UI::PushVar(UI::VAR_DragMinSize, MinDragSize);
      for(int a = 0; a < Animations.Count; a++)
      {
        const float TotalWidthAfterText =
          MaxFloat(MinDragSize, ShowAnimationNames ? (FullWidth - AnimNameWidth) : FullWidth);

        // Finding scale factor to transform ranges in seconds to widths in pixels
        const float PixelsPerSecond =
          TotalWidthAfterText /
          ((RangeScaleOption == RANGE_SCALE_Relative) ? MaxAnimDuration : AnimDurations[a]);

        const float AnimStartTime = Animations[a]->SampleTimes[0];
        const float AnimEndTime   = Animations[a]->SampleTimes[Animations[a]->KeyframeCount - 1];
        const float InfoRangeStartTime = AnimInfoRanges[a].StartTimeInAnim;
        const float InfoRangeEndTime =
          InfoRangeStartTime +
          float(AnimInfoRanges[a].End - AnimInfoRanges[a].Start) / InfoSamplingFrequency;

        fixed_stack<UI::colored_range, 3> ColoredRanges = {};

        if(ShowUnusableRegions)
          ColoredRanges.Push({ AnimStartTime, InfoRangeStartTime, 0, { 0.2f, 0.45f, 0.3f, 0.9f } });

        ColoredRanges.Push({ InfoRangeStartTime, InfoRangeEndTime, 1, { 0.5f, 0.5f, 0.5f, 0.9f } });

        if(ShowUnusableRegions)
          ColoredRanges.Push({ InfoRangeEndTime, AnimEndTime, 0, { 0.2f, 0.5f, 0.3f, 0.9f } });

        fixed_stack<float, ANIM_PLAYER_MAX_ANIM_COUNT>   AnimPlayheads      = {};
        fixed_stack<vec4, ANIM_PLAYER_MAX_ANIM_COUNT>    AnimPlayheadColors = {};
        fixed_stack<int32_t, ANIM_PLAYER_MAX_ANIM_COUNT> BlendStackIndices  = {};
        for(int i = BlendStack.Count - 1; i >= 0; i--)
        {
          vec4 PlayheadColor = { 1, 0.5f, 0.3f, 1 };
          if(BlendStack[i].Animation == Animations[a])
          {
            AnimPlayheads.Push(AnimPlayerTime - BlendStack[i].GlobalAnimStartTime);
            BlendStackIndices.Push(i);
            if(i == BlendStack.Count - 1)
            {
              PlayheadColor = { 0, 1, 0, 1 };
            }
            AnimPlayheadColors.Push(PlayheadColor);
          }
        }

        if(AnimPlayheads.Empty())
        {
          AnimPlayheadColors.Push({});
          AnimPlayheads.Push(AnimStartTime);
        }

        const float LeftSliderLimit  = (ShowUnusableRegions) ? AnimStartTime : InfoRangeStartTime;
        const float RightSliderLimit = (ShowUnusableRegions) ? AnimEndTime : InfoRangeEndTime;

        const float AnimSliderWidth = (RangeScaleOption == RANGE_SCALE_Relative)
                                        ? ((RightSliderLimit - LeftSliderLimit) * PixelsPerSecond)
                                        : TotalWidthAfterText;

        if(ShowAnimationNames)
        {
          UI::Text(strrchr(AnimPaths[a].Name, '/') + 1);
          UI::SameLine(AnimNameWidth, 0);
        }

        if(AnimSliderWidth > MinDragSize && LeftSliderLimit < RightSliderLimit)
        {
          UI::PushID(a);
          UI::PushWidth(AnimSliderWidth);
          UI::AnimInfoSlider("Anim", AnimPlayheads.Elements, AnimPlayheadColors.Elements,
                             AnimPlayheads.Count, LeftSliderLimit, RightSliderLimit,
                             ColoredRanges.Elements, ColoredRanges.Count, 1 / InfoSamplingFrequency,
                             InfoRangeStartTime);
          UI::PopWidth();
          UI::PopID();

          if(UI::IsItemActive())
          {
            Scrubbing = true;
            bool MirrorModified =
              !BlendStackIndices.Empty() ? BlendStack[BlendStackIndices[0]].Mirror : false;

            BlendStack.Clear();
            PlayAnimation(&BlendStack, Animations[a], AnimPlayheads[0], AnimPlayerTime, 0,
                          MirrorModified, false);
          }
        }
      }
      UI::PopVar();
      UI::PopColor();
    }
  }
  UI::EndChildWindow();
  if(BlendStack.Count > 1)
  {
    blend_in_info CandidateBlend = BlendStack.Peek();
    blend_in_info CurrentBlend   = BlendStack[BlendStack.Count - 2];
    int32_t       IndexInSet     = -1;
    for(int i = 0; i < Animations.Count; i++)
    {
      if(Animations[i] == CandidateBlend.Animation)
      {
        IndexInSet = i;
        break;
      }
    }
    assert(IndexInSet != -1);
    float CandidateLocalTime     = AnimPlayerTime - CandidateBlend.GlobalAnimStartTime;
    float CandidateAnimStartTime = CandidateBlend.Animation->SampleTimes[0];
    float CandidateAnimEndTime =
      CandidateBlend.Animation->SampleTimes[CandidateBlend.Animation->KeyframeCount - 1];
    assert(CandidateAnimStartTime <= CandidateLocalTime &&
           CandidateLocalTime <= CandidateAnimEndTime);
    int32_t FrameInfoIndex =
      AnimInfoRanges[IndexInSet].Start +
      int32_t((CandidateLocalTime - AnimInfoRanges[IndexInSet].StartTimeInAnim) /
              InfoSamplingFrequency);

    // UI::PushWidth(0.5f*UI::GetAvailableWidth())
    const mm_dynamic_params& Params = MMController->Params.DynamicParams;

    float BonePCost;
    float BoneVCost;
    float TrajPCost;
    float TrajVCost;
    float TrajACost;
    float Cost = ComputeCostComponents(&BonePCost, &BoneVCost, &TrajPCost, &TrajVCost, &TrajACost,
                                       AnimGoal, MMController->FrameInfos[FrameInfoIndex],
                                       Params.BonePCoefficient, Params.BoneVCoefficient,
                                       Params.TrajPCoefficient, Params.TrajVCoefficient,
                                       Params.TrajAngleCoefficient, Params.TrajectoryWeights);
    float FullCostWidth = 0.3f * UI::GetUsableWindowWidth() * Cost;
    UI::Button("Cost", FullCostWidth);

    float PixelsPerCost = FullCostWidth / Cost;

    assert(BonePCost + BoneVCost + TrajPCost + TrajVCost + TrajACost == Cost);
    // Show individual components
    if(FullCostWidth > UI::GetStyle()->Vars[UI::VAR_DragMinSize])
    {
      fixed_stack<UI::colored_range, 5> ColoredRanges = {};

      float CumulativeSum = 0;
      ColoredRanges.Push({ CumulativeSum, CumulativeSum + BonePCost, 0, { 1, 1, 0, 0.9f } });
      CumulativeSum += BonePCost;
      ColoredRanges.Push({ CumulativeSum, CumulativeSum + BoneVCost, 0, { 1, 0, 1, 0.9f } });
      CumulativeSum += BoneVCost;
      ColoredRanges.Push({ CumulativeSum, CumulativeSum + TrajPCost, 0, { 0, 1, 0, 0.9f } });
      CumulativeSum += TrajPCost;
      ColoredRanges.Push({ CumulativeSum, CumulativeSum + TrajVCost, 0, { 0, 1, 1, 0.9f } });
      CumulativeSum += TrajVCost;
      ColoredRanges.Push({ CumulativeSum, CumulativeSum + TrajACost, 0, { 1, 0, 0, 0.9f } });
      UI::PushWidth(FullCostWidth);
      float DummyFloat = 0;
      vec4  DummyColor = {};
      UI::AnimInfoSlider("Cost Components", &DummyFloat, &DummyColor, 1, 0.0f, Cost,
                         ColoredRanges.Elements, ColoredRanges.Count, 1, 0);
      UI::PopWidth();
    }
    // UI::PopWidth();
  }

  TimelineState->SavedTransform = Transform;
  TimelineState->Scrubbing      = Scrubbing;

  if(!TimelineState->Paused || TimelineState->Scrubbing)
  {
    TimelineState->SavedBlendStack     = BlendStack;
    TimelineState->SavedAnimPlayerTime = AnimPlayerTime;
  }

  if(Input->Space.EndedDown && Input->Space.Changed)
  {
    TimelineState->Paused = !TimelineState->Paused;
  }
}
