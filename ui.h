#pragma once

#include "game.h"
#include "common.h"
#include "linear_math/vector.h"

namespace UI
{
  struct im_layout
  {
    float X;
    float Y;
    float Width;
    float RowHeight;
    float ColumnWidth;
    float TopLeft[2];
    float ScrollbarWidth;
    float ButtonBorder;
    float TextPadding;
    float AspectRatio;
  };

  im_layout NewLayout(float X, float Y, float Width, float RowHeight, float AspectRatio,
                      float ScrollbarWidth = 0.05f);
  void Row(im_layout* Layout, int ColumnCount = 1);
  void Row(game_state* GameState, UI::im_layout* Layout, int ColumnCount, const char* Text);

  void DrawBox(game_state* GameState, float X, float Y, float Width, float Height, vec4 InnerColor,
               vec4 BorderColor);
  void DrawBox(game_state* GameState, im_layout* Layout, vec4 InnerColor, vec4 BorderColor);

  void DrawBox(game_state* GameState, float X, float Y, float Width, float Height, vec4 InnerColor,
               vec4 BorderColor = { 0.1f, 0.1f, 0.1f, 1 });
  void DrawTextBox(game_state* GameState, float X, float Y, float Width, float Height,
                   const char* Text, vec4 InnerColor, vec4 BorderColor = { 0.1f, 0.1f, 0.1f, 1 });
  void DrawTextBox(game_state* GameState, im_layout* Layout, const char* Text, vec4 InnerColor,
                   vec4 BorderColor = { 0.1f, 0.1f, 0.1f, 1 });
  bool ExpandableButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                        const char* Text, bool* IsExpanded);
  void DrawTextButton(game_state* GameState, float X, float Y, float Width, float Height,
                      const game_input* Input, const char* Text);
  void DrawTextButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                      const char* Text);
  void DrawBoolButton(game_state* GameState, float X, float Y, float Width, float Height,
                      const game_input* Input, const char* Text, bool Pushed);
  void DrawBoolButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                      const char* Text, bool Pushed);
  void BoolButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                  const char* Text, bool* Toggle);
  bool PushButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                  const char* Text, vec4 InnerColor, vec4 BorderColor = { 0.1f, 0.1f, 0.1f, 1 });
  bool PushButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                  const char* Text, bool TestChanged = true);
  void DrawCollapsableButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                             bool IsExpanded, const char* Text, vec4 InnerColor,
                             vec4 BorderColor = { 0.1f, 0.1f, 0.1f, 1 });
  int BeginScrollableList(game_state* GameState, im_layout* Layout, const game_input* Input,
                          int TotalRowCount, int ScrollRowCount, float g_ScrollK);
  void EndScrollableList(im_layout* Layout);
}

#define _Row(Layout, Columns, Text) Row(GameState, (Layout), (Columns), (Text));
#define _DrawTextButton(Layout, Text) DrawTextButton(GameState, (Layout), (Text))
#define _PushButton(Layout, Input, Text) PushButton(GameState, (Layout), (Input), (Text))
#define _HoldButton(Layout, Input, Text) PushButton(GameState, (Layout), (Input), (Text), false)
#define _BoolButton(Layout, Input, Text, Bool)                                                     \
  BoolButton(GameState, (Layout), (Input), (Text), (Bool))
#define _ExpandableButton(Layout, Input, Text, IsExpanded)                                         \
  ExpandableButton(GameState, (Layout), (Input), (Text), (IsExpanded))
#define _BeginScrollableList(Layout, Input, TotalRowCount, ScrollRowCount, g_ScrollK)              \
  BeginScrollableList(GameState, (Layout), (Input), (TotalRowCount), (ScrollRowCount), (g_ScrollK))
