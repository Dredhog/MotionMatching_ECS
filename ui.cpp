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

// Internal drawing API
void DrawBox(game_state* GameState, vec3 TopLeft, float Width, float Height, vec4 InnerColor,
             vec4 BorderColor);
void DrawBox(game_state* GameState, UI::im_layout* Layout, vec4 InnerColor, vec4 BorderColor);

static const float ANOTATION_WIDTH_PERCENTAGE = 0.4f;

struct ui_id
{
  uintptr_t DataPtr;
  uintptr_t NamePtr;
  uintptr_t OwnerPtr;
};

vec4 g_BorderColor        = { 0.1f, 0.1f, 0.1f, 1 };
vec4 g_NormalColor        = { 0.4f, 0.4f, 0.4f, 1 };
vec4 g_HighlightColor     = { 0.5f, 0.5f, 0.5f, 1 };
vec4 g_PressedColor       = { 0.3f, 0.3f, 0.3f, 1 };
vec4 g_BoolNormalColor    = { 0.3f, 0.3f, 0.3f, 1 };
vec4 g_BoolPressedColor   = { 0.2f, 0.2f, 0.4f, 1 };
vec4 g_BoolHighlightColor = { 0.3f, 0.3f, 0.5f, 1 };
vec4 g_DescriptionColor   = { 0.3f, 0.35f, 0.4f, 1 };
vec4 g_FontColor          = { 1.0f, 1.0f, 1.0f, 1 };

#define NOT_ACTIVE                                                                                 \
  ui_id { 1, 1, 1 }

static ui_id g_Active = NOT_ACTIVE;
static ui_id g_Hot    = NOT_ACTIVE;

bool
AreUI_IDsEqual(ui_id A, ui_id B)
{
  bool Result =
    ((A.DataPtr == B.DataPtr) && (A.NamePtr == B.NamePtr) && (A.OwnerPtr == B.OwnerPtr));

  return Result;
}

bool
IsHot(ui_id ID)
{
  if(AreUI_IDsEqual(ID, g_Hot))
  {
    return true;
  }
  return false;
}

bool
IsActive(ui_id ID)
{
  return AreUI_IDsEqual(ID, g_Active);
}

void
SetActive(ui_id ID)
{
  g_Active = ID;
}

void
SetHot(ui_id ID)
{
  if(IsActive(NOT_ACTIVE) || IsActive(ID))
  {
    g_Hot = ID;
  }
}

void
UnsetHot(ui_id ID)
{
  if(IsActive(NOT_ACTIVE) || IsActive(ID))
  {
    g_Hot = NOT_ACTIVE;
  }
}

UI::im_layout
UI::NewLayout(vec3 TopLeft, float Width, float RowHeight, float ScrollbarWidth, float AspectRatio)
{
  im_layout Result   = {};
  Result.CurrentP    = TopLeft;
  Result.TopLeft     = TopLeft;
  Result.Width       = Width;
  Result.ColumnWidth = Width;
  Result.RowHeight   = RowHeight;
  Result.AspectRatio = AspectRatio;
  return Result;
}

void
UI::Row(UI::im_layout* Layout, int ColumnCount)
{
  Layout->X = Layout->TopLeft.X;
  Layout->Y -= Layout->RowHeight;

  assert(ColumnCount >= 1);
  Layout->ColumnWidth = Layout->Width / (float)ColumnCount;
}

void
UI::Row(game_state* GameState, UI::im_layout* Layout, int ColumnCount, const char* Text,
        vec4 DescrioptionBGColor)
{
  Layout->X = Layout->TopLeft.X;
  Layout->Y -= Layout->RowHeight;

  Layout->ColumnWidth = ANOTATION_WIDTH_PERCENTAGE * Layout->Width;
  UI::DrawTextBox(GameState, Layout, Text, g_DescriptionColor);
  assert(ANOTATION_WIDTH_PERCENTAGE < 1.0f);
  Layout->X += ANOTATION_WIDTH_PERCENTAGE * Layout->Width;
  assert(ColumnCount >= 1);
  Layout->ColumnWidth = (1.0f - ANOTATION_WIDTH_PERCENTAGE) * Layout->Width / (float)ColumnCount;
}

