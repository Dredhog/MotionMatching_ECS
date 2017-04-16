#include "ui.h"
#include "debug_drawing.h"
#include "misc.h"

#if 0
struct ui_id
{
  uintptr_t DataPrt;
};

static const ui_id g_Active;
static const ui_id g_Hot;
#endif

#define _LayoutIntersects(Layout, Input)                                                           \
  (((Layout)->X <= (Input)->NormMouseX) &&                                                         \
   ((Input)->NormMouseX < (Layout)->X + (Layout)->ColumnWidth) &&                                  \
   ((Layout)->Y - (Layout)->RowHeight < (Input)->NormMouseY) &&                                    \
   ((Input)->NormMouseY <= (Layout)->Y))

#define _Intersects(X, Y, Width, Height, Input)                                                    \
  ((X <= (Input)->NormMouseX) && ((Input)->NormMouseX < (X) + (Width)) &&                          \
   ((Y) - (Height) < (Input)->NormMouseY) && ((Input)->NormMouseY <= (Y)))

static const float ANOTATION_WIDTH_PERCENTAGE = 0.4f;

UI::im_layout
UI::NewLayout(float X, float Y, float Width, float RowHeight, float AspectRatio,
              float ScrollbarWidth)
{
  im_layout Result      = {};
  Result.X              = X;
  Result.Y              = Y;
  Result.TopLeft[0]     = X;
  Result.TopLeft[1]     = Y;
  Result.Width          = Width;
  Result.RowHeight      = RowHeight;
  Result.AspectRatio    = AspectRatio;
  Result.ScrollbarWidth = ScrollbarWidth;
  return Result;
}

void
UI::Row(UI::im_layout* Layout, int ColumnCount)
{
  Layout->X = Layout->TopLeft[0];
  Layout->Y -= Layout->RowHeight;

  assert(ColumnCount >= 1);
  Layout->ColumnWidth = Layout->Width / (float)ColumnCount;
}

void
UI::SquareQuad(game_state* GameState, UI::im_layout* Layout, uint32_t TextureID)
{
  float QuadWidth  = Layout->Width;
  float QuadHeight = Layout->Width;// Layout->AspectRatio;

  UI::Row(Layout);
  Layout->X = Layout->TopLeft[0];
  Layout->Y -= QuadHeight;

  DEBUGDrawUnflippedTexturedQuad(GameState, TextureID, { Layout->X, Layout->Y }, QuadWidth,
                                 QuadHeight);
  Layout->ColumnWidth = Layout->Width;
  Layout->Y += Layout->RowHeight;
}

void
UI::Row(game_state* GameState, UI::im_layout* Layout, int ColumnCount, const char* Text)
{
  Layout->X = Layout->TopLeft[0];
  Layout->Y -= Layout->RowHeight;

  Layout->ColumnWidth = ANOTATION_WIDTH_PERCENTAGE * Layout->Width;
  UI::DrawTextBox(GameState, Layout, Text, { 0.4f, 0.6f, 0.4f, 1.0f });
  assert(ANOTATION_WIDTH_PERCENTAGE < 1.0f);
  Layout->X += ANOTATION_WIDTH_PERCENTAGE * Layout->Width;
  assert(ColumnCount >= 1);
  Layout->ColumnWidth = (1.0f - ANOTATION_WIDTH_PERCENTAGE) * Layout->Width / (float)ColumnCount;
}

void
UI::DrawBox(game_state* GameState, float X, float Y, float Width, float Height, vec4 InnerColor,
            vec4 BorderColor)
{
  float ButtonBorder = 0.002f;
  DEBUGDrawTopLeftQuad(GameState, vec3{ X, Y, 0.0f }, Width, Height, BorderColor);
  DEBUGDrawTopLeftQuad(GameState, vec3{ X + ButtonBorder, Y - ButtonBorder, 0.0f },
                       Width - 2 * ButtonBorder, Height - 2 * ButtonBorder, InnerColor);
}

