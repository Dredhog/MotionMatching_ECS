#pragma once

#include "game.h"
#include "common.h"
#include "linear_math/vector.h"

namespace UI
{
  enum ui_colors
  {
    COLOR_Border,
    COLOR_ButtonNormal,
    COLOR_ButtonHover,
    COLOR_ButtonPressed,
    COLOR_HeaderNormal,
    COLOR_HeaderHover,
    COLOR_HeaderPressed,
    COLOR_CheckboxNormal,
    COLOR_CheckboxPressed,
    COLOR_CheckboxHover,
    COLOR_WindowBackground,
    COLOR_WindowBorder,
    COLOR_ScrollbarBox,
    COLOR_ScrollbarDrag,
    COLOR_Text,
    COLOR_Count
  };

  enum ui_style_vars
  {
    VAR_BorderWidth,
    VAR_ScrollbarSize,
    VAR_DragMinSize,
    VAR_Count
  };

  enum ui_window_flags
  {
    WINDOW_UseVerticalScrollbar   = 1 << 0,
    WINDOW_UseHorizontalScrollbar = 1 << 1,
    WINDOW_Usetitlebar            = 1 << 2,
    WINDOW_ISCollapsable          = 1 << 3,
  };

  enum ui_button_flags
  {
    BUTTON_ActivateOnRelease,
    BUTTON_ActivateOnClick,
  };

  struct gui_style
  {
    vec4 Colors[COLOR_Count];
    vec3 StyleVars[VAR_Count];
  };

  typedef uint32_t window_flags_t;
  typedef uint32_t button_flags_t;

  void BeginFrame(game_state* GameState, const game_input* Input);
  void EndFrame();

  void BeginWindow(const char* Name, vec3 Position, vec3 Size, window_flags_t Flags = WINDOW_UseVerticalScrollbar);
  void EndWindow();

  void SameLine();

  bool CollapsingHeader(const char* Text, bool* IsExpanded, vec3 Size);
  bool ReleaseButton(const char* Text, vec3 Size);
  bool ClickButton(const char* Text, vec3 Size);
  void Checkbox(const char* Text, bool* Toggle);

  void SliderFloat(const char* Label, float* Value, float MinValue, float MaxValue, bool Horizontal, vec3 Size, float DragSize = 0);
  void SliderFloat3(const char* Text, vec3* VecPtr, float Min = -INFINITY, float Max = INFINITY, float ValueScreenDelta = 10.0f);
  void SliderFloat4(const char* Text, vec3* VecPtr, float Min = 0.0f, float Max = 1.0f, float ValueScreenDelta = 3.0f);

  void Combo(int32_t* ActiveIndex, void* ItemList, int32_t ListLength, size_t ElementSize, char* (*ElementToCharPtr)(void*));
}