void
UI::DrawSquareTexture(game_state* GameState, UI::im_layout* Layout, uint32_t TextureID)
{
  float QuadWidth  = Layout->Width;
  float QuadHeight = Layout->Width; // Layout->AspectRatio;

  UI::Row(Layout);
  Layout->X = Layout->TopLeft.X;
  Debug::PushTopLeftTexturedQuad(TextureID, { Layout->X, Layout->Y }, QuadWidth, QuadHeight);
  Layout->Y -= QuadHeight;

  Layout->ColumnWidth = Layout->Width;
  Layout->Y += Layout->RowHeight;
}

void
DrawBox(game_state* GameState, vec3 TopLeft, float Width, float Height, vec4 InnerColor,
        vec4 BorderColor)
{
  float ButtonBorder = 0.002f;
  Debug::PushTopLeftQuad(TopLeft, Width, Height, BorderColor);
  Debug::PushTopLeftQuad(vec3{ TopLeft.X + ButtonBorder, TopLeft.Y - ButtonBorder, TopLeft.Z },
                         Width - 2 * ButtonBorder, Height - 2 * ButtonBorder, InnerColor);
}

void
DrawBox(game_state* GameState, UI::im_layout* Layout, vec4 InnerColor, vec4 BorderColor)
{
  DrawBox(GameState, Layout->CurrentP, Layout->ColumnWidth, Layout->RowHeight, InnerColor,
          BorderColor);
}

void
UI::DrawTextBox(game_state* GameState, vec3 TopLeft, float Width, float Height, const char* Text,
                vec4 InnerColor, vec4 BorderColor)
{
  float TextPadding = 0.005f;
  DrawBox(GameState, TopLeft, Width, Height, InnerColor, BorderColor);
  int32_t  TextureWidth;
  int32_t  TextureHeight;
  uint32_t TextureID =
    Text::GetTextTextureID(&GameState->Font, (int32_t)(10 * ((float)Width / (float)Height)), Text,
                           g_FontColor, &TextureWidth, &TextureHeight);
  float TextAspect = (float)Width / (float)Height;
  Debug::PushTopLeftTexturedQuad(TextureID,
                                 vec3{ TopLeft.X + TextPadding, TopLeft.Y - TextPadding,
                                       TopLeft.Z },
                                 TextAspect * (Height - 2 * TextPadding), Height - 2 * TextPadding);
  // TextAspect * Height, Height);
}

void
UI::DrawTextBox(game_state* GameState, im_layout* Layout, const char* Text, vec4 InnerColor,
                vec4 BorderColor)
{
  UI::DrawTextBox(GameState, Layout->CurrentP, Layout->ColumnWidth, Layout->RowHeight, Text,
                  InnerColor, BorderColor);
}

bool
UI::PushButton(game_state* GameState, im_layout* Layout, const game_input* Input, const char* Text,
               const void* OwnerID)
{
  ui_id ID    = {};
  ID.DataPtr  = (uintptr_t)Text;
  ID.OwnerPtr = (uintptr_t)OwnerID;

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else
  {
    UnsetHot(ID);
  }

  bool Result = false;
  if(IsHot(ID) && IsActive(NOT_ACTIVE))
  {
    if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(ID);
      Result = true;
    }
  }
  vec4 InnerColor = IsActive(ID) ? g_PressedColor : (IsHot(ID) ? g_HighlightColor : g_NormalColor);
  DrawTextBox(GameState, Layout, Text, InnerColor, g_BorderColor);
  Layout->X += Layout->ColumnWidth;
  if(IsActive(ID))
  {
    SetActive(NOT_ACTIVE);
  }

  return Result;
}