void
UI::DrawBox(game_state* GameState, im_layout* Layout, vec4 InnerColor, vec4 BorderColor)
{
  UI::DrawBox(GameState, Layout->X, Layout->Y, Layout->ColumnWidth, Layout->RowHeight, InnerColor,
              BorderColor);
}

void
UI::DrawTextBox(game_state* GameState, float X, float Y, float Width, float Height,
                const char* Text, vec4 InnerColor, vec4 BorderColor)
{
  float TextPadding = 0.005f;
  UI::DrawBox(GameState, X, Y, Width, Height, InnerColor, BorderColor);
  DEBUGDrawTopLeftTexturedQuad(GameState, GameState->TextTexture,
                               vec3{ X + TextPadding, Y - TextPadding, 0.0f },
                               Width - 2 * TextPadding, Height - 2 * TextPadding);
}

void
UI::DrawTextBox(game_state* GameState, im_layout* Layout, const char* Text, vec4 InnerColor,
                vec4 BorderColor)
{
  UI::DrawTextBox(GameState, Layout->X, Layout->Y, Layout->ColumnWidth, Layout->RowHeight, Text,
                  InnerColor, BorderColor);
}

void
UI::DrawBoolButton(game_state* GameState, float X, float Y, float Width, float Height,
                   const game_input* Input, const char* Text, bool Pushed)
{
  vec4 NormalColor   = { 0.3f, 0.3f, 0.3f, 1.0f };
  vec4 HoveringColor = { 0.35f, 0.35f, 0.35f, 1.0f };
  vec4 PushedColor   = { 0.3f, 0.3f, 0.7f, 1.0f };
  if(Pushed)
  {
    DrawTextBox(GameState, X, Y, Width, Height, Text, PushedColor);
  }
  else if(_Intersects(X, Y, Width, Height, Input))
  {
    DrawTextBox(GameState, X, Y, Width, Height, Text, HoveringColor);
  }
  else
  {
    DrawTextBox(GameState, X, Y, Width, Height, Text, NormalColor);
  }
}

void
UI::DrawBoolButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                   const char* Text, bool Pushed)
{
  DrawBoolButton(GameState, Layout->X, Layout->Y, Layout->ColumnWidth, Layout->RowHeight, Input,
                 Text, Pushed);
}

void
UI::DrawCollapsableButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                          bool IsExpanded, const char* Text, vec4 InnerColor, vec4 BorderColor)
{
  // draw square icon
  float   IconWidthK = (Layout->RowHeight / Layout->AspectRatio) / Layout->ColumnWidth;
  int32_t TextureID  = (IsExpanded) ? GameState->ExpandedTexture : GameState->CollapsedTexture;
  DEBUGDrawTopLeftTexturedQuad(GameState, TextureID, vec3{ Layout->X, Layout->Y, 0.0f },
                               IconWidthK * Layout->ColumnWidth, Layout->RowHeight);

  DrawBoolButton(GameState, IconWidthK * Layout->ColumnWidth + Layout->X, Layout->Y,
                 (1.0f - IconWidthK) * Layout->ColumnWidth, Layout->RowHeight, Input, Text,
                 IsExpanded);
}

void
UI::DrawTextButton(game_state* GameState, float X, float Y, float Width, float Height,
                   const game_input* Input, const char* Text)
{
  vec4 NormalColor   = { 0.4f, 0.4f, 0.4f, 1.0f };
  vec4 HoveringColor = { 0.5f, 0.5f, 0.5f, 1.0f };
  vec4 PushedColor   = { 0.2f, 0.2f, 0.2f, 1.0f };
  if(Input->MouseLeft.EndedDown && _Intersects(X, Y, Width, Height, Input))
  {
    DrawTextBox(GameState, X, Y, Width, Height, Text, PushedColor);
  }
  else if(_Intersects(X, Y, Width, Height, Input))
  {
    DrawTextBox(GameState, X, Y, Width, Height, Text, HoveringColor);
  }
  else
  {
    DrawTextBox(GameState, X, Y, Width, Height, Text, NormalColor);
  }
}

