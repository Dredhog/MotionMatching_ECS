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
    COLOR_ButtonHovered,
    COLOR_ButtonPressed,
    COLOR_HeaderNormal,
    COLOR_HeaderHovered,
    COLOR_HeaderPressed,
    COLOR_CheckboxNormal,
    COLOR_CheckboxPressed,
    COLOR_CheckboxHovered,
    COLOR_WindowBackground,
    COLOR_WindowBorder,
    COLOR_ChildWindowBackground,
    COLOR_ScrollbarBox,
    COLOR_ScrollbarDrag,
    COLOR_Text,
    COLOR_Count
  };

  enum ui_style_vars
  {
    VAR_BorderThickness,
    VAR_FontSize,
    VAR_BoxPaddingX,
    VAR_BoxPaddingY,
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
    WINDOW_IsChildWindow          = 1 << 4,
    WINDOW_IsNotMovable           = 1 << 5,
    WINDOW_IsNotResizable         = 1 << 6,
    WINDOW_Popup                  = 1 << 7,
    WINDOW_Combo                  = 1 << 8,
  };

  enum ui_button_flags
  {
    BUTTON_PressOnRelease = 1 << 0,
    BUTTON_PressOnClick   = 1 << 1,
    BUTTON_PressOnHold    = 1 << 2,
  };

  struct gui_style
  {
    vec4  Colors[COLOR_Count];
    float Vars[VAR_Count];
  };

  typedef uint32_t window_flags_t;
  typedef uint32_t button_flags_t;

  void BeginFrame(game_state* GameState, const game_input* Input);
  void EndFrame();

  void BeginWindow(const char* Name, vec3 InitialPosition, vec3 Size, window_flags_t Flags = 0);
  void EndWindow();
  void BeginChildWindow(const char* Name, vec3 Size, window_flags_t Flags = 0);
  void EndChildWindow();
  void BeginPopupWindow(const char* Name, vec3 Size, window_flags_t Flags = 0);
  void EndPopupWindow();

  void SameLine();

  bool CollapsingHeader(const char* Text, bool* IsExpanded);
  bool ReleaseButton(const char* Text);
  bool ClickButton(const char* Text);
  void Checkbox(const char* Label, bool* Toggle);

  void SliderFloat(const char* Label, float* Value, float MinValue, float MaxValue, bool Vertical = false);
  void SliderFloat3(const char* Text, vec3* VecPtr, float Min = -INFINITY, float Max = INFINITY, float ValueScreenDelta = 10.0f);
  void SliderFloat4(const char* Text, vec3* VecPtr, float Min = 0.0f, float Max = 1.0f, float ValueScreenDelta = 3.0f);

  void Combo(int32_t* ActiveIndex, void* ItemList, int32_t ListLength, size_t ElementSize, char* (*ElementToCharPtr)(void*));
  void Combo(const char* Label, int* CurrentItem, const char** Items, int ItemCount, int HeightInItems);

  void Image(int32_t TextureID, const char* Name, vec3 Size);
  void Text(const char* Text);

  gui_style* GetStyle();
}
