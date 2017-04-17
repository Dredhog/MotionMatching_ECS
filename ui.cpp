#include "ui.h"
#include "debug_drawing.h"
#include "misc.h"
#include "text.h"

#define _LayoutIntersects(Layout, Input)                                                           \
  (((Layout)->X <= (Input)->NormMouseX) &&                                                         \
   ((Input)->NormMouseX < (Layout)->X + (Layout)->ColumnWidth) &&                                  \
   ((Layout)->Y - (Layout)->RowHeight < (Input)->NormMouseY) &&                                    \
   ((Input)->NormMouseY <= (Layout)->Y))

#define _Intersects(X, Y, Width, Height, Input)                                                    \
  ((X <= (Input)->NormMouseX) && ((Input)->NormMouseX < (X) + (Width)) &&                          \
   ((Y) - (Height) < (Input)->NormMouseY) && ((Input)->NormMouseY <= (Y)))

static const float ANOTATION_WIDTH_PERCENTAGE = 0.4f;

struct ui_id
{
  uintptr_t DataPtr;
  uintptr_t NamePtr;
  uint32_t  SectionID;
};

#define NOT_ACTIVE                                                                                 \
  ui_id { 1, 1, 1 }

static ui_id g_Active = NOT_ACTIVE;
static ui_id g_Hot    = NOT_ACTIVE;

bool
IsHot(ui_id ID)
{
  if((ID.DataPtr != g_Hot.DataPtr) || (ID.NamePtr != g_Hot.NamePtr) ||
     (ID.SectionID != g_Hot.SectionID))
  {
    return false;
  }
  return true;
}

bool
IsActive(ui_id ID)
{
  if((ID.DataPtr != g_Active.DataPtr) || (ID.NamePtr != g_Active.NamePtr) ||
     (ID.SectionID != g_Active.SectionID))
  {
    return false;
  }
  return true;
}

void
SetActive(ui_id ID)
{
  g_Active = ID;
}

void
SetHot(ui_id ID)
{
  if(IsActive(NOT_ACTIVE))
  {
    g_Hot = ID;
  }
}

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
  Result.ColumnWidth    = Width;
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
UI::DrawSquareQuad(game_state* GameState, UI::im_layout* Layout, uint32_t TextureID)
{
  float QuadWidth  = Layout->Width;
  float QuadHeight = Layout->Width; // Layout->AspectRatio;

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
  uint32_t TextureID =
    Text::GetTextTextureID(&GameState->Font, (int32_t)(10 * ((float)Width / (float)Height)), Text,
                           vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
  DEBUGDrawTopLeftTexturedQuad(GameState, TextureID, vec3{ X + TextPadding, Y - TextPadding, 0.0f },
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
  ui_id ID   = {};
  ID.DataPtr = (uintptr_t)Text;

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
  ui_id ID   = {};
  ID.DataPtr = (uintptr_t)Text;

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
UI::SliderFloat(game_state* GameState, im_layout* Layout, const game_input* Input, char* Text,
                float* Var, float Min, float Max, float ScreenValue, vec4 InnerColor)
{
  ui_id ID   = {};
  ID.DataPtr = (uintptr_t)Var;
  ID.NamePtr = (uintptr_t)Text;

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else if(IsHot(ID))
  {
    SetHot(NOT_ACTIVE);
  }

  if(IsActive(ID))
  {
    *Var += Input->NormdMouseX * ScreenValue;
    if(!Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(NOT_ACTIVE);
    }
  }
  else if(IsHot(ID))
  {
    if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(ID);
    }
  }
  char FloatTextBuffer[20];

  *Var = ClampFloat(Min, *Var, Max);
  sprintf(FloatTextBuffer, "%5.2f", (double)*Var);
  UI::DrawTextBox(GameState, Layout, FloatTextBuffer, InnerColor);
  Layout->X += Layout->ColumnWidth;
}

void
UI::SliderInt(game_state* GameState, im_layout* Layout, const game_input* Input, char* Text,
              int32_t* Var, int32_t Min, int32_t Max, float ScreenValue, vec4 InnerColor)
{
  ui_id ID   = {};
  ID.DataPtr = (uintptr_t)Var;
  ID.NamePtr = (uintptr_t)Text;

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else if(IsHot(ID))
  {
    SetHot(NOT_ACTIVE);
  }

  if(IsActive(ID))
  {
    *Var += (int32_t)(Input->NormdMouseX * ScreenValue);
    if(!Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(NOT_ACTIVE);
    }
  }
  else if(IsHot(ID))
  {
    if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(ID);
    }
  }
  char TextBuffer[20];

  *Var = ClampInt32InIn(Min, *Var, Max);
  sprintf(TextBuffer, "%d", *Var);
  UI::DrawTextBox(GameState, Layout, TextBuffer, InnerColor);
  Layout->X += Layout->ColumnWidth;
}

void
UI::SliderUint(game_state* GameState, im_layout* Layout, const game_input* Input, char* Text,
               uint32_t* Var, uint32_t Min, uint32_t Max, float ScreenValue, vec4 InnerColor)
{
  ui_id ID   = {};
  ID.DataPtr = (uintptr_t)Var;
  ID.NamePtr = (uintptr_t)Text;

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else if(IsHot(ID))
  {
    SetHot(NOT_ACTIVE);
  }

  if(IsActive(ID))
  {
    *Var += (uint32_t)(Input->dMouseX / ScreenValue);

    if(!Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(NOT_ACTIVE);
    }
  }
  else if(IsHot(ID))
  {
    if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(ID);
    }
  }
  char TextBuffer[20];

  *Var = ClampInt32InIn(Min, *Var, Max);
  sprintf(TextBuffer, "%u", *Var);
  UI::DrawTextBox(GameState, Layout, TextBuffer, InnerColor);
  Layout->X += Layout->ColumnWidth;
}

void
UI::BoolButton(game_state* GameState, im_layout* Layout, const game_input* Input, const char* Text,
               bool* Toggle)
{
#if 1

  ui_id ID   = {};
  ID.DataPtr = (uintptr_t)Toggle;

  if(IsActive(ID))
  {
    // Mouse went up
    if(!Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      if(IsHot(ID))
      {
        *Toggle = !*Toggle;
      }
      SetActive(NOT_ACTIVE);
    }
  }
  else if(IsHot(ID))
  {
    // Mouse went down
    if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(ID);
    }
  }

  // Inside
  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else if(IsHot(ID))
  {
    SetHot(NOT_ACTIVE);
  }

  DrawBoolButton(GameState, Layout, Input, Text, *Toggle);
  Layout->X += Layout->ColumnWidth;

#else
  if((Input->MouseLeft.EndedDown && Input->MouseLeft.Changed) && _LayoutIntersects(Layout, Input))
  {
    *Toggle = !*Toggle;
  }
  DrawBoolButton(GameState, Layout, Input, Text, *Toggle);
  Layout->X += Layout->ColumnWidth;
#endif
}

bool
UI::ExpandableButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                     const char* Text, bool* IsExpanded)
{
  ui_id ID   = {};
  ID.NamePtr = (uintptr_t)Text;

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