void
UI::DrawTextButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                   const char* Text)
{
  DrawTextButton(GameState, Layout->X, Layout->Y, Layout->ColumnWidth, Layout->RowHeight, Input,
                 Text);
}

bool
UI::PushButton(game_state* GameState, im_layout* Layout, const game_input* Input, const char* Text,
               vec4 InnerColor, vec4 BorderColor)
{
  bool Result = false;
  DrawTextBox(GameState, Layout, Text, InnerColor, BorderColor);
  if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed && _LayoutIntersects(Layout, Input))
  {
    Result = true;
  }
  Layout->X += Layout->ColumnWidth;
  return Result;
}

bool
UI::PushButton(game_state* GameState, im_layout* Layout, const game_input* Input, const char* Text,
               bool TestChanged)
{
  bool Result = false;
  DrawTextButton(GameState, Layout, Input, Text);
  if(Input->MouseLeft.EndedDown && (!TestChanged || (TestChanged && Input->MouseLeft.Changed)) &&
     _LayoutIntersects(Layout, Input))
  {
    Result = true;
  }
  Layout->X += Layout->ColumnWidth;
  return Result;
}

void
UI::BoolButton(game_state* GameState, im_layout* Layout, const game_input* Input, const char* Text,
               bool* Toggle)
{
  if((Input->MouseLeft.EndedDown && Input->MouseLeft.Changed) && _LayoutIntersects(Layout, Input))
  {
    *Toggle = !*Toggle;
  }
  DrawBoolButton(GameState, Layout, Input, Text, *Toggle);
  Layout->X += Layout->ColumnWidth;
}

bool
UI::ExpandableButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                     const char* Text, bool* IsExpanded)
{
  if((Input->MouseLeft.EndedDown && Input->MouseLeft.Changed) && _LayoutIntersects(Layout, Input))
  {
    *IsExpanded = !*IsExpanded;
  }
  DrawCollapsableButton(GameState, Layout, Input, *IsExpanded, Text, { 0.3f, 0.3f, 0.3f, 1 });
  Layout->X += Layout->ColumnWidth;

  return *IsExpanded;
}

int
UI::BeginScrollableList(game_state* GameState, im_layout* Layout, const game_input* Input,
                        int TotalRowCount, int ScrollRowCount, float g_ScrollK)
{
  g_ScrollK      = ClampFloat(0.0f, g_ScrollK, 1.0f);
  ScrollRowCount = ClampMinInt32(0, ScrollRowCount);
  TotalRowCount  = ClampMinInt32(ScrollRowCount, TotalRowCount);

  float ResultStartRow      = g_ScrollK * (float)(TotalRowCount - ScrollRowCount);
  int   ResultStartRowIndex = (int)ResultStartRow;

  Layout->Width -= Layout->ScrollbarWidth;

  float ScrollBoxHeight = (float)ScrollRowCount * Layout->RowHeight;
  float ButtonHeight    = ((float)ScrollRowCount / (float)TotalRowCount) * ScrollBoxHeight;
  float ButtonYOffset   = ScrollBoxHeight * (ResultStartRow / (float)TotalRowCount);

  UI::DrawBox(GameState, Layout->X - Layout->ScrollbarWidth, Layout->Y - Layout->RowHeight,
              Layout->ScrollbarWidth, ScrollBoxHeight, { 0.4f, 0.4f, 0.4f, 1.0f },
              { 0.1f, 0.1f, 0.1f, 1.0f });
  UI::DrawBox(GameState, Layout->X - Layout->ScrollbarWidth,
              (Layout->Y - Layout->RowHeight) - ButtonYOffset, Layout->ScrollbarWidth, ButtonHeight,
              { 0.6f, 0.6f, 0.6f, 1.0f }, { 0.1f, 0.1f, 0.1f, 1.0f });

  return ResultStartRowIndex;
}

void
UI::EndScrollableList(im_layout* Layout)
{
  Layout->Width += Layout->ScrollbarWidth;
}
