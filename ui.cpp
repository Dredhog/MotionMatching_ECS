#include "ui.h"
#include "ui_internal.h"
#include "debug_drawing.h"
#include "misc.h"
#include "text.h"

static bool ButtonBehavior(rect BoundingBox, ui_id ID, bool* OutHovered = NULL, bool* OutHeld = NULL, bool PressOnClick = false, bool PressOnHold = false);
static void Scrollbar(gui_window* Window, bool Vertical);

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

// currently only vertical supported
static void
Scrollbar(gui_window* Window, bool Vertical)
{
  gui_context& g = *GetContext();

  assert(Window);
  assert(Window->ContentsSize.X > 0 && Window->ContentsSize.Y > 0);
  assert(Window->Size.X >= g.Style.StyleVars[UI::VAR_ScrollbarSize].X);
  assert(Window->Size.Y >= g.Style.StyleVars[UI::VAR_ScrollbarSize].X);

  ui_id ID = Window->GetID(Vertical ? "##VertScrollbar" : "##HirizScrollbar");

  // Determine drag size
  const float DragOverWindowSize = MaxFloat((Window->Size.Y < Window->ContentsSize.Y) ? Window->Size.Y / Window->ContentsSize.Y : 1, g.Style.StyleVars[UI::VAR_DragMinSize].X / Window->Size.Y);
  const float DragSize           = DragOverWindowSize * Window->Size.Y;

  // Determine min and max positions is screen space
  assert(Window->Size.Y - DragSize >= 0);
  const float ScrollRange = Window->Size.Y - DragSize;

  float& ScrollNorm = (Vertical) ? Window->ScrollNorm.Y : Window->ScrollNorm.X;

  rect ScrollRect = NewRect(Window->Position + vec3{ Window->Size.X - g.Style.StyleVars[UI::VAR_ScrollbarSize].X, 0 }, Window->Position + Window->Size);
  rect DragRect =
    NewRect(ScrollRect.MinP.X, ScrollRect.MinP.Y + ScrollNorm * ScrollRange, ScrollRect.MinP.X + g.Style.StyleVars[UI::VAR_ScrollbarSize].X, ScrollRect.MinP.Y + ScrollNorm * ScrollRange + DragSize);

  // Handle movement
  bool Held = false;
  ButtonBehavior(DragRect, ID, NULL, &Held);
  if(Held)
  {
    // Clamp ScrollNorm between min and max positions
    if(ScrollRange > 0)
    {
      ScrollNorm    = ClampFloat(0, ScrollNorm + (float)g.Input->dMouseScreenY / ScrollRange, 1);
      rect DragRect = NewRect(ScrollRect.MinP.X, ScrollRect.MinP.Y + ScrollNorm * ScrollRange, ScrollRect.MinP.X + g.Style.StyleVars[UI::VAR_ScrollbarSize].X,
                              ScrollRect.MinP.Y + ScrollNorm * ScrollRange + DragSize);
    }
  }

  DrawBox(ScrollRect.MinP + vec3{ g.Style.StyleVars[UI::VAR_DragMinSize].X, 0 }, { g.Style.StyleVars[UI::VAR_DragMinSize].X, ScrollRange }, { 1, 0, 1, 1 }, g.Style.Colors[UI::COLOR_ScrollbarBox]);
  DrawBox(ScrollRect.MinP, ScrollRect.MaxP - ScrollRect.MinP, g.Style.Colors[UI::COLOR_ScrollbarBox], g.Style.Colors[UI::COLOR_ScrollbarBox]);
  DrawBox(DragRect.MinP, DragRect.MaxP - DragRect.MinP, Held ? g.Style.Colors[UI::COLOR_ScrollbarBox] : g.Style.Colors[UI::COLOR_ScrollbarDrag], g.Style.Colors[UI::COLOR_ScrollbarDrag]);

  Window->ScrollRange.Y = ScrollRange;
}

void
UI::BeginWindow(const char* Name, vec3 Position, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window*  Window = NULL;

  ui_id ID = { IDHash(Name, sizeof(Name), 0) };

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
    NewWindow.ClipRect = NewRect(Position, Position + Size);
    Window             = g.Windows.Append(NewWindow);
  }
  g.CurrentWindow = Window;

  Window->Position = Window->MaxPos = Window->CurrentPos = Position;
  Window->CurrentPos.Y -= Window->ScrollNorm.Y * Window->ScrollRange.Y;

  DrawBox(Window->Position, Window->Size, g.Style.Colors[UI::COLOR_WindowBackground], g.Style.Colors[UI::COLOR_WindowBorder]);
}

