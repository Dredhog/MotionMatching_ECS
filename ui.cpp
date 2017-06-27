#include "ui.h"
#include "ui_internal.h"
#include "debug_drawing.h"
#include "misc.h"
#include "text.h"

static bool ButtonBehavior(rect BoundingBox, ui_id ID, bool* OutHeld = NULL, bool* OutHovered = NULL, bool PressOnClick = false, bool PressOnHold = false);
static rect SliderBehavior(ui_id ID, rect BB, float* ScrollNorm, float NormDragSize, bool Vertical = false, bool* OutHeld = NULL, bool* OutHovering = NULL);
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

void
UI::SliderFloat(const char* Label, float* Value, float MinValue, float MaxValue, bool Vertical, vec3 Size, float DragSize)
{
  assert(Label);
  assert(Value);
  assert(MinValue < MaxValue);
  assert(0 < Size.X && 0 < Size.Y);
  assert(Vertical && DragSize < Size.Y || !Vertical && DragSize < Size.X);

  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Label);

  const float ValueRange   = MaxValue - MinValue;
  const float NormDragSize = DragSize / (Vertical ? Size.Y : Size.X);
  float       NormValue    = ((*Value) - MinValue) / ValueRange;

  rect SliderRect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);
  if(!TestIfVisible(SliderRect))
  {
    return;
  }

  bool Held     = false;
  bool Hovered  = false;
  rect DragRect = SliderBehavior(ID, SliderRect, &NormValue, NormDragSize, Vertical, &Held, &Hovered);
  *Value        = NormValue * ValueRange + MinValue;

  DrawBox(SliderRect.MinP, SliderRect.GetSize(), GetUIColor(ScrollbarBox), GetUIColor(ScrollbarBox));
  DrawBox(DragRect.MinP, DragRect.GetSize(), Held ? GetUIColor(ScrollbarBox) : GetUIColor(ScrollbarDrag), GetUIColor(ScrollbarDrag));
}

static void
Scrollbar(gui_window* Window, bool Vertical)
{
  gui_context& g = *GetContext();

  assert(Window);
  assert(0 < Window->ContentsSize.X);
  assert(0 < Window->ContentsSize.Y);
  assert(g.Style.StyleVars[UI::VAR_ScrollbarSize].X <= Window->Size.X);
  assert(g.Style.StyleVars[UI::VAR_ScrollbarSize].X <= Window->Size.Y);

  ui_id ID = Window->GetID(Vertical ? "##VertScrollbar" : "##HirizScrollbar");

  // Determine drag size
  float*      ScrollNorm   = (Vertical) ? &Window->ScrollNorm.Y : &Window->ScrollNorm.X;
  const float NormDragSize = (Vertical) ? MaxFloat((Window->Size.Y < Window->ContentsSize.Y) ? Window->Size.Y / Window->ContentsSize.Y : 1, g.Style.StyleVars[UI::VAR_DragMinSize].X / Window->Size.Y)
                                        : MaxFloat((Window->Size.X < Window->ContentsSize.X) ? Window->Size.X / Window->ContentsSize.X : 1, g.Style.StyleVars[UI::VAR_DragMinSize].X / Window->Size.X);
  rect ScrollRect = (Vertical) ? NewRect(Window->Position + vec3{ Window->Size.X - g.Style.StyleVars[UI::VAR_ScrollbarSize].X, 0 }, Window->Position + Window->Size)
                               : NewRect(Window->Position + vec3{ 0, Window->Size.Y - g.Style.StyleVars[UI::VAR_ScrollbarSize].X }, Window->Position + Window->Size);

  bool Held;
  bool Hovering;
  rect DragRect = SliderBehavior(ID, ScrollRect, ScrollNorm, NormDragSize, Vertical, &Held, &Hovering);

  DrawBox(ScrollRect.MinP, ScrollRect.GetSize(), GetUIColor(ScrollbarBox), GetUIColor(ScrollbarBox));
  DrawBox(DragRect.MinP, DragRect.GetSize(), Held ? GetUIColor(ScrollbarBox) : GetUIColor(ScrollbarDrag), GetUIColor(ScrollbarDrag));

  if(Vertical)
  {
    Window->ScrollRange.Y = (Window->Size.Y < Window->ContentsSize.Y) ? Window->ContentsSize.Y - Window->Size.Y : 0; // Delta in screen space that the window content can scroll
  }
  else
  {
    Window->ScrollRange.X = (Window->Size.X < Window->ContentsSize.X) ? Window->ContentsSize.X - Window->Size.X : 0; // Delta in screen space that the window content can scroll
  }
}