bool
UI::ReleaseButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                  const char* Text, const void* OwnerID)
{
  ui_id ID    = {};
  ID.DataPtr  = (uintptr_t)Text;
  ID.OwnerPtr = (uintptr_t)OwnerID;

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else
  {
    UnsetHot(ID);
  }

  bool Result = false;
  if(IsActive(ID))
  {
    if(!Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      if(IsHot(ID))
      {
        Result = true;
      }
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
  vec4 InnerColor = IsActive(ID) ? g_PressedColor : (IsHot(ID) ? g_HighlightColor : g_NormalColor);
  DrawTextBox(GameState, Layout, Text, InnerColor, g_BorderColor);
  Layout->X += Layout->ColumnWidth;

  return Result;
}

void
UI::SliderFloat(game_state* GameState, im_layout* Layout, const game_input* Input, char* Text,
                float* Var, float Min, float Max, float ScreenValue)
{
  ui_id ID   = {};
  ID.DataPtr = (uintptr_t)Var;
  ID.NamePtr = (uintptr_t)Text;

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else
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
  sprintf(FloatTextBuffer, "%5.3f", (double)*Var);

  vec4 InnerColor = IsActive(ID) ? g_PressedColor : (IsHot(ID) ? g_HighlightColor : g_NormalColor);
  UI::DrawTextBox(GameState, Layout, FloatTextBuffer, InnerColor, g_BorderColor);
  Layout->X += Layout->ColumnWidth;
}

void
UI::BoolButton(game_state* GameState, im_layout* Layout, const game_input* Input, const char* Text,
               bool* Toggle)
{
  ui_id ID   = {};
  ID.NamePtr = (uintptr_t)Text;
  ID.DataPtr = (uintptr_t)Toggle;

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else
  {
    UnsetHot(ID);
  }

  if(IsActive(ID))
  {
    SetActive(NOT_ACTIVE);
  }
  else if(IsHot(ID))
  {
    if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(ID);
      *Toggle = !*Toggle;
    }
  }

  vec4 InnerColor =
    ((*Toggle) ? g_BoolPressedColor : (IsHot(ID) ? g_BoolHighlightColor : g_BoolNormalColor));

  UI::DrawTextBox(GameState, Layout, Text, InnerColor, g_BorderColor);
  Layout->X += Layout->ColumnWidth;
}

bool
UI::ExpandableButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                     const char* Text, bool* IsExpanded)
{
  ui_id ID   = {};
  ID.NamePtr = (uintptr_t)Text;

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else
  {
    UnsetHot(ID);
  }

  if(IsActive(ID) && !Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
  {
    SetActive(NOT_ACTIVE);
  }
  else if(IsHot(ID))
  {
    if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(ID);
      *IsExpanded = !*IsExpanded;
    }
  }

  // draw square icon
  float   IconWidthK = (Layout->RowHeight / Layout->AspectRatio) / Layout->ColumnWidth;
  int32_t TextureID  = (*IsExpanded) ? GameState->ExpandedTextureID : GameState->CollapsedTextureID;
  Debug::PushTopLeftTexturedQuad(TextureID, { Layout->X, Layout->Y, 0.0f },
                                 IconWidthK * Layout->ColumnWidth, Layout->RowHeight);

  vec4 InnerColor =
    ((*IsExpanded) ? g_BoolPressedColor : (IsHot(ID) ? g_BoolHighlightColor : g_BoolNormalColor));
  UI::DrawTextBox(GameState, { IconWidthK * Layout->ColumnWidth + Layout->X, Layout->Y },
                  (1.0f - IconWidthK) * Layout->ColumnWidth, Layout->RowHeight, Text, InnerColor,
                  g_BorderColor);
  Layout->X += Layout->ColumnWidth;

  Layout->X += Layout->ColumnWidth;
  return *IsExpanded;
}

float
VerticalScrollbar(game_state* GameState, const game_input* Input, const float X, const float Y,
                  const float SectionHeight, const float ScrollbarWidth,
                  const float ScrollbarHeight, float* ScrollK)
{
  ui_id ID   = {};
  ID.DataPtr = (uintptr_t)ScrollK;

  if(IsActive(ID))
  {
  }
  *ScrollK = ClampFloat(0.0f, *ScrollK, 1.0f);
  return 0;
}

