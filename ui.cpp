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

  gui_context& g = g_Context;

  g.HotID         = 0;
  g.HoveredWindow = NULL;
  int WindowCount = g.Windows.GetCount();
  for(int i = WindowCount - 1; 0 <= i; i--)
  {
    if(IsMouseInsideRect(g.Windows[i].Position, g.Windows[i].Position + g.Windows[i].Size))
    {
      g.HoveredWindow = &g.Windows[i];
      break;
    }
  }
  for(int i = 0; i < WindowCount; i++)
  {
    if(g.ActiveID && g.Windows[i].MoveID == g.ActiveID)
    {
      g.Windows[i].Position += { (float)g.Input->dMouseScreenX, (float)g.Input->dMouseScreenY };
      break;
    }
  }
}

void
UI::EndFrame()
{
  gui_context& g = *GetContext();
  g.ClipStack.Clear();
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
  if(Window->ContentsSize.X == 0 || Window->ContentsSize.Y == 0)
  {
    return;
  }
  assert(0 < Window->ContentsSize.X);
  assert(0 < Window->ContentsSize.Y);
  assert(0 < Window->SizeNoScroll.X);
  assert(0 < Window->SizeNoScroll.Y);
  assert(g.Style.StyleVars[UI::VAR_ScrollbarSize].X <= Window->Size.X);
  assert(g.Style.StyleVars[UI::VAR_ScrollbarSize].X <= Window->Size.Y);

  ui_id ID = Window->GetID(Vertical ? "##VertScrollbar" : "##HirizScrollbar");

  // WithoutScrollbars
  const float ScrollbarSize = g.Style.StyleVars[UI::VAR_ScrollbarSize].X;

  // Determine drag size
  float*      ScrollNorm = (Vertical) ? &Window->ScrollNorm.Y : &Window->ScrollNorm.X;
  const float NormDragSize =
    (Vertical) ? MaxFloat((Window->SizeNoScroll.Y < Window->ContentsSize.Y) ? Window->SizeNoScroll.Y / Window->ContentsSize.Y : 1, g.Style.StyleVars[UI::VAR_DragMinSize].X / Window->SizeNoScroll.Y)
               : MaxFloat((Window->SizeNoScroll.X < Window->ContentsSize.X) ? Window->SizeNoScroll.X / Window->ContentsSize.X : 1, g.Style.StyleVars[UI::VAR_DragMinSize].X / Window->SizeNoScroll.X);
  rect ScrollRect = (Vertical) ? NewRect(Window->Position + vec3{ Window->SizeNoScroll.X, 0 }, Window->Position + vec3{ Window->Size.X, Window->SizeNoScroll.Y })
                               : NewRect(Window->Position + vec3{ 0, Window->SizeNoScroll.Y }, Window->Position + vec3{ Window->SizeNoScroll.X, Window->Size.Y });

  bool Held;
  bool Hovering;
  rect DragRect = SliderBehavior(ID, ScrollRect, ScrollNorm, NormDragSize, Vertical, &Held, &Hovering);

  DrawBox(ScrollRect.MinP, ScrollRect.GetSize(), GetUIColor(ScrollbarBox), GetUIColor(ScrollbarBox));
  DrawBox(DragRect.MinP, DragRect.GetSize(), Held ? GetUIColor(ScrollbarBox) : GetUIColor(ScrollbarDrag), GetUIColor(ScrollbarDrag));

  if(Vertical)
  {
    Window->ScrollRange.Y = (Window->SizeNoScroll.Y < Window->ContentsSize.Y) ? Window->ContentsSize.Y - Window->SizeNoScroll.Y : 0; // Delta in screen space that the window content can scroll
  }
  else
  {
    Window->ScrollRange.X = (Window->SizeNoScroll.X < Window->ContentsSize.X) ? Window->ContentsSize.X - Window->SizeNoScroll.X : 0; // Delta in screen space that the window content can scroll
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
UI::BeginWindow(const char* Name, vec3 InitialPosition, vec3 Size, window_flags_t Flags)
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
    NewWindow.MoveID   = NewWindow.GetID("##Move");
    NewWindow.Flags    = Flags;
    NewWindow.Size     = Size;
    NewWindow.Position = InitialPosition;
    Window             = g.Windows.Append(NewWindow);
  }
  g.CurrentWindow = Window;

  Window->MaxPos = Window->CurrentPos = Window->Position;
  Window->ClipRect                    = NewRect(Window->Position, Window->Position + Size);

  PushClipRect(Window->ClipRect);
  DrawBox(Window->Position, Window->Size, GetUIColor(WindowBackground), GetUIColor(WindowBorder));

  // Order matters
  //#1
  Window->SizeNoScroll = Window->Size;
  Window->SizeNoScroll -= vec3{ (Window->Flags & UI::WINDOW_UseVerticalScrollbar) ? g.Style.StyleVars[UI::VAR_ScrollbarSize].X : 0,
                                (Window->Flags & UI::WINDOW_UseHorizontalScrollbar) ? g.Style.StyleVars[UI::VAR_ScrollbarSize].X : 0 };
  if(Window->SizeNoScroll.Y < Window->ContentsSize.Y) // Add vertical Scrollbar
  {
    Window->Flags |= UI::WINDOW_UseVerticalScrollbar;
  }
  if(Window->SizeNoScroll.X < Window->ContentsSize.X) // Add horizontal Scrollbar
  {
    Window->Flags |= UI::WINDOW_UseHorizontalScrollbar;
  }
  if(Window->SizeNoScroll.Y >= Window->ContentsSize.Y) // Remove vertical Scrollbar
  {
    Window->Flags        = (Window->Flags & ~UI::WINDOW_UseVerticalScrollbar);
    Window->ScrollNorm.Y = 0;
  }
  if(Window->SizeNoScroll.X >= Window->ContentsSize.X) // Remove horizontal Scrollbar
  {
    Window->Flags        = (Window->Flags & ~UI::WINDOW_UseHorizontalScrollbar);
    Window->ScrollNorm.X = 0;
  }
  Window->SizeNoScroll = Window->Size;
  Window->SizeNoScroll -= vec3{ (Window->Flags & UI::WINDOW_UseVerticalScrollbar) ? g.Style.StyleVars[UI::VAR_ScrollbarSize].X : 0,
                                (Window->Flags & UI::WINDOW_UseHorizontalScrollbar) ? g.Style.StyleVars[UI::VAR_ScrollbarSize].X : 0 };
  //#2
  if(Window->Flags & UI::WINDOW_UseVerticalScrollbar)
    Scrollbar(g.CurrentWindow, true);
  if(Window->Flags & UI::WINDOW_UseHorizontalScrollbar)
    Scrollbar(g.CurrentWindow, false);

  //#3
  Window->CurrentPos -= { Window->ScrollNorm.X * Window->ScrollRange.X, Window->ScrollNorm.Y * Window->ScrollRange.Y };
  PushClipRect(NewRect(Window->Position, Window->Position + Window->SizeNoScroll));
}