/*
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
  const float SliderRange = Window->Size.Y - DragSize;

  float& ScrollNorm = (Vertical) ? Window->ScrollNorm.Y : Window->ScrollNorm.X;

  rect ScrollRect = NewRect(Window->Position + vec3{ Window->Size.X - g.Style.StyleVars[UI::VAR_ScrollbarSize].X, 0 }, Window->Position + Window->Size);
  rect DragRect =
    NewRect(ScrollRect.MinP.X, ScrollRect.MinP.Y + ScrollNorm * SliderRange, ScrollRect.MinP.X + g.Style.StyleVars[UI::VAR_ScrollbarSize].X, ScrollRect.MinP.Y + ScrollNorm * SliderRange + DragSize);

  // Handle movement
  bool Held = false;
  ButtonBehavior(DragRect, ID, NULL, &Held);
  if(Held)
  {
    // Clamp ScrollNorm between min and max positions
    if(SliderRange > 0)
    {
      ScrollNorm    = ClampFloat(0, ScrollNorm + (float)g.Input->dMouseScreenY / SliderRange, 1);
      rect DragRect = NewRect(ScrollRect.MinP.X, ScrollRect.MinP.Y + ScrollNorm * SliderRange, ScrollRect.MinP.X + g.Style.StyleVars[UI::VAR_ScrollbarSize].X,
                              ScrollRect.MinP.Y + ScrollNorm * SliderRange + DragSize);
    }
  }

  DrawBox(ScrollRect.MinP + vec3{ g.Style.StyleVars[UI::VAR_DragMinSize].X, 0 }, { g.Style.StyleVars[UI::VAR_DragMinSize].X, SliderRange }, { 1, 0, 1, 1 }, GetUIColor(ScrollbarBox));
  DrawBox(ScrollRect.MinP, ScrollRect.MaxP - ScrollRect.MinP, GetUIColor(ScrollbarBox), GetUIColor(ScrollbarBox));
  DrawBox(DragRect.MinP, DragRect.MaxP - DragRect.MinP, Held ? GetUIColor(ScrollbarBox) : GetUIColor(ScrollbarDrag), GetUIColor(ScrollbarDrag));

  Window->ScrollRange.Y = (Window->Size.Y < Window->ContentsSize.Y) ? Window->ContentsSize.Y - Window->Size.Y : 0; // Delta in screen space that the window content can scroll
}
*/

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
    NewWindow.ClipRect = NewRect(Position, Position + Size - vec3{ g.Style.StyleVars[UI::VAR_ScrollbarSize].X, g.Style.StyleVars[UI::VAR_ScrollbarSize].X });
    Window             = g.Windows.Append(NewWindow);
  }
  g.CurrentWindow = Window;

  Window->Position = Window->MaxPos = Window->CurrentPos = Position;
  Window->CurrentPos -= { Window->ScrollNorm.X * Window->ScrollRange.X, Window->ScrollNorm.Y * Window->ScrollRange.Y };

  DrawBox(Window->Position, Window->Size, GetUIColor(WindowBackground), GetUIColor(WindowBorder));
  Debug::UIPushClipQuad(Window->ClipRect.MinP, Window->ClipRect.GetSize());
}

void
UI::EndWindow()
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
  vec3         MinPos = Window.Position - vec3{ Window.ScrollNorm.X * Window.ScrollRange.X, Window.ScrollNorm.Y * Window.ScrollRange.Y };
  Window.ContentsSize = Window.MaxPos - MinPos;
  // DrawBox(MinPos, Window.ContentsSize, { 1, 0, 1, 1 }, { 1, 1, 1, 1 });
  Scrollbar(g.CurrentWindow, true);
  Scrollbar(g.CurrentWindow, false);
  g.CurrentWindow = NULL;
}

bool
UI::ReleaseButton(const char* Text, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Text);

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);

  if(!TestIfVisible(Rect))
  {
    return false;
  }

  bool Result     = ButtonBehavior(Rect, ID);
  vec4 InnerColor = IsActive(ID) ? GetUIColor(ButtonPressed) : (IsHot(ID) ? GetUIColor(ButtonHover) : GetUIColor(ButtonNormal));
  DrawTextBox(Rect.MinP, Size, Text, InnerColor, GetUIColor(Border));

  return Result;
}

