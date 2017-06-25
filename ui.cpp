#include "ui.h"
#include "ui_internal.h"
#include "debug_drawing.h"
#include "misc.h"
#include "text.h"

void
UI::BeginFrame(game_state* GameState, const game_input* Input)
{
  if(g_Context.InitChecksum != CONTEXT_CHECKSUM)
  {
    Create(&g_Context, GameState);
  }
  g_Context.Input = Input;
}

void
UI::EndFrame()
{
}

void
UI::BeginWindow(const char* Name, vec3 StartP, vec3 Size)
{
  ui_id ID = { IDHash(Name, sizeof(Name), 0) };

  gui_context& g      = *GetContext();
  gui_window*  Window = NULL;

  int WindowCount = g.Windows.GetCount();
  for(int i = 0; i < WindowCount; i++)
  {
    if(g.Windows[i].ID == ID)
    {
      Window = &g.Windows[i];
    }
  }

  if(!Window)
  {
    gui_window NewWindow  = {};
    size_t     NameLength = strlen(Name);
    assert(strlen(Name) < ARRAY_SIZE(NewWindow.Name.Name));
    strcpy(NewWindow.Name.Name, Name);
    NewWindow.ID       = ID;
    NewWindow.Size     = Size;
    NewWindow.ClipRect = NewRect(StartP, StartP + vec3{ Size.X, -Size.Y });
    Window             = g.Windows.Append(NewWindow);
  }
  g.CurrentWindow = Window;

  Window->StartPos = Window->CurrentPos = StartP;
  DrawBox(Window->StartPos, Window->Size, g.Style.Colors[UI::GUI_COLOR_WindowBackground], g.Style.Colors[UI::GUI_COLOR_WindowBorder]);
}

void
UI::EndWindow()
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
  g.CurrentWindow     = NULL;
}

bool
UI::ReleaseButton(const char* Text, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Text);

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  PushSize(Size);
  /*if(PushForClipping(Rect))
  {
    return false;
  }
  */

  if(Rect.Encloses({ (float)g.Input->MouseScreenX, (float)g.Input->MouseScreenY }))
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
    if(!g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
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
    if(g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      SetActive(ID);
    }
  }
  vec4 InnerColor = IsActive(ID) ? g.Style.Colors[UI::GUI_COLOR_ButtonPressed] : (IsHot(ID) ? g.Style.Colors[UI::GUI_COLOR_ButtonHover] : g.Style.Colors[UI::GUI_COLOR_ButtonNormal]);
  DrawTextBox(Rect.MinP, Size, Text, InnerColor, g.Style.Colors[UI::GUI_COLOR_Border]);

  return Result;
}

/*
bool
UI::ClickButton(const char* Text, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = { IDHash(Text, sizeof(Text), 0) };

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + vec3{ Size.X, -Size.Y });
  PushSize(Size);

  if(Rect.Encloses({ g.Input->NormMouseX, g.Input->NormMouseY, 0 }))
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
    if(g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      SetActive(ID);
      Result = true;
    }
  }

  vec4 InnerColor = IsActive(ID) ? g.Style.Colors[UI::GUI_COLOR_ButtonPressed] : (IsHot(ID) ? g.Style.Colors[UI::GUI_COLOR_ButtonHover] : g.Style.Colors[UI::GUI_COLOR_ButtonNormal]);
  DrawTextBox(Window.CurrentPos, Size, Text, InnerColor, g.Style.Colors[UI::GUI_COLOR_Border]);
  PushSize({ 0, -Size.Y });
  if(IsActive(ID))
  {
    SetActive(NOT_ACTIVE);
  }

  return Result;
}

void
UI::SliderFloat(char* Text, float* Var, float Min, float Max, float ScreenValue)
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
  sprintf(FloatTextBuffer, "%5.2f", (double)*Var);

  vec4 InnerColor = IsActive(ID) ? g_PressedColor : (IsHot(ID) ? g_HighlightColor : g_NormalColor);
  UI::DrawTextBox(GameState, Layout, FloatTextBuffer, InnerColor, g_BorderColor);
  Layout->X += Layout->ColumnWidth;
}

void
UI::Checkbox(const char* Text, bool* Toggle)
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

  vec4 InnerColor = ((*Toggle) ? g_BoolPressedColor : (IsHot(ID) ? g_BoolHighlightColor : g_BoolNormalColor));

  UI::DrawTextBox(GameState, Layout, Text, InnerColor, g_BorderColor);
  Layout->X += Layout->ColumnWidth;
}

bool
UI::CollapsableHeader(const char* Text, bool* IsExpanded)
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
  Debug::PushTopLeftTexturedQuad(TextureID, { Layout->X, Layout->Y, 0.0f }, IconWidthK * Layout->ColumnWidth, Layout->RowHeight);

  vec4 InnerColor = ((*IsExpanded) ? g_BoolPressedColor : (IsHot(ID) ? g_BoolHighlightColor : g_BoolNormalColor));
  UI::DrawTextBox(GameState, { IconWidthK * Layout->ColumnWidth + Layout->X, Layout->Y }, (1.0f - IconWidthK) * Layout->ColumnWidth, Layout->RowHeight, Text, InnerColor, g_BorderColor);
  Layout->X += Layout->ColumnWidth;

  Layout->X += Layout->ColumnWidth;
  return *IsExpanded;
}
*/
