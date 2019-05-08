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
	EDITOR_MODE(GENERATE_TIMELINE_EDITOR_MODE_ENUM)
	TIMELINE_EDITOR_MODE_EnumCount
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
static const char* g_TimelineEditorModeOptions[TIMELINE_EDITOR_MODE_EnumCount] = { EDITOR_MODE(GENERATE_STRING) };
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
  float EndTime
};

void
MMTimelineWindow(blend_stack* BlendStack, float* PlayerGlobalTime,
                 const used_range** AnimUsedRanges, const int32_t* AnimRangeCounts,
                 const mm_frame_info_range* AnimRanges, int32_t AnimationCount)
{
  const float InfoSamplingFrequency = 60;
  float       AnimationFrameInfoCounts[ARRAY_COUNT(AnimationRanges)];
  float       AnimationLengths[ARRAY_COUNT(AnimationRanges)];

  static float PlayHead      = 0.0f;
  static int   OrderByOption = 0;
  static int   RangeScaleOption;
  static bool  ShowAvoidedRegions  = true;
  static bool  ShowUnusableRegions = true;
  static int   TimelineEditorMode  = 0;

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
	static float LeftRange = 1;
	static float RightRange = 2;
  UI::SliderRange("Range Widget Test", &LeftRange, &RightRange, 0, 3);

  // Timeline child window
  UI::BeginChildWindow("Controller Animation Timeline", { UI::GetAvailableWidth(), 300 });
  {
    const float FullWidth = UI::GetAvailableWidth();

    // Computing lengths and frame info counts and finding max length
    float MaxAnimationLength = 0;
    for(int a = 0; a < ARRAY_COUNT(AnimationRanges); a++)
    {
      AnimationFrameInfoCounts[a] = float(int(AnimationRanges[a][1] * InfoSamplingFrequency));

      int AnimPartCount   = ARRAY_COUNT(AnimationRanges[0]);
      AnimationLengths[a] = 0;
      for(int i = 0; i < AnimPartCount; i++)
      {
        AnimationLengths[a] +=
          (!ShowUnusableRegions && (i % 2 == 0)) ? 0.0f : AnimationRanges[a][i];
      }
      MaxAnimationLength = MaxFloat(MaxAnimationLength, AnimationLengths[a]);
    }

    // Drawing the ranges
    UI::PushColor(UI::COLOR_ScrollbarDrag, { 0, 1, 0, 1 });
    for(int a = 0; a < ARRAY_COUNT(AnimationRanges); a++)
    {
      // Finding scale factor to transform ranges in seconds to widths in pixels
      const float PixelsPerSecond =
        FullWidth /
        ((RangeScaleOption == RANGE_SCALE_Relative) ? MaxAnimationLength : AnimationLengths[a]);

      UI::PushID(a);

      if(ShowUnusableRegions)
      {
        UI::Button("L", AnimationRanges[a][0] * PixelsPerSecond);
        UI::SameLine(0, 0);
      }

      UI::PushWidth(AnimationRanges[a][1] * PixelsPerSecond);
      UI::SliderFloat("Playhead", &PlayHead, 0, 1);
      UI::PopWidth();

      if(ShowUnusableRegions)
      {
        UI::SameLine(0, 0);
        UI::Button("R", AnimationRanges[a][2] * PixelsPerSecond);
      }

      UI::PopID();
    }
    UI::PopColor();
  }
  UI::EndChildWindow();
}
