#pragma once

#include "game.h"
#include "common.h"
#include "linear_math/vector.h"

namespace UI
{
  enum ui_colors
  {
    GUI_COLOR_Border,
    GUI_COLOR_ButtonNormal,
    GUI_COLOR_ButtonHover,
    GUI_COLOR_ButtonPressed,
    GUI_COLOR_CheckboxNormal,
    GUI_COLOR_CheckboxPressed,
    GUI_COLOR_CheckboxHover,
    GUI_COLOR_WindowBackground,
    GUI_COLOR_WindowBorder,
    GUI_COLOR_Text,
    GUI_COLOR_Count
  };

  enum ui_style_vars
  {
    GUI_VAR_ScrollbarSize,
    GUI_VAR_Count
  };

  struct gui_style
  {
    vec4 Colors[GUI_COLOR_Count];
    vec3 StyleVars[GUI_VAR_Count];
  };

  void BeginFrame(game_state* GameState, const game_input* Input);
  void EndFrame();

  void BeginWindow(const char* Name, vec3 StartP, vec3 Size);
  void EndWindow();

  void SameLine();

  bool CollapsableHeader(const char* Text, bool* IsExpanded);
  bool ReleaseButton(const char* Text, vec3 Size);
  bool ClickButton(const char* Text, vec3 Size);
  void Checkbox(const char* Text, bool* Toggle);

  void SliderFloat(char* Text, float* Var, float Min, float Max, float ScreenValue);
  void SliderFloat3(const char* Text, vec3* VecPtr, float Min = -INFINITY, float Max = INFINITY, float ValueScreenDelta = 10.0f);
  void SliderFloat4(const char* Text, vec3* VecPtr, float Min = 0.0f, float Max = 1.0f, float ValueScreenDelta = 3.0f);

  void Combo(int32_t* ActiveIndex, void* ItemList, int32_t ListLength, size_t ElementSize, char* (*ElementToCharPtr)(void*));
}
