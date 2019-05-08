#pragma once

#include "motion_matching.h"
#include "ui.h"

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

#define COMBO_ARRAY_ARG(Array) Array, ARRAY##_COUNT(Array)

#define ANIM_DESCRIPTION_LENGTH
#define ANIM_DESCRIPTION_NEXT_RANGE
#define ANIM_DESCRIPTION_

struct used_range
{
  float StartTime;
  float EndTime;
};

void
MMTimelineWindow(blend_stack& BlendStack, float* AnimPlayerTime, mm_controller_data* MMController, const game_input* Input)
{
  assert(AnimPlayerTime);
  assert(MMController);

	static bool Paused = false;
  static float LastSavedTime;
  if(Input->Space.EndedDown && Input->Space.Changed)
	{
    Paused = !Paused;
    LastSavedTime = *AnimPlayerTime;
  }

	if(Paused)
	{
    *AnimPlayerTime = LastSavedTime;
  }


  // Definin what will be used from mm_controller_data
  const array_handle<Anim::animation*>    Animations = MMController->Animations.GetArrayHandle();
  const array_handle<mm_frame_info_range> AnimInfoRanges =
    MMController->AnimFrameInfoRanges.GetArrayHandle();
  float InfoSamplingFrequency = MMController->Params.FixedParams.MetadataSamplingFrequency;
  // const used_range** AnimUsedRanges;
  // const int32_t*     AnimRangeCounts;

  assert(Animations.Count == AnimInfoRanges.Count);
  for(int i = 0; i < Animations.Count; i++)
  {
    assert(Animations[i]);
    assert(AnimInfoRanges[i].StartTimeInAnim >= 0);
    assert(AnimInfoRanges[i].Start < AnimInfoRanges[i].End);
  }

  static int  OrderByOption = 0;
  static int  RangeScaleOption;
  static bool ShowAvoidedRegions  = true;
  static bool ShowUnusableRegions = true;
  static int  TimelineEditorMode  = 0;

  float   AnimationLengths[MM_ANIM_CAPACITY];
  float   AnimationPlayheads[MM_ANIM_CAPACITY];
  int32_t BlendStackIndices[MM_ANIM_CAPACITY];
  // TOP LINE VISUALIZATION PARAMETER UI
  {
    UI::PushWidth(200);
    UI::Combo("Editor Mode", &TimelineEditorMode, COMBO_ARRAY_ARG(g_TimelineEditorModeOptions));
    TimelineEditorMode = ClampInt32InIn(0, TimelineEditorMode, TIMELINE_EDITOR_MODE_EnumCount - 1);
    UI::SameLine();
    // UI::Combo("Order By", &OrderByOption, COMBO_ARRAY_ARG(g_OrderByOptions));
    // UI::SameLine(300);
    RangeScaleOption = ClampInt32InIn(0, RangeScaleOption, RANGE_SCALE_EnumCount - 1);
    UI::Combo("Animation Ranges", &RangeScaleOption, COMBO_ARRAY_ARG(g_RangeScaleOptions));
    UI::PopWidth();
    RangeScaleOption = ClampInt32InIn(0, RangeScaleOption, RANGE_SCALE_EnumCount - 1);
    UI::SameLine();
    UI::Checkbox("Show avoided regions", &ShowAvoidedRegions);
    UI::SameLine();
    UI::Checkbox("Show unusable regions", &ShowUnusableRegions);
    // RANGE WIDGET TEST
#if 0
    {
      static float LeftRange  = 1;
      static float RightRange = 2;
      UI::SliderRange("Range Widget Test", &LeftRange, &RightRange, 0, 3);
    }
#endif
  }

  // TIMELINE CHILD WINDOW
  UI::BeginChildWindow("Controller Animation Timeline",
                       { UI::GetAvailableWidth() /*- 10 * (1 + sinf(*AnimPlayerTime))*/, 200 });
  {
    const float FullWidth = UI::GetAvailableWidth();
    if(TimelineEditorMode == TIMELINE_EDITOR_MODE_RuntimeAnalysis)
    {
      // Computing lengths and frame info counts and finding max length
      float MaxAnimationLength = 0;
      for(int a = 0; a < Animations.Count; a++)
      {
        AnimationLengths[a]  = Anim::GetAnimDuration(Animations[a]);
        MaxAnimationLength   = MaxFloat(MaxAnimationLength, AnimationLengths[a]);
        BlendStackIndices[a] = -1;
        AnimationPlayheads[a] = 0;
        for(int b = 0; b < BlendStack.Count; b++)
        {
          if(BlendStack[b].Animation == Animations[a])
          {
            AnimationPlayheads[a] = *AnimPlayerTime - BlendStack[b].GlobalAnimStartTime;
            BlendStackIndices[a]  = b;
            break;
          }
        }
      }

      // Drawing the ranges
      UI::PushColor(UI::COLOR_SliderDragPressed, { 1, 1, 0, 1 });
      UI::PushColor(UI::COLOR_SliderDragNormal, { 0, 1, 0, 1 });
      UI::PushVar(UI::VAR_DragMinSize, 4);
      for(int a = 0; a < Animations.Count; a++)
      {
        // Finding scale factor to transform ranges in seconds to widths in pixels
        const float PixelsPerSecond =
          FullWidth /
          ((RangeScaleOption == RANGE_SCALE_Relative) ? MaxAnimationLength : AnimationLengths[a]);

        UI::PushID(a);

        float LeftSliderLimit = Animations[a]->SampleTimes[0];
        float RightSliderLimit = Animations[a]->SampleTimes[Animations[a]->KeyframeCount - 1];
        float AnimSliderWidth  = AnimationLengths[a] * PixelsPerSecond;

        if(ShowUnusableRegions)
        {
          LeftSliderLimit = AnimInfoRanges[a].StartTimeInAnim;
          RightSliderLimit =
            AnimInfoRanges[a].StartTimeInAnim +
            float(AnimInfoRanges[a].End - AnimInfoRanges[a].Start) / InfoSamplingFrequency;
          assert(LeftSliderLimit < RightSliderLimit);
          AnimSliderWidth = (RightSliderLimit - LeftSliderLimit) * PixelsPerSecond;
          UI::Dummy(LeftSliderLimit * PixelsPerSecond, 0);
					UI::SameLine(0,0);
        }

        if(BlendStackIndices[a] == BlendStack.Count - 1)
          UI::PushColor(UI::COLOR_SliderDragNormal, { 1, 0, 1, 1 });

        UI::PushWidth(AnimSliderWidth);
        float NewPlayheadTime = AnimationPlayheads[a];
        UI::SliderFloat("Playhead", &NewPlayheadTime, LeftSliderLimit, RightSliderLimit);
        UI::PopWidth();

        if(BlendStackIndices[a] == BlendStack.Count - 1)
          UI::PopColor();

        if(ShowUnusableRegions)
        {
          float UnusedWidth =
            (Animations[a]->SampleTimes[Animations[a]->KeyframeCount - 1] - RightSliderLimit) *
            PixelsPerSecond;
					UI::SameLine(0, 0);
          UI::Button("U", UnusedWidth);
        }

        UI::PopID();

        bool SliderIsActive = false;
        if(SliderIsActive)
        {
          bool MirrorModified =
            (BlendStackIndices[a] != -1) ? BlendStack[BlendStackIndices[a]].Mirror : false;

          BlendStack.Clear();
          PlayAnimation(&BlendStack, Animations[a], NewPlayheadTime, *AnimPlayerTime, 0,
                        MirrorModified, false);
          for(int i = 0; i < MM_ANIM_CAPACITY; i++)
          {
            AnimationPlayheads[i] = 0;
            BlendStackIndices[i]  = -1;
          }
        }
      }
			UI::PopVar();
      UI::PopColor();
      UI::PopColor();
    }
  }
  UI::EndChildWindow();
}