bool
UI::CollapsingHeader(const char* Text, bool* IsExpanded, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Text);

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);

  if(!TestIfVisible(Rect))
  {
    return *IsExpanded;
  }

  bool Result = ButtonBehavior(Rect, ID);
  *IsExpanded = (Result) ? !*IsExpanded : *IsExpanded;

  // draw square icon
  int32_t TextureID = (*IsExpanded) ? g.GameState->ExpandedTextureID : g.GameState->CollapsedTextureID;
  Debug::UIPushTexturedQuad(TextureID, Rect.MinP, { Size.Y, Size.Y });

  vec4 InnerColor = IsActive(ID) ? GetUIColor(HeaderPressed) : (IsHot(ID) ? GetUIColor(HeaderHover) : GetUIColor(HeaderNormal));
  DrawTextBox({ Rect.MinP.X + Size.Y, Rect.MinP.Y }, { Size.X - Size.Y, Size.Y }, Text, InnerColor, GetUIColor(Border));

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
  AddSize(Size);

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

  vec4 InnerColor = IsActive(ID) ? GetUIColor(ButtonPressed) : (IsHot(ID) ? GetUIColor(ButtonHover) : GetUIColor(ButtonNormal));
  DrawTextBox(Window.CurrentPos, Size, Text, InnerColor, GetUIColor(Border));

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

static bool
ButtonBehavior(rect BoundingBox, ui_id ID, bool* OutHeld, bool* OutHovered, bool PressOnClick, bool PressOnHold)
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

static rect
SliderBehavior(ui_id ID, rect BB, float* ScrollNorm, float NormDragSize, bool Vertical, bool* OutHeld, bool* OutHovering)
{
  assert(BB.MinP.X < BB.MaxP.X);
  assert(BB.MinP.Y < BB.MaxP.Y);
  *ScrollNorm  = ClampFloat(0, *ScrollNorm, 1);
  NormDragSize = ClampFloat(0, NormDragSize, 1);

  gui_context& g = *GetContext();

  // Determine actual drag size
  const float RegionSize = (Vertical) ? BB.Height() : BB.Width();
  assert(g.Style.StyleVars[UI::VAR_DragMinSize].X <= RegionSize);

  const float DragSize          = (NormDragSize * RegionSize < g.Style.StyleVars[UI::VAR_DragMinSize].X) ? g.Style.StyleVars[UI::VAR_DragMinSize].X : NormDragSize * RegionSize;
  const float MovableRegionSize = RegionSize - DragSize;
  const float DragOffset        = (*ScrollNorm) * MovableRegionSize;

  rect DragRect;
  if(Vertical)
  {
    DragRect = NewRect(BB.MinP.X, BB.MinP.Y + DragOffset, BB.MinP.X + BB.Width(), BB.MinP.Y + DragOffset + DragSize);
  }
  else
  {
    DragRect = NewRect(BB.MinP.X + DragOffset, BB.MinP.Y, BB.MinP.X + DragOffset + DragSize, BB.MinP.Y + BB.Height());
  }

  bool Held     = false;
  bool Hovering = false;
  bool Pressed  = ButtonBehavior(BB, ID, &Held, &Hovering);
  if(Held)
  {
    float ScreenRegionStart = ((Vertical) ? BB.MinP.Y : BB.MinP.X) + DragSize / 2;
    float ScreenRegionEnd   = ScreenRegionStart + MovableRegionSize;

    if(Vertical)
    {
      *ScrollNorm = ClampFloat(ScreenRegionStart, (float)g.Input->MouseScreenY, ScreenRegionEnd);
    }
    else
    {
      *ScrollNorm = ClampFloat(ScreenRegionStart, (float)g.Input->MouseScreenX, ScreenRegionEnd);
    }

    // TODO(Lukas) connect to other floating point precision facilities
    *ScrollNorm = (MovableRegionSize > 0.00001f) ? (((*ScrollNorm) - ScreenRegionStart) / MovableRegionSize) : 0;

    if(Vertical)
    {
      DragRect = NewRect(BB.MinP.X, BB.MinP.Y + DragOffset, BB.MinP.X + BB.Width(), BB.MinP.Y + DragOffset + DragSize);
    }
    else
    {
      DragRect = NewRect(BB.MinP.X + DragOffset, BB.MinP.Y, BB.MinP.X + DragOffset + DragSize, BB.MinP.Y + BB.Height());
    }
  }
  if(OutHeld)
  {
    *OutHeld = Held;
  }
  if(OutHovering)
  {
    *OutHovering = Hovering;
  }

  return DragRect;
}