void
UI::ComboBox(int32_t* ActiveIndex, void* ItemList, int32_t ListLength, game_state* GameState,
             im_layout* Layout, const game_input* Input, size_t ElementSize,
             char* (*ElementToCharPtr)(void*))
{
  ui_id ID    = {};
  ID.DataPtr  = (uintptr_t)ItemList;
  ID.NamePtr  = (uintptr_t)(1000.0f * Layout->Y);
  ID.OwnerPtr = (uintptr_t)(1000.0f * Layout->X);

  float IconWidth       = Layout->RowHeight / Layout->AspectRatio;
  float TextRegionWidth = Layout->ColumnWidth - IconWidth;
  assert(TextRegionWidth > 0);

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else
  {
    UnsetHot(ID);
  }

  if(IsActive(ID))
  {
    if(!_Intersects(Layout->X - IconWidth, Layout->Y, Layout->ColumnWidth + 2 * IconWidth,
                    Layout->RowHeight * (ListLength + 2), Input))
    {
      SetActive(NOT_ACTIVE);
    }
    else if(IsHot(ID) && Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(NOT_ACTIVE);
    }
  }
  else if(IsHot(ID) && (Input->MouseLeft.EndedDown && Input->MouseLeft.Changed))
  {
    SetActive(ID);
  }

#define GetStringAtIndex(Index) (ElementToCharPtr((char*)ItemList + ElementSize * (Index)))
  vec4 HeaderColor = IsHot(ID) ? g_HighlightColor : g_NormalColor;
  // DrawCurrentElement
  if(ListLength > 0)
  {
    DrawTextBox(GameState, Layout->CurrentP, TextRegionWidth, Layout->RowHeight,
                GetStringAtIndex(*ActiveIndex), HeaderColor, g_BorderColor);
    Debug::PushTopLeftTexturedQuad(GameState->ExpandedTextureID,
                                   vec3{ Layout->X + TextRegionWidth, Layout->Y, 0.0f }, IconWidth,
                                   Layout->RowHeight);
  }
  else
  {
    DrawTextBox(GameState, Layout->CurrentP, TextRegionWidth, Layout->RowHeight, "Empty List",
                HeaderColor, g_BorderColor);
    Debug::PushTopLeftTexturedQuad(GameState->ExpandedTextureID,
                                   vec3{ Layout->X + TextRegionWidth, Layout->Y, 0.0f }, IconWidth,
                                   Layout->RowHeight);
  }
  im_layout TempLayout = *Layout;
  TempLayout.TopLeft.X = TempLayout.CurrentP.X;
  TempLayout.CurrentP.Y -= Layout->RowHeight;
  TempLayout.Width      = TempLayout.ColumnWidth;
  TempLayout.CurrentP.Z = -0.1f;
  TempLayout.RowHeight  = 0.9f * Layout->RowHeight;

  if(IsActive(ID))
  {
    for(int i = 0; i < ListLength; i++)
    {
      vec4 BoxColor = g_NormalColor;
      if(_LayoutIntersects(&TempLayout, Input))
      {
        BoxColor = g_HighlightColor;

        if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
        {
          *ActiveIndex = i;
        }
      }
      UI::DrawTextBox(GameState, &TempLayout, GetStringAtIndex(i), BoxColor);
      UI::Row(&TempLayout);
    }
  }
  Layout->X += Layout->ColumnWidth;
#undef GetStringAtIndex
}

void
UI::SliderVec3(game_state* GameState, im_layout* Layout, const game_input* Input, const char* Text,
               vec3* VecPtr, float Min, float Max, float ValueScreenDelta)
{
  Row(GameState, Layout, 3, Text);
  SliderFloat(GameState, Layout, Input, "x", &VecPtr->X, Min, Max, ValueScreenDelta);
  SliderFloat(GameState, Layout, Input, "y", &VecPtr->Y, Min, Max, ValueScreenDelta);
  SliderFloat(GameState, Layout, Input, "z", &VecPtr->Z, Min, Max, ValueScreenDelta);
}

void
UI::SliderVec3Color(game_state* GameState, im_layout* Layout, const game_input* Input,
                    const char* Text, vec3* VecPtr, float Min, float Max, float ValueScreenDelta)
{
  Row(GameState, Layout, 3, Text);
  SliderFloat(GameState, Layout, Input, "r", &VecPtr->R, Min, Max, ValueScreenDelta);
  SliderFloat(GameState, Layout, Input, "g", &VecPtr->G, Min, Max, ValueScreenDelta);
  SliderFloat(GameState, Layout, Input, "b", &VecPtr->B, Min, Max, ValueScreenDelta);
}

void
UI::SliderVec4Color(game_state* GameState, im_layout* Layout, const game_input* Input,
                    const char* Text, vec4* VecPtr, float Min, float Max, float ValueScreenDelta)
{
  Row(GameState, Layout, 4, Text);
  SliderFloat(GameState, Layout, Input, "r", &VecPtr->R, Min, Max, ValueScreenDelta);
  SliderFloat(GameState, Layout, Input, "g", &VecPtr->G, Min, Max, ValueScreenDelta);
  SliderFloat(GameState, Layout, Input, "b", &VecPtr->B, Min, Max, ValueScreenDelta);
  SliderFloat(GameState, Layout, Input, "a", &VecPtr->A, Min, Max, ValueScreenDelta);
}