void
UI::EndWindow()
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
  vec3         MinPos = Window.Position - vec3{ Window.ScrollNorm.X * Window.ScrollRange.X, Window.ScrollNorm.Y * Window.ScrollRange.Y };
  Window.ContentsSize = Window.MaxPos - MinPos;

  //----------Window mooving-----------
  // Automatically sets g.ActiveID = Window.MoveID
  ButtonBehavior(NewRect(Window.Position, Window.Position + Window.Size), Window.MoveID);
  //----------------------------------

  // DrawBox(MinPos, Window.ContentsSize, { 1, 0, 1, 1 }, { 1, 1, 1, 1 });
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
  vec4 InnerColor = (ID == g.ActiveID) ? GetUIColor(ButtonPressed) : ((ID == g.HotID) ? GetUIColor(ButtonHover) : GetUIColor(ButtonNormal));
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

  vec4 InnerColor = (ID == g.ActiveID) ? GetUIColor(HeaderPressed) : ((ID == g.HotID) ? GetUIColor(HeaderHover) : GetUIColor(HeaderNormal));
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
  if(IsHot(ID) && (ID == g.ActiveID)
  {
    if(g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      SetActive(ID);
      Result = true;
    }
  }

  vec4 InnerColor = (ID == g.ActiveID) ? GetUIColor(ButtonPressed) : (IsHot(ID) ? GetUIColor(ButtonHover) : GetUIColor(ButtonNormal));
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

  if((ID == g.ActiveID)
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

  vec4 InnerColor = (ID == g.ActiveID) ? g_PressedColor : (IsHot(ID) ? g_HighlightColor : g_NormalColor);
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

  if((ID == g.ActiveID)
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
ButtonBehavior(rect BB, ui_id ID, bool* OutHeld, bool* OutHovered, bool PressOnClick, bool PressOnHold)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  bool PressOnRelease = !(PressOnClick || PressOnHold);

  bool Hovered = IsHovered(BB, ID);
  if(Hovered)
  {
    SetHot(ID);
  }

  bool Result = false;
  if(ID == g.HotID)
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
  if(ID == g.ActiveID)
  {
    if(g.Input->MouseLeft.EndedDown)
    {
      Held = true;
    }
    else if(PressOnRelease && !g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      if(ID == g.HotID)
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
  const float RegionSize = (Vertical) ? BB.GetHeight() : BB.GetWidth();
  assert(g.Style.StyleVars[UI::VAR_DragMinSize].X <= RegionSize);

  const float DragSize          = (NormDragSize * RegionSize < g.Style.StyleVars[UI::VAR_DragMinSize].X) ? g.Style.StyleVars[UI::VAR_DragMinSize].X : NormDragSize * RegionSize;
  const float MovableRegionSize = RegionSize - DragSize;
  const float DragOffset        = (*ScrollNorm) * MovableRegionSize;

  rect DragRect;
  if(Vertical)
  {
    DragRect = NewRect(BB.MinP.X, BB.MinP.Y + DragOffset, BB.MinP.X + BB.GetWidth(), BB.MinP.Y + DragOffset + DragSize);
  }
  else
  {
    DragRect = NewRect(BB.MinP.X + DragOffset, BB.MinP.Y, BB.MinP.X + DragOffset + DragSize, BB.MinP.Y + BB.GetHeight());
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
      DragRect = NewRect(BB.MinP.X, BB.MinP.Y + DragOffset, BB.MinP.X + BB.GetWidth(), BB.MinP.Y + DragOffset + DragSize);
    }
    else
    {
      DragRect = NewRect(BB.MinP.X + DragOffset, BB.MinP.Y, BB.MinP.X + DragOffset + DragSize, BB.MinP.Y + BB.GetHeight());
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