void
UI::EndWindow()
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
  vec3         MinPos = Window.Position - vec3{ Window.ScrollNorm.X * Window.ScrollRange.X, Window.ScrollNorm.Y * Window.ScrollRange.Y };
  Window.ContentsSize = Window.MaxPos - MinPos;
  // DrawBox(Window.Position, Window.ContentsSize, { 1, 0, 1, 1 }, { 1, 1, 1, 1 });
  Scrollbar(g.CurrentWindow, true);
  g.CurrentWindow = NULL;
}

bool
ButtonBehavior(rect BoundingBox, ui_id ID, bool* OutHovered, bool* OutHeld, bool PressOnClick, bool PressOnHold)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  bool PressOnRelease = !(PressOnClick || PressOnHold);

  // TODO(Lukas) look into mouse precision
  bool Hovered = BoundingBox.Encloses({ (float)g.Input->MouseScreenX, (float)g.Input->MouseScreenY });
  Hovered ? SetHot(ID) : UnsetHot(ID);

  bool Result = false;
  if(IsHot(ID))
  {
    if(PressOnRelease && g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      SetActive(ID);
    }
    else if(PressOnClick && g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      SetActive(NOT_ACTIVE);
      Result = true;
    }
  }

  bool Held = false;
  if(IsActive(ID))
  {
    if(g.Input->MouseLeft.EndedDown)
    {
      Held = true;
    }
    else if(PressOnRelease && !g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      if(IsHot(ID))
      {
        Result = true;
      }
      SetActive(NOT_ACTIVE);
    }
  }

  if(OutHovered)
  {
    *OutHovered = Hovered;
  }
  if(OutHeld)
  {
    *OutHeld = Held;
  }

  return Result;
}

bool
UI::ReleaseButton(const char* Text, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Text);

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  PushSize(Size);

  if(!TestIfVisible(Rect))
  {
    return false;
  }

  bool Result     = ButtonBehavior(Rect, ID);
  vec4 InnerColor = IsActive(ID) ? g.Style.Colors[UI::COLOR_ButtonPressed] : (IsHot(ID) ? g.Style.Colors[UI::COLOR_ButtonHover] : g.Style.Colors[UI::COLOR_ButtonNormal]);
  DrawTextBox(Rect.MinP, Size, Text, InnerColor, g.Style.Colors[UI::COLOR_Border]);

  return Result;
}

bool
UI::CollapsingHeader(const char* Text, bool* IsExpanded, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Text);

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  PushSize(Size);

  if(!TestIfVisible(Rect))
  {
    return *IsExpanded;
  }

  bool Result = ButtonBehavior(Rect, ID);
  *IsExpanded = (Result) ? !*IsExpanded : *IsExpanded;

  // draw square icon
  int32_t TextureID = (*IsExpanded) ? g.GameState->ExpandedTextureID : g.GameState->CollapsedTextureID;
  Debug::UIPushTexturedQuad(TextureID, Rect.MinP, { Size.Y, Size.Y });

  vec4 InnerColor = IsActive(ID) ? g.Style.Colors[UI::COLOR_HeaderPressed] : (IsHot(ID) ? g.Style.Colors[UI::COLOR_HeaderHover] : g.Style.Colors[UI::COLOR_HeaderNormal]);
  DrawTextBox({ Rect.MinP.X + Size.Y, Rect.MinP.Y }, { Size.X - Size.Y, Size.Y }, Text, InnerColor, g.Style.Colors[UI::COLOR_Border]);

  return *IsExpanded;
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

  vec4 InnerColor = IsActive(ID) ? g.Style.Colors[UI::COLOR_ButtonPressed] : (IsHot(ID) ? g.Style.Colors[UI::COLOR_ButtonHover] : g.Style.Colors[UI::COLOR_ButtonNormal]);
  DrawTextBox(Window.CurrentPos, Size, Text, InnerColor, g.Style.Colors[UI::COLOR_Border]);

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
*/
