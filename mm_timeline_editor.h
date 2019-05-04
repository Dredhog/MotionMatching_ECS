#pragma once

#include "motion_matching.h"
#include "ui.h"

void MotionMatchingTimelineWindow()
{
  UI::BeginWindow("MM Timeline", { 1200, 700 }, { 1500, 500 });

	UI::PushStyleVar(UI::VAR_DefaultItem
  static float PlayHead = 0.0f UI::SliderFloat(&PlayHead, 0, 1, GetAvailableWidth());

  UI::EndWindow();
}
