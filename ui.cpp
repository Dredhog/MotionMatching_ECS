#include "ui.h"
#include "ui_internal.h"
#include "debug_drawing.h"
#include "misc.h"
#include "text.h"

static bool ButtonBehavior(rect BoundingBox, ui_id ID, bool* OutHeld = NULL,
                           bool* OutHovered = NULL, UI::button_flags_t Flags = 0);
static void DragBehavior(ui_id ID, rect BB, float* Value, float MinValue, float MaxValue,
                         float DeltaPerScreen, bool* OutHeld = NULL, bool* OutHovering = NULL);
static rect SliderBehavior(ui_id ID, rect BB, float* ScrollNorm, float NormDragSize,
                           bool Vertical = false, bool* OutHeld = NULL, bool* OutHovering = NULL);
static void Scrollbar(gui_window* Window, bool Vertical);

UI::gui_style*
UI::GetStyle()
{
  gui_context& g = *GetContext();
  return &g.Style;
}

void
UI::BeginFrame(game_state* GameState, const game_input* Input)
{
  if(g_Context.InitChecksum != CONTEXT_CHECKSUM)
  {
    Init(&g_Context, GameState);
  }
  g_Context.Input = Input;

  gui_context& g = g_Context;

  g.HotID         = 0;
  g.HoveredWindow = NULL;
  if(g.Input->ToggledEditorMode)
  {
    g.ActiveID = 0;
  }

  for(int i = g.OrderedWindows.Count - 1; 0 <= i; i--)
  {
    if(IsMouseInsideRect(g.OrderedWindows[i]->ClippedSizeRect))
    {
      // Set hovered window
      g.HoveredWindow = g.OrderedWindows[i];

      // Perform mouse scrolling
      if(g.Input->dMouseWheelScreen)
      {
        const float MagicScrollPixelAmount = 50;
        if(g.Input->LeftShift.EndedDown) // Horizontal scrolling
        {
          const float NormScrollDistance =
            (g.HoveredWindow->SizeNoScroll.X < g.HoveredWindow->ContentsSize.X)
              ? (g.Input->dMouseWheelScreen * MagicScrollPixelAmount) /
                  (g.HoveredWindow->ContentsSize.X - g.HoveredWindow->SizeNoScroll.X)
              : 0;
          g.HoveredWindow->ScrollNorm.X =
            ClampFloat(0, g.HoveredWindow->ScrollNorm.X + NormScrollDistance, 1);
        }
        else // Vertical scrolling
        {
          const float NormScrollDistance =
            (g.HoveredWindow->SizeNoScroll.Y < g.HoveredWindow->ContentsSize.Y)
              ? (g.Input->dMouseWheelScreen * MagicScrollPixelAmount) /
                  (g.HoveredWindow->ContentsSize.Y - g.HoveredWindow->SizeNoScroll.Y)
              : 0;
          g.HoveredWindow->ScrollNorm.Y =
            ClampFloat(0, g.HoveredWindow->ScrollNorm.Y + NormScrollDistance, 1);
        }
      }

      break;
    }
  }

  for(int i = 0; i < g.Windows.Count; i++)
  {
    if(g.ActiveID && g.Windows[i].MoveID == g.ActiveID)
    {
      FocusWindow(&g.Windows[i]);
      if(!(g.Windows[i].Flags & WINDOW_IsNotMovable))
      {
        gui_window* RootMoveWindow = g.Windows[i].RootWindow;
        assert(RootMoveWindow);
        RootMoveWindow->Position +=
          { (float)g.Input->dMouseScreenX, (float)g.Input->dMouseScreenY };
      }
      break;
    }
  }
}

void
r_AddWindowToSortedBuffer(fixed_stack<gui_window*, 10>* SortedWindows, gui_window* Window)
{
  SortedWindows->Push(Window);
  for(int i = 0; i < Window->ChildWindows.Count; i++)
  {
    gui_window* ChildWindow = Window->ChildWindows[i];
    r_AddWindowToSortedBuffer(SortedWindows, ChildWindow);
  }
}

void
UI::EndFrame()
{
  gui_context&                 g = *GetContext();
  fixed_stack<gui_window*, 10> SortBuffer;
  SortBuffer.Clear();
  // Order windows back to front
  for(int i = 0; i < g.OrderedWindows.Count; i++)
  {
    gui_window* Window = g.OrderedWindows[i];
    if(!(Window->Flags & WINDOW_IsChildWindow) && Window->UsedThisFrame)
    {
      r_AddWindowToSortedBuffer(&SortBuffer, Window);
    }
  }
  g.OrderedWindows = SortBuffer;

  // Submit for drawing
  for(int i = 0; i < g.OrderedWindows.Count; i++)
  {
    gui_window* Window = g.OrderedWindows[i];

    for(int j = 0; j < Window->DrawArray.Count; j++)
    {
      const quad_instance& Quad = Window->DrawArray[j];
      switch(Quad.Type)
      {
        case QuadType_Colored:
        {
          Debug::UIPushQuad(Quad.LowerLeft, Quad.Dimensions, Quad.Color);
          break;
        }
        case QuadType_Textured:
        {
          Debug::UIPushTexturedQuad(Quad.TextureID, Quad.LowerLeft, Quad.Dimensions);
          break;
        }
        case QuadType_Clip:
        {
          Debug::UIPushClipQuad(Quad.LowerLeft, Quad.Dimensions, (int32_t)Quad.Color.A);
          break;
        }
      }
    }
    g.OrderedWindows[i]->DrawArray.Clear();
    g.OrderedWindows[i]->ChildWindows.Clear();
  }

  if(g.ActiveID == 0 && g.HotID == 0 && g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
  {
    if(g.FocusedWindow != NULL)
    {
      FocusWindow(NULL);
    }
  }

  for(int i = 0; i < g.Windows.Count; i++)
  {
    g.Windows[i].UsedLastFrame = g.Windows[i].UsedThisFrame;
    g.Windows[i].UsedThisFrame = false;
  }

  g.ClipRectStack.Clear();
  g.CurrentPopupStack.Clear();
  CloseInactivePopups();
  g.LatestClipRectIndex = 0;
}

// TODO(Lukas) For debuggig use only (REMOVE)
uint32_t
UI::GetActiveID()
{
  gui_context& g = *GetContext();
  return g.ActiveID;
}

uint32_t
UI::GetHotID()
{
  gui_context& g = *GetContext();
  return g.HotID;
}

void
UI::SliderFloat(const char* Label, float* Value, float MinValue, float MaxValue, bool Vertical)
{
  assert(Label);
  assert(Value);
  assert(MinValue < MaxValue);
  // assert(0 < Size.X && 0 < Size.Y);
  // assert(Vertical && DragSize < Size.Y || !Vertical && DragSize < Size.X);

  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  float DragSize = g.Style.Vars[UI::VAR_DragMinSize];
  vec3  Size     = Window.GetItemSize();

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

  bool Held    = false;
  bool Hovered = false;
  rect DragRect =
    SliderBehavior(ID, SliderRect, &NormValue, NormDragSize, Vertical, &Held, &Hovered);
  *Value = NormValue * ValueRange + MinValue;
  if(Held && g.Input->LeftCtrl.EndedDown)
  {
    *Value = floorf(*Value + 0.5f);
  }

  DrawBox(SliderRect.MinP, SliderRect.GetSize(), _GetGUIColor(ScrollbarBox),
          _GetGUIColor(ScrollbarBox));
  DrawBox(DragRect.MinP, DragRect.GetSize(),
          Held ? _GetGUIColor(ScrollbarBox) : _GetGUIColor(ScrollbarDrag),
          _GetGUIColor(ScrollbarDrag));
  DrawText(SliderRect.MaxP +
             vec3{ g.Style.Vars[UI::VAR_InternalSpacing], -g.Style.Vars[UI::VAR_BoxPaddingY] },
           Label);

  char TempBuffer[20];
  snprintf(TempBuffer, sizeof(TempBuffer), "%.2f", *Value);
  DrawText({ SliderRect.MinP.X + g.Style.Vars[UI::VAR_BoxPaddingX],
             SliderRect.MinP.Y + SliderRect.GetHeight() - g.Style.Vars[UI::VAR_BoxPaddingY] },
           TempBuffer);
}

void
UI::SliderRange(const void* IDPtr, float* LeftRange, float* RightRange, float MinValue, float MaxValue)
{
  assert(IDPtr);
  assert(MinValue < MaxValue);
  assert(*LeftRange <= *RightRange);
  assert(MinValue <= *LeftRange);
  assert(*RightRange <= MaxValue);

  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  float DragSize = g.Style.Vars[UI::VAR_DragMinSize];
  vec3  Size     = Window.GetItemSize();

  ui_id ID = Window.GetID(IDPtr);

  rect BoundingRect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  // DrawBox(BoundingRect.MinP, BoundingRect.GetSize(), _GetGUIColor(ScrollbarBox),
  //_GetGUIColor(ScrollbarBox));
  AddSize(Size);
  if(!TestIfVisible(BoundingRect))
  {
    return;
  }

  UI::PushID("Range");
  UI::PushWidth(Size.X * ((*RightRange - MinValue) / (MaxValue - MinValue)));
  UI::SliderFloat("L", LeftRange, MinValue, *RightRange);
  UI::PopWidth();

  UI::Dummy();
  UI::SameLine(Size.X * ((*LeftRange - MinValue)/ (MaxValue-MinValue)), 0);
  UI::PushWidth(Size.X * ((MaxValue - *LeftRange) / (MaxValue - MinValue)));
  UI::SliderFloat("R", RightRange, *LeftRange, MaxValue);
  UI::PopWidth();
  UI::PopID();
}

void
UI::SliderInt(const char* Label, int32_t* Value, int32_t MinValue, int32_t MaxValue, bool Vertical)
{
  assert(Label);
  assert(Value);
  assert(MinValue < MaxValue);

  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  float DragSize = g.Style.Vars[UI::VAR_DragMinSize];
  vec3  Size     = Window.GetItemSize();

  ui_id ID = Window.GetID(Label);

  float       ValueF    = (float)*Value;
  const float MinValueF = (float)MinValue;
  const float MaxValueF = (float)MaxValue;

  const float ValueRange   = MaxValue - MinValue;
  const float NormDragSize = DragSize / (Vertical ? Size.Y : Size.X);
  float       NormValue    = ((ValueF)-MinValue) / ValueRange;

  rect SliderRect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);
  if(!TestIfVisible(SliderRect))
  {
    return;
  }

  bool Held    = false;
  bool Hovered = false;
  rect DragRect =
    SliderBehavior(ID, SliderRect, &NormValue, NormDragSize, Vertical, &Held, &Hovered);
  ValueF = NormValue * ValueRange + MinValueF;
  *Value = (int32_t)(ValueF + 0.5f);

  DrawBox(SliderRect.MinP, SliderRect.GetSize(), _GetGUIColor(ScrollbarBox),
          _GetGUIColor(ScrollbarBox));
  DrawBox(DragRect.MinP, DragRect.GetSize(),
          Held ? _GetGUIColor(ScrollbarBox) : _GetGUIColor(ScrollbarDrag),
          _GetGUIColor(ScrollbarDrag));
  DrawText(SliderRect.MaxP +
             vec3{ g.Style.Vars[UI::VAR_InternalSpacing], -g.Style.Vars[UI::VAR_BoxPaddingY] },
           Label);

  char TempBuffer[20];
  snprintf(TempBuffer, sizeof(TempBuffer), "%d", *Value);
  DrawText({ SliderRect.MinP.X + g.Style.Vars[UI::VAR_BoxPaddingX],
             SliderRect.MinP.Y + SliderRect.GetHeight() - g.Style.Vars[UI::VAR_BoxPaddingY] },
           TempBuffer);
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
  assert(g.Style.Vars[UI::VAR_ScrollbarSize] <= Window->Size.X);
  assert(g.Style.Vars[UI::VAR_ScrollbarSize] <= Window->Size.Y);

  ui_id ID = Window->GetID(Vertical ? "##VertScrollbar" : "##HirizScrollbar");

  // WithoutScrollbars
  const float ScrollbarSize = g.Style.Vars[UI::VAR_ScrollbarSize];

  // Determine drag size
  float*      ScrollNorm = (Vertical) ? &Window->ScrollNorm.Y : &Window->ScrollNorm.X;
  const float NormDragSize =
    (Vertical) ? MaxFloat((Window->SizeNoScroll.Y < Window->ContentsSize.Y)
                            ? Window->SizeNoScroll.Y / Window->ContentsSize.Y
                            : 1,
                          g.Style.Vars[UI::VAR_DragMinSize] / Window->SizeNoScroll.Y)
               : MaxFloat((Window->SizeNoScroll.X < Window->ContentsSize.X)
                            ? Window->SizeNoScroll.X / Window->ContentsSize.X
                            : 1,
                          g.Style.Vars[UI::VAR_DragMinSize] / Window->SizeNoScroll.X);
  rect ScrollRect = (Vertical)
                      ? NewRect(Window->Position + vec3{ Window->SizeNoScroll.X, 0 },
                                Window->Position + vec3{ Window->Size.X, Window->SizeNoScroll.Y })
                      : NewRect(Window->Position + vec3{ 0, Window->SizeNoScroll.Y },
                                Window->Position + vec3{ Window->SizeNoScroll.X, Window->Size.Y });

  bool Held;
  bool Hovering;
  rect DragRect =
    SliderBehavior(ID, ScrollRect, ScrollNorm, NormDragSize, Vertical, &Held, &Hovering);

  DrawBox(ScrollRect.MinP, ScrollRect.GetSize(), _GetGUIColor(ScrollbarBox),
          _GetGUIColor(ScrollbarBox));
  DrawBox(DragRect.MinP, DragRect.GetSize(),
          Held ? _GetGUIColor(ScrollbarBox) : _GetGUIColor(ScrollbarDrag),
          _GetGUIColor(ScrollbarDrag));

  if(Vertical)
  {
    Window->ScrollRange.Y = (Window->SizeNoScroll.Y < Window->ContentsSize.Y)
                              ? Window->ContentsSize.Y - Window->SizeNoScroll.Y
                              : 0; // Delta in screen space that the window content can scroll
  }
  else
  {
    Window->ScrollRange.X = (Window->SizeNoScroll.X < Window->ContentsSize.X)
                              ? Window->ContentsSize.X - Window->SizeNoScroll.X
                              : 0; // Delta in screen space that the window content can scroll
  }
}

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
      break;
    }
  }

  if(!Window)
  {
    gui_window NewWindow  = {};
    size_t     NameLength = strlen(Name);
    assert(strlen(Name) < ARRAY_SIZE(NewWindow.Name.Name));
    strcpy(NewWindow.Name.Name, Name);
    NewWindow.ID                 = ID;
    NewWindow.MoveID             = NewWindow.GetID("##Move");
    NewWindow.Flags              = Flags;
    NewWindow.Size               = Size;
    NewWindow.Position           = InitialPosition;
    NewWindow.CurrentLineHeight  = 0;
    NewWindow.PreviousLineHeight = 0;
    NewWindow.UsedThisFrame      = false;
    NewWindow.UsedLastFrame      = false;
    Window                       = g.Windows.Append(NewWindow);
  }
  g.CurrentWindow = Window;

  Window->UsedThisFrame = true;
  if(!Window->UsedLastFrame && Window->UsedThisFrame)
  {
    g.OrderedWindows.Push(Window);
  }

  // Find parent and root windows
  Window->ParentWindow = (0 < g.CurrentWindowStack.Count) ? g.CurrentWindowStack.Peek() : NULL;
  assert(!(Window->Flags & WINDOW_IsChildWindow) || Window->ParentWindow != NULL);
  g.CurrentWindowStack.Push(Window);

  int RootWindowIndex;
  for(RootWindowIndex = g.CurrentWindowStack.Count - 1; RootWindowIndex >= 0; RootWindowIndex--)
  {
    if(!(g.CurrentWindowStack[RootWindowIndex]->Flags & WINDOW_IsChildWindow))
    {
      break;
    }
  }
  Window->RootWindow = g.CurrentWindowStack[RootWindowIndex];

  if(Window->Flags & WINDOW_Popup)
  {
    gui_popup* Popup = &g.OpenPopupStack[g.CurrentPopupStack.Count];
    Popup->Window    = Window;
    g.CurrentPopupStack.Push(*Popup);
    FocusWindow(Window);
  }
  if(Window->Flags & WINDOW_IsChildWindow)
  {
    assert(Window->ParentWindow);

    Window->Position          = Window->ParentWindow->CurrentPos;
    Window->IndexWithinParent = g.CurrentWindowStack.Count - 1;
    assert(0 < Window->IndexWithinParent);
    assert(Window->ParentWindow);
    Window->ParentWindow->ChildWindows.Append(Window);
  }
  if(Window->Flags & WINDOW_Combo)
  {
    Window->Position = Window->ParentWindow->CurrentPos;
    FocusWindow(Window);
    Window->Size = Size;
  }

  PushClipQuad(Window, Window->Position, Window->Size,
               (Window->Flags & WINDOW_IsChildWindow) &&
                 !(Window->Flags & (WINDOW_Popup | WINDOW_Combo)));
  Window->ClippedSizeRect = NewRect(Window->Position, Window->Position + Size); // used for hovering
  Window->ClippedSizeRect.Clip(g.ClipRectStack.Peek());                         // used for hovering
  DrawBox(Window->Position, Window->Size, _GetGUIColor(WindowBackground),
          _GetGUIColor(WindowBorder));

  // Order matters
  //#1
  {
    Window->SizeNoScroll = Window->Size;
    Window->SizeNoScroll -=
      vec3{ (Window->Flags & UI::WINDOW_UseVerticalScrollbar) ? g.Style.Vars[UI::VAR_ScrollbarSize]
                                                              : 0,
            (Window->Flags & UI::WINDOW_UseHorizontalScrollbar)
              ? g.Style.Vars[UI::VAR_ScrollbarSize]
              : 0 };
    if(Window->SizeNoScroll.Y < Window->ContentsSize.Y) // Add vertical Scrollbar
    {
      Window->Flags |= UI::WINDOW_UseVerticalScrollbar;
    }
    else
    {
      Window->Flags        = (Window->Flags & ~UI::WINDOW_UseVerticalScrollbar);
      Window->ScrollNorm.Y = 0;
    }
    if(Window->SizeNoScroll.X < Window->ContentsSize.X) // Add horizontal Scrollbar
    {
      Window->Flags |= UI::WINDOW_UseHorizontalScrollbar;
    }
    else
    {
      Window->Flags        = (Window->Flags & ~UI::WINDOW_UseHorizontalScrollbar);
      Window->ScrollNorm.X = 0;
    }
  }
  Window->SizeNoScroll = Window->Size;
  Window->SizeNoScroll -=
    vec3{ (Window->Flags & UI::WINDOW_UseVerticalScrollbar) ? g.Style.Vars[UI::VAR_ScrollbarSize]
                                                            : 0,
          (Window->Flags & UI::WINDOW_UseHorizontalScrollbar) ? g.Style.Vars[UI::VAR_ScrollbarSize]
                                                              : 0 };

  // Used to preserve scroll position accross content size changes
  // TODO(Lukas): could instead store current scroll position in pixels, would not need to recompute
  // the position when the content size changes
  {
    float NewScrollRangeX =
      MaxFloat(0, Window->ContentsSize.X -
                    Window->SizeNoScroll
                      .X); // Delta in screen space that the window content can scroll
    float NewScrollRangeY =
      MaxFloat(0, Window->ContentsSize.Y -
                    Window->SizeNoScroll
                      .Y); // Delta in screen space that the window content can scroll

    float PrevOffsetFromTop  = Window->ScrollNorm.Y * Window->ScrollRange.Y;
    float PrevOffsetFromLeft = Window->ScrollNorm.X * Window->ScrollRange.X;
    if(0 < NewScrollRangeX)
    {
      Window->ScrollNorm.X = ClampFloat(0, PrevOffsetFromLeft / NewScrollRangeX, 1);
    }
    if(0 < NewScrollRangeY)
    {
      Window->ScrollNorm.Y = ClampFloat(0, PrevOffsetFromTop / NewScrollRangeY, 1);
    }
  }

  //#2
  if(Window->Flags & UI::WINDOW_UseVerticalScrollbar)
    Scrollbar(g.CurrentWindow, true);
  if(Window->Flags & UI::WINDOW_UseHorizontalScrollbar)
    Scrollbar(g.CurrentWindow, false);

  //#Before 3
  Window->Padding =
    (Window->Flags & WINDOW_NoWindowPadding)
      ? vec3{}
      : vec3{ g.Style.Vars[UI::VAR_WindowPaddingX], g.Style.Vars[UI::VAR_WindowPaddingY] };

  //#3
  Window->IndentX = Window->Padding.X - Window->ScrollNorm.X * Window->ScrollRange.X;
  Window->StartPos = Window->MaxPos = Window->CurrentPos =
    Window->Position +
    vec3{ Window->IndentX, Window->Padding.Y - Window->ScrollNorm.Y * Window->ScrollRange.Y };

  PushClipQuad(Window, Window->Position, Window->SizeNoScroll);

  Window->DefaultItemWidth = (Window->Flags & WINDOW_Popup ? 1 : 0.65f) * Window->SizeNoScroll.X;
  // Note(Lukas) Added this in 2019, might not be the best place but seems good enough
  assert(Window->ItemWidthStack.Empty());
}

vec3
GetScrollAmountInPixels()
{
  gui_window& Window       = *GetCurrentWindow();
  vec3 ScrollAmount = { MaxFloat(0, Window.ContentsSize.X - Window.SizeNoScroll.X),
                        MaxFloat(0, Window.ContentsSize.Y - Window.SizeNoScroll.Y) };
  return ScrollAmount;
}

void
UI::EndWindow()
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
  vec3         MinPos = Window.Position - vec3{ Window.ScrollNorm.X * Window.ScrollRange.X,
                                        Window.ScrollNorm.Y * Window.ScrollRange.Y };
  Window.ContentsSize = Window.MaxPos - MinPos;

  //----------Window mooving-----------
  // Automatically sets g.ActiveID = Window.MoveID
  bool Held = false;
  ButtonBehavior(NewRect(Window.Position, Window.Position + Window.Size), Window.MoveID, &Held);
  //----------------------------------
  g.ClipRectStack.Pop();
  g.ClipRectStack.Pop();

  /*PushClipQuad(&Window, {}, { SCREEN_WIDTH, SCREEN_HEIGHT }, false);
  DrawBox(MinPos, Window.ContentsSize, { 1, 0, 1, 1 }, { 1, 1, 1, 1 });
  g.ClipRectStack.Pop();*/
  assert(Window.IDStack.Empty());

  g.CurrentWindowStack.Pop();
  g.CurrentWindow = (0 < g.CurrentWindowStack.Count) ? g.CurrentWindowStack.Peek() : NULL;
}

void
UI::BeginChildWindow(const char* Name, vec3 Size, window_flags_t Flags)
{
  UI::BeginWindow(Name, {}, Size, Flags | WINDOW_IsChildWindow);
}

void
UI::EndChildWindow()
{
  gui_window* Window = GetCurrentWindow();
  assert(Window->Flags & WINDOW_IsChildWindow);
  UI::EndWindow();
  AddSize(Window->Size);
}

void
UI::BeginPopupWindow(const char* Name, vec3 Size, window_flags_t Flags)
{
  // NOTE(Lukas): the WINDOW_NoWindowPadding flag is only a quick hack for combos, it can be removed
  // or replaced with no consequences
  UI::BeginWindow(Name, {}, Size,
                  Flags | WINDOW_Popup | WINDOW_IsNotMovable | WINDOW_IsNotResizable | WINDOW_NoWindowPadding);
}

void
UI::EndPopupWindow()
{
  gui_window* Window = GetCurrentWindow();
  assert(Window->Flags & WINDOW_Popup);
  gui_context& g = *GetContext();
  g.CurrentPopupStack.Pop();
  UI::EndWindow();
}

void
UI::Combo(const char* Label, int* CurrentItem, const char** Items, int ItemCount,
          int HeightInItems)
{
  UI::Combo(Label, CurrentItem, Items, ItemCount, StringArrayToString, HeightInItems);
}

void
UI::Combo(const char* Label, int* CurrentItem, const void* Data, int ItemCount,
          const char* (*DataToText)(const void*, int), int HeightInItems)
{
  gui_context& g      = *GetContext();
  gui_window*  Window = GetCurrentWindow();

  const ui_id ID = Window->GetID(Label);

  vec3 ButtonSize = Window->GetItemSize();

  const rect ButtonBB     = NewRect(Window->CurrentPos, Window->CurrentPos + ButtonSize);
  const rect ButtonTextBB = NewRect(ButtonBB.MinP, ButtonBB.MaxP - vec3{ ButtonSize.Y, 0 });
  const rect IconBB       = NewRect({ ButtonTextBB.MaxP.X, ButtonTextBB.MinP.Y }, ButtonBB.MaxP);

  const float ItemWidth   = ButtonSize.X;
  const float ItemHeight  = ButtonSize.Y;
  const float ItemSpacing = 0;
  const float PopupHeight = ItemHeight * (float)MinInt32(ItemCount + 1, HeightInItems);
  const rect  PopupBB     = NewRect({ ButtonBB.MinP.X, ButtonBB.MaxP.Y },
                               { ButtonBB.MaxP.X, ButtonBB.MaxP.Y + PopupHeight });

  if(!TestIfVisible(ButtonBB))
  {
    AddSize(ButtonBB.GetSize());
    UI::SameLine();
    UI::Text(Label);
    return;
  }
  AddSize(ButtonBB.GetSize());

  bool Hovered = IsHovered(ButtonBB, ID);
  if(Hovered)
  {
    SetHot(ID);
    if(g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      SetActive(0, NULL);
      if(!IsPopupOpen(ID))
      {
        FocusWindow(Window);
        OpenPopup(ID);
      }
    }
  }

  if(IsPopupOpen(ID))
  {
    // Remove the spacing between the box and window
    vec3 SavedCurrentPos = Window->CurrentPos;
    Window->CurrentPos   = ButtonBB.MinP + vec3{ 0, ButtonBB.GetHeight() };
		UI::BeginPopupWindow("Combo", PopupBB.GetSize(), WINDOW_Combo);
    gui_window* PopupWindow = GetCurrentWindow();

		UI::PushVar(UI::VAR_SpacingY, ItemSpacing);
    bool SelectedSomething = false;

		UI::PushColor(UI::COLOR_ButtonNormal, g.Style.Colors[UI::COLOR_ComboNormal]);
    if(UI::Button("    ", PopupWindow->SizeNoScroll.X))
    {
      *CurrentItem      = -1;
      SelectedSomething = true;
    }
    for(int i = 0; i < ItemCount; i++)
    {
      if(UI::Button(DataToText(Data, i), PopupWindow->SizeNoScroll.X))
      {
        *CurrentItem      = i;
        SelectedSomething = true;
      }
    }
		UI::PopColor();

		UI::PopVar();
		UI::EndPopupWindow();
    Window->CurrentPos = SavedCurrentPos;

    if(SelectedSomething)
    {
      ClosePopup(ID);
      FocusWindow(NULL);
    }
  }
  // So space appears after combo
  const char* ActiveText = (*CurrentItem < 0 || ItemCount <= *CurrentItem)
                             ? "not selected"
                             : DataToText(Data, *CurrentItem);

  DrawBox(ButtonTextBB.MinP, ButtonTextBB.GetSize() + vec3{ IconBB.GetWidth() },
          (Hovered) ? _GetGUIColor(ButtonHovered) : _GetGUIColor(ButtonNormal),
          _GetGUIColor(Border));
  DrawText(vec3{ ButtonBB.MinP.X, ButtonBB.MinP.Y + ItemHeight } +
             vec3{ g.Style.Vars[UI::VAR_BoxPaddingX], -g.Style.Vars[UI::VAR_BoxPaddingY] },
           ActiveText);
  PushTexturedQuad(Window, IconBB.MinP, IconBB.GetSize(), g.GameState->ExpandedTextureID);
#if 1
  UI::SameLine();
  UI::Text(Label);
#else
  DrawText(ButtonBB.MaxP +
             vec3{ g.Style.Vars[UI::VAR_InternalSpacing], -g.Style.Vars[UI::VAR_BoxPaddingY] },
           Label);
#endif
}

bool
UI::Button(const char* Label, float Width)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Label);

  vec3 Size;
  int  LabelWidth, LabelHeight;
  Text::GetTextSize(g.Font, Label, &LabelWidth, &LabelHeight);
  Size = vec3{ (float)LabelWidth, (float)LabelHeight } +
         2 * vec3{ g.Style.Vars[UI::VAR_BoxPaddingX], g.Style.Vars[UI::VAR_BoxPaddingY] };

	if(Width != 0)
	{
    Size.X = Width;
  }

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);

  AddSize(Size);
  if(!TestIfVisible(Rect))
  {
    return false;
  }

  bool Result     = ButtonBehavior(Rect, ID);
  vec4 InnerColor = (ID == g.ActiveID) ? _GetGUIColor(ButtonPressed)
                                       : ((ID == g.HotID) ? _GetGUIColor(ButtonHovered)
                                                          : _GetGUIColor(ButtonNormal));
  DrawBox(Rect.MinP, Size, InnerColor, _GetGUIColor(Border));
  DrawText(Rect.MinP +
             vec3{ g.Style.Vars[UI::VAR_BoxPaddingX], Size.Y - g.Style.Vars[UI::VAR_BoxPaddingY] },
           Label);

  return Result;
}

void
UI::Checkbox(const char* Label, bool* Toggle)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Label);

  int32_t LabelWidth, LabelHeight;
  Text::GetTextSize(g.Font, Label, &LabelWidth, &LabelHeight);
  vec3 LabelSize = { (float)LabelWidth, (float)LabelHeight };

  float CheckboxHeight = Window.GetItemSize().Y;

  const vec3  IconSize = vec3{ CheckboxHeight, CheckboxHeight };
  const rect  IconBB   = NewRect(Window.CurrentPos, Window.CurrentPos + IconSize);
  const rect& HitRegionBB =
    NewRect(Window.CurrentPos,
            Window.CurrentPos + IconSize +
              vec3{ LabelSize.X + 2 * g.Style.Vars[UI::VAR_InternalSpacing], 0 });

  AddSize(HitRegionBB.GetSize());
  if(!TestIfVisible(HitRegionBB))
  {
    return;
  }

  bool Released = ButtonBehavior(HitRegionBB, ID);
  if(Released)
  {
    *Toggle = !(*Toggle);
  }

  vec4 InnerColor =
    ((ID == g.HotID) ? _GetGUIColor(CheckboxHovered) : _GetGUIColor(CheckboxNormal));
  DrawBox(IconBB.MinP, IconSize, InnerColor, _GetGUIColor(Border));
  if(*Toggle)
  {
    PushColoredQuad(&Window, IconBB.MinP + 0.2f * IconSize, 0.6f * IconSize,
                    _GetGUIColor(CheckboxPressed));
  }

  DrawText(IconBB.MaxP +
             vec3{ g.Style.Vars[UI::VAR_InternalSpacing], -g.Style.Vars[UI::VAR_BoxPaddingY] },
           Label);
}

bool
UI::TreeNode(const char* Label, bool* IsExpanded)
{
  gui_context& g = *GetContext();

  UI::PushColor(COLOR_HeaderNormal, {});
  bool Expanded = UI::CollapsingHeader(Label, IsExpanded, false);
  UI::PopColor();

  if(Expanded)
  {
    UI::Indent();
	}

  return Expanded;
}

void
UI::TreePop()
{
  gui_context& g = *GetContext();
  UI::Unindent();
}

bool
UI::CollapsingHeader(const char* Label, bool* IsExpanded, bool UseAvailableWidth)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Label);

  const float BoxHeight        = Window.GetItemSize().Y;
  const float IconWidth        = BoxHeight;
  const float ArrowIconOffsetX = g.Style.Vars[UI::VAR_BoxPaddingX];
  const float TextOffsetX      = ArrowIconOffsetX + BoxHeight + g.Style.Vars[UI::VAR_BoxPaddingX];

  vec3 Size = {};
  if(UseAvailableWidth)
  {
    Size = { GetAvailableWidth(), BoxHeight };
  }
  else
  {
    int TextWidth, TextHeight;
    Text::GetTextSize(g.Font, Label, &TextWidth, &TextHeight);
    Size = vec3{ ArrowIconOffsetX + IconWidth + (float)TextWidth, (float)TextHeight } +
           2 * vec3{ g.Style.Vars[UI::VAR_BoxPaddingX], g.Style.Vars[UI::VAR_BoxPaddingY] };
  }

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);

  if(!TestIfVisible(Rect))
  {
    return *IsExpanded;
  }

  bool Result = ButtonBehavior(Rect, ID);
  *IsExpanded = (Result) ? !*IsExpanded : *IsExpanded;

  vec4 InnerColor = (ID == g.ActiveID) ? _GetGUIColor(HeaderPressed)
                                       : ((ID == g.HotID) ? _GetGUIColor(HeaderHovered)
                                                          : _GetGUIColor(HeaderNormal));
  DrawBox({ Rect.MinP.X, Rect.MinP.Y }, Size, InnerColor, _GetGUIColor(Border));
  DrawText(vec3{ Rect.MinP.X, Rect.MinP.Y } +
             vec3{ TextOffsetX, Size.Y - g.Style.Vars[UI::VAR_BoxPaddingY] },
           Label);

  // draw square icon
  int32_t IconTextureID =
    (*IsExpanded) ? g.GameState->ExpandedTextureID : g.GameState->CollapsedTextureID;
  PushTexturedQuad(&Window, Rect.MinP + vec3{ ArrowIconOffsetX }, { Size.Y, Size.Y },
                   IconTextureID);

  return *IsExpanded;
}

static bool
ButtonBehavior(rect BB, ui_id ID, bool* OutHeld, bool* OutHovered, UI::button_flags_t Flags)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  if(!(Flags & (UI::BUTTON_PressOnClick | UI::BUTTON_PressOnHold)))
  {
    Flags = UI::BUTTON_PressOnRelease;
  }

  bool Hovered = IsHovered(BB, ID);
  if(Hovered)
  {
    SetHot(ID);
  }

  bool Result = false;
  if(ID == g.HotID)
  {
    if((Flags & UI::BUTTON_PressOnRelease) && g.Input->MouseLeft.EndedDown &&
       g.Input->MouseLeft.Changed)
    {
      SetActive(ID, &Window);
    }
    else if((Flags & UI::BUTTON_PressOnClick) && g.Input->MouseLeft.EndedDown &&
            g.Input->MouseLeft.Changed)
    {
      SetActive(0, NULL);
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
    else if((Flags & UI::BUTTON_PressOnRelease) && !g.Input->MouseLeft.EndedDown &&
            g.Input->MouseLeft.Changed)
    {
      if(ID == g.HotID)
      {
        Result = true;
      }
      SetActive(0, NULL);
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
SliderBehavior(ui_id ID, rect BB, float* ScrollNorm, float NormDragSize, bool Vertical,
               bool* OutHeld, bool* OutHovering)
{
  assert(BB.MinP.X < BB.MaxP.X);
  assert(BB.MinP.Y < BB.MaxP.Y);
  *ScrollNorm  = ClampFloat(0, *ScrollNorm, 1);
  NormDragSize = ClampFloat(0, NormDragSize, 1);

  gui_context& g = *GetContext();

  // Determine actual drag size
  const float RegionSize = (Vertical) ? BB.GetHeight() : BB.GetWidth();
  assert(g.Style.Vars[UI::VAR_DragMinSize] <= RegionSize);

  const float DragSize = (NormDragSize * RegionSize < g.Style.Vars[UI::VAR_DragMinSize])
                           ? g.Style.Vars[UI::VAR_DragMinSize]
                           : NormDragSize * RegionSize;
  const float MovableRegionSize = RegionSize - DragSize;
  const float DragOffset        = (*ScrollNorm) * MovableRegionSize;

  rect DragRect;
  if(Vertical)
  {
    DragRect = NewRect(BB.MinP.X, BB.MinP.Y + DragOffset, BB.MinP.X + BB.GetWidth(),
                       BB.MinP.Y + DragOffset + DragSize);
  }
  else
  {
    DragRect = NewRect(BB.MinP.X + DragOffset, BB.MinP.Y, BB.MinP.X + DragOffset + DragSize,
                       BB.MinP.Y + BB.GetHeight());
  }

  bool Held     = false;
  bool Hovering = false;
  ButtonBehavior(BB, ID, &Held, &Hovering);
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
    *ScrollNorm = (MovableRegionSize > 0.00001f)
                    ? (((*ScrollNorm) - ScreenRegionStart) / MovableRegionSize)
                    : 0;

    if(Vertical)
    {
      DragRect = NewRect(BB.MinP.X, BB.MinP.Y + DragOffset, BB.MinP.X + BB.GetWidth(),
                         BB.MinP.Y + DragOffset + DragSize);
    }
    else
    {
      DragRect = NewRect(BB.MinP.X + DragOffset, BB.MinP.Y, BB.MinP.X + DragOffset + DragSize,
                         BB.MinP.Y + BB.GetHeight());
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

static void
DragBehavior(ui_id ID, rect BB, float* Value, float MinValue, float MaxValue, float DeltaPerScreen,
             bool* OutHeld, bool* OutHovering)
{
  assert(BB.MinP.X < BB.MaxP.X);
  assert(BB.MinP.Y < BB.MaxP.Y);
  assert(MinValue < MaxValue);

  gui_context& g = *GetContext();

  bool Held     = false;
  bool Hovering = false;
  bool Clicked  = ButtonBehavior(BB, ID, &Held, &Hovering);

  float Delta = 0;
  if(Held)
  {
    Delta = ((float)g.Input->dMouseScreenX / (float)SCREEN_WIDTH) * DeltaPerScreen;
  }
  *Value = ClampFloat(MinValue, *Value + Delta, MaxValue);

  if(OutHeld)
  {
    *OutHeld = Held;
  }
  if(OutHovering)
  {
    *OutHovering = Hovering;
  }
}

void
UI::DragFloat(const char* Label, float* Value, float MinValue, float MaxValue, float ScreenDelta)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID;
  if(Label)
  {
    ID = Window.GetID(Label);
  }
  else
  {
    ID = Window.GetID((uintptr_t)Value);
  }

  vec3 Size = Window.GetItemSize();

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);

  if(!TestIfVisible(Rect))
  {
    return;
  }

  bool Held    = false;
  bool Hovered = false;
  DragBehavior(ID, Rect, Value, MinValue, MaxValue, ScreenDelta, &Held, &Hovered);

  vec4 InnerColor = (Held) ? _GetGUIColor(ButtonPressed)
                           : ((Hovered) ? _GetGUIColor(ButtonHovered) : _GetGUIColor(ButtonNormal));
  DrawBox(Rect.MinP, Size, InnerColor, _GetGUIColor(Border));
  if(Label)
  {
    DrawText(Rect.MaxP +
               vec3{ g.Style.Vars[UI::VAR_BoxPaddingX], -g.Style.Vars[UI::VAR_BoxPaddingY] },
             Label);
  }

  char TempBuffer[20];
  snprintf(TempBuffer, sizeof(TempBuffer), "%.4f", *Value);
  DrawText({ Rect.MinP.X + g.Style.Vars[UI::VAR_BoxPaddingX],
             Rect.MinP.Y + Rect.GetHeight() - g.Style.Vars[UI::VAR_BoxPaddingY] },
           TempBuffer);
}

void
UI::SameLine(float PosX, float SpacingWidth)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
  if(PosX != 0) // Absolute positioning from the left edge - scrolling
  {
    if(SpacingWidth < 0)
    {
      SpacingWidth = 0;
    }
    Window.CurrentPos.X = Window.Position.X + Window.IndentX + PosX + SpacingWidth;
  }
  else // Continue from last item
  {
    if(SpacingWidth < 0)
    {
      SpacingWidth = g.Style.Vars[UI::VAR_SpacingX];
    }
    Window.CurrentPos.X = Window.PreviousLinePos.X + SpacingWidth;
  }
  Window.CurrentPos.Y      = Window.PreviousLinePos.Y;
  Window.CurrentLineHeight = Window.PreviousLineHeight;
}

void
UI::NewLine()
{
  gui_window&  Window = *GetCurrentWindow();
	Window.PreviousLineHeight = Window.CurrentLineHeight;

	if(Window.CurrentLineHeight > 0)
	{
    AddSize({0, 0});
  }
  // TODO(Lukas): when the line is empty add a standard item sized vertical space
}

void
UI::Dummy(float Width, float Height)
{
  gui_window& Window = *GetCurrentWindow();

  vec3 Size = Window.GetItemSize();
	if(Width != 0)
		Size.X = Width;

	if(Height != 0)
    Size.X = Height;

  AddSize(Size);
}

float
UI::GetWindowWidth()
{
  gui_window& Window = *GetCurrentWindow();
  return Window.Size.X;
}

float
UI::GetAvailableWidth()
{
  gui_window& Window = *GetCurrentWindow();
  return MaxFloat(0, Window.Position.X + Window.SizeNoScroll.X - Window.CurrentPos.X -
                       Window.Padding.X);
}

void
UI::PushID(const void* ID)
{
  gui_window* Window = GetCurrentWindow();
  Window->IDStack.Push(Window->GetID(ID));
}

void
UI::PushID(const int ID)
{
  gui_window* Window = GetCurrentWindow();
  Window->IDStack.Push(Window->GetID(ID));
}

void
UI::PushID(const char* ID)
{
  gui_window* Window = GetCurrentWindow();
  Window->IDStack.Push(Window->GetID(ID));
}

void
UI::PopID()
{
  gui_window* Window = GetCurrentWindow();
  Window->IDStack.Pop();
}

void
UI::PushVar(int32_t Index, float Value)
{
  gui_context& g = *GetContext();
  assert(0 <= Index && Index < UI::VAR_Count);

  style_var_memo Memo = {};
  Memo.Value          = g.Style.Vars[Index];
  Memo.Index          = Index;
  g.StyleVarStack.Push(Memo);
  g.Style.Vars[Index] = Value;
}

void
UI::PopVar()
{
  gui_context& g = *GetContext();

  assert(0 < g.StyleVarStack.Count);
  style_var_memo Memo      = g.StyleVarStack.Pop();
  g.Style.Vars[Memo.Index] = Memo.Value;
}

void
UI::PushColor(int32_t Index, vec4 Color)
{
  gui_context& g = *GetContext();
  assert(0 <= Index && Index < UI::COLOR_Count);

  style_color_memo Memo = {};
  Memo.Color            = g.Style.Colors[Index];
  Memo.Index            = Index;
  g.StyleColorStack.Push(Memo);
  g.Style.Colors[Index] = Color;
}

void
UI::PopColor()
{
  gui_context& g = *GetContext();

  assert(0 < g.StyleColorStack.Count);
  style_color_memo Memo      = g.StyleColorStack.Pop();
  g.Style.Colors[Memo.Index] = Memo.Color;
}

void
UI::PushWidth(float Width)
{
  gui_window& Window = *GetCurrentWindow();
  if(Width == 0)
    Width = Window.GetItemSize().X;

	if(Width < 0)
	{
    Width = MaxFloat(1, UI::GetAvailableWidth() + Width);
  }

  Window.ItemWidthStack.Push(Width);
}

void
UI::PopWidth()
{
  gui_window&  Window = *GetCurrentWindow();
  Window.ItemWidthStack.Pop();
}

void
UI::Indent(float IndentWidth)
{
  gui_context& g = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
	if(IndentWidth == 0)
		IndentWidth = g.Style.Vars[UI::VAR_IndentSpacing];

	Window.IndentX += IndentWidth;
  Window.CurrentPos.X = Window.Position.X + Window.IndentX;
}

void
UI::Unindent(float IndentWidth)
{
  gui_context& g = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
	if(IndentWidth == 0)
		IndentWidth = g.Style.Vars[UI::VAR_IndentSpacing];

  Window.IndentX -= IndentWidth;
  Window.CurrentPos.X = Window.Position.X + Window.IndentX;
}

void
UI::DragFloat3(const char* Label, float Value[3], float MinValue, float MaxValue, float ScreenDelta)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  vec3 Size = Window.GetItemSize();
  vec3 TextPosition =
    Window.CurrentPos + Size +
    vec3{ g.Style.Vars[UI::VAR_InternalSpacing], -g.Style.Vars[UI::VAR_BoxPaddingY] };
  float SingleSliderWidth = Size.X / 3;

  rect DragRect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  if(!TestIfVisible(DragRect))
  {
    AddSize(Size);
    return;
  }

  PushVar(UI::VAR_SpacingX, 0);
  PushWidth(SingleSliderWidth);

  UI::DragFloat(NULL, &Value[0], MinValue, MaxValue, ScreenDelta);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[1], MinValue, MaxValue, ScreenDelta);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[2], MinValue, MaxValue, ScreenDelta);

  PopWidth();
  PopVar();

  DrawText(TextPosition, Label);
}

void
UI::DragFloat4(const char* Label, float Value[4], float MinValue, float MaxValue, float ScreenDelta)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  vec3 Size = Window.GetItemSize();
  vec3 TextPosition =
    Window.CurrentPos + Size +
    vec3{ g.Style.Vars[UI::VAR_InternalSpacing], -g.Style.Vars[UI::VAR_BoxPaddingY] };
  float SingleSliderWidth = Size.X / 4;

  rect DragRect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  if(!TestIfVisible(DragRect))
  {
    AddSize(Size);
    return;
  }

  PushVar(UI::VAR_SpacingX, 0);
	UI::PushWidth(SingleSliderWidth);

  UI::DragFloat(NULL, &Value[0], MinValue, MaxValue, ScreenDelta);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[1], MinValue, MaxValue, ScreenDelta);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[2], MinValue, MaxValue, ScreenDelta);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[3], MinValue, MaxValue, ScreenDelta);

  UI::PopWidth();
  PopVar();

  DrawText(TextPosition, Label);
}

void
UI::Image(const char* Name, int32_t TextureID, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  rect ImageRect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);
  if(!TestIfVisible(ImageRect))
  {
    return;
  }
  PushTexturedQuad(&Window, ImageRect.MinP, ImageRect.GetSize(), TextureID);
}

void
UI::Text(const char* Text)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  int32_t TextWidth, TextHeight;
  Text::GetTextSize(g.Font, Text, &TextWidth, &TextHeight);
  vec3 Size = { (float)TextWidth, (float)TextHeight };

  rect TextRect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);
  if(!TestIfVisible(TextRect))
  {
    return;
  }
  DrawText({ TextRect.MinP.X, TextRect.MinP.Y + Size.Y }, Text);
}
#include "intersection_testing.h"

void
_MoveAxis()
{
}

void _MovePlane(parametric_plane) {}

float
GetGizmoScale(vec3 Position)
{
  gui_context& g = *GetContext();
  return g.GameState->R.GizmoScaleFactor * Math::Length(Position - g.GameState->Camera.Position);
}

void
_MoveAxes(vec3* Position, const vec3 InputAxes[3])
{
  vec3 Axes[3] = { InputAxes[0], InputAxes[1], InputAxes[2] };

  gui_context& g = *GetContext();

  ui_id ID = { IDHash(&Position, sizeof(vec3*), 0) };

  float GizmoScale = GetGizmoScale(*Position);

  float AxisRadius = 0.08f * GizmoScale;
  float AxisWorldLength = 0.85f * GizmoScale;

  static parametric_plane AxisPlane = {};
  static float            InitialU  = 0.0f;

  vec3 RayDir =
    GetRayDirFromScreenP({ g.Input->NormMouseX, g.Input->NormMouseY },
                         g.GameState->Camera.ProjectionMatrix, g.GameState->Camera.ViewMatrix);
  vec3 RayOrig = g.GameState->Camera.Position + RayDir * g.GameState->Camera.NearClipPlane;

  if(g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
  {
    float DistXOrig;
    float DistYOrig;
    float DistZOrig;
    float HitDistX = MinDistRaySegment(&DistXOrig, RayOrig, RayDir, *Position,
                                       *Position + AxisWorldLength * Axes[0]);
    float HitDistY = MinDistRaySegment(&DistYOrig, RayOrig, RayDir, *Position,
                                       *Position + AxisWorldLength * Axes[1]);
    float HitDistZ = MinDistRaySegment(&DistZOrig, RayOrig, RayDir, *Position,
                                       *Position + AxisWorldLength * Axes[2]);

    if(MinFloat(HitDistX, MinFloat(HitDistY, HitDistZ)) < AxisRadius)
    {
      int   AxisIndex     = -1;
      float MinOrigToAxis = FLT_MAX;
      if(HitDistX < AxisRadius && DistXOrig < MinOrigToAxis)
      {
        AxisIndex     = 0;
        MinOrigToAxis = DistXOrig;
      }
      if(HitDistY < AxisRadius && DistYOrig < MinOrigToAxis)
      {
        AxisIndex     = 1;
        MinOrigToAxis = DistYOrig;
      }
      if(HitDistZ < AxisRadius && DistZOrig < MinOrigToAxis)
      {
        AxisIndex     = 2;
        MinOrigToAxis = DistZOrig;
      }

      assert(AxisIndex != -1);
      AxisPlane.u = Axes[AxisIndex];
      vec3 tempV  = Math::Cross(AxisPlane.u, RayDir);
      AxisPlane.n = Math::Normalized(Math::Cross(AxisPlane.u, tempV));
      AxisPlane.o = *Position;

      if(IntersectRayParametricPlane(&InitialU, RayOrig, RayDir, AxisPlane))
      {
        SetHot(ID);
        if(ID == g.HotID)
        {
          SetActive(ID, NULL);
        }
      }
    }
  }
  else if(g.Input->MouseLeft.EndedDown && ID == g.ActiveID)
  {
    float CurrentU;
    if(IntersectRayParametricPlane(&CurrentU, RayOrig, RayDir, AxisPlane))
    {
      vec3 ClosestPOnAxis = AxisPlane.o + CurrentU * AxisPlane.u;
      Debug::PushWireframeSphere(ClosestPOnAxis, AxisRadius);
      *Position = AxisPlane.o + (CurrentU - InitialU) * AxisPlane.u;

      // Snap To Grid
      if(g.Input->LeftCtrl.EndedDown)
      {
        float AbsoluteU = Math::Dot(*Position, AxisPlane.u);
        float RoundedAbsoluteU = roundf(AbsoluteU);
        float CorrectionU      = RoundedAbsoluteU - AbsoluteU;
        *Position += CorrectionU * AxisPlane.u;
      }
    }
    else
    {
      *Position = AxisPlane.o;
    }
  }
  else if(!g.Input->MouseLeft.EndedDown && ID == g.ActiveID)
  {
    SetActive(0, NULL);
  }

  Debug::PushGizmo(&g.GameState->Camera, Math::Mat4Translate(*Position));
}

void
_MovePlanes(vec3* Position, const vec3 InputAxes[3], float PlaneQuadWidth = 0.3f)
{
  gui_context& g  = *GetContext();
  ui_id        ID = { IDHash(&Position, sizeof(vec3*), 1) };

  vec3 Axes[3] = { InputAxes[0], InputAxes[1], InputAxes[2] };

  // Variables stored between calls of the funtion (Only used when ID == g.ActiveID)
  static parametric_plane ActivePlane;
  static int              ActivePlaneIndex;
  static float            InitialU;
  static float            InitialV;

  float GizmoScale      = GetGizmoScale(*Position);
  float PlaneWorldWidth = GizmoScale * PlaneQuadWidth;

  // Flipping the axes to face the camera
  vec3 GizmoToCamera = g.GameState->Camera.Position - *Position;
  for(int i = 0; i < 3; i++)
  {
    if(Math::Dot(GizmoToCamera, Axes[i]) < 0)
    {
      Axes[i] *= -1;
    }
  }

  vec3 RayDir =
    GetRayDirFromScreenP({ g.Input->NormMouseX, g.Input->NormMouseY },
                         g.GameState->Camera.ProjectionMatrix, g.GameState->Camera.ViewMatrix);
  vec3 RayOrig = g.GameState->Camera.Position + RayDir * g.GameState->Camera.NearClipPlane;
  if(g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
  {

    for(int i = 0; i < 3; i++)
    {
      // Construct the plane
      parametric_plane TempPlane;
      TempPlane.o = *Position;
      TempPlane.u = Axes[(i + 1) % 3];
      TempPlane.v = Axes[(i + 2) % 3];

      float TempInitialU;
      float TempInitialV;
      if(IntersectRayParametricPlane(&TempInitialU, &TempInitialV, RayOrig, RayDir, TempPlane, 0.0f,
                                     PlaneWorldWidth))
      {
        SetHot(ID);
        if(g.HotID == ID)
        {
          SetActive(ID, NULL);
          ActivePlaneIndex = i;
          ActivePlane      = TempPlane;
          InitialU         = TempInitialU;
          InitialV         = TempInitialV;
          break;
        }
      }
    }
  }
  else if(g.Input->MouseLeft.EndedDown && ID == g.ActiveID)
  {
    float CurrentU;
    float CurrentV;
    if(IntersectRayParametricPlane(&CurrentU, &CurrentV, RayOrig, RayDir, ActivePlane, -FLT_MAX,
                                   FLT_MAX))
    {
      *Position = ActivePlane.o + (CurrentU - InitialU) * ActivePlane.u +
                  (CurrentV - InitialV) * ActivePlane.v;
      // Snap to grid
      if(g.Input->LeftCtrl.EndedDown)
      {
        vec3  Normal           = Math::Cross(ActivePlane.u, ActivePlane.v);
        float GizmoSpacePos[3] = { Math::Dot(*Position, ActivePlane.u),
                                   Math::Dot(*Position, ActivePlane.v),
                                   Math::Dot(*Position, Normal) };

        GizmoSpacePos[0] = roundf(GizmoSpacePos[0]);
        GizmoSpacePos[1] = roundf(GizmoSpacePos[1]);

        *Position = GizmoSpacePos[0] * ActivePlane.u + GizmoSpacePos[1] * ActivePlane.v +
                    GizmoSpacePos[2] * Normal;
      }
    }
  }
  else if(ID == g.ActiveID)
  {
    SetActive(0, NULL);
  }

  // Draw the planes
  vec4 PlaneColors[3] = { { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 } };
  for(int i = 0; i < 3; i++)
  {
    if(ID != g.ActiveID || (ID == g.ActiveID && i == ActivePlaneIndex))
    {
      int IndA = (i + 1) % 3;
      int IndB = (i + 2) % 3;

      vec3 PtAxisA  = *Position + PlaneWorldWidth * Axes[IndA];
      vec3 PtAxisB  = *Position + PlaneWorldWidth * Axes[IndB];
      vec3 PtCorner = *Position + PlaneWorldWidth * (Axes[IndA] + Axes[IndB]);
      Debug::PushLine(*Position, PtAxisA, PlaneColors[i]);
      Debug::PushLine(*Position, PtAxisB, PlaneColors[i]);
      Debug::PushLine(PtAxisA, PtCorner, PlaneColors[i]);
      Debug::PushLine(PtAxisB, PtCorner, PlaneColors[i]);
    }
  }
}

bool
UI::SelectSphere(vec3* Position, float Radius, vec4 Color, bool PerspectiveInvariant)
{
  gui_context& g = *GetContext();

  ui_id ID = { IDHash(&Position, sizeof(vec3*), 2) };

  static float ClosestActiveT  = -FLT_MAX;

  bool Result = false;

  vec3 RayDir =
    GetRayDirFromScreenP({ g.Input->NormMouseX, g.Input->NormMouseY },
                         g.GameState->Camera.ProjectionMatrix, g.GameState->Camera.ViewMatrix);
  vec3 RayOrig = g.GameState->Camera.Position + RayDir * g.GameState->Camera.NearClipPlane;
  raycast_result Raycast = RayIntersectSphere(RayOrig, RayDir, *Position, Radius);

  if(Raycast.Success)
  {
    Color += { 0.5f, 0.5f, 0.5f, 0 };
    ClampFloat(0, Color.X, 1);
    ClampFloat(0, Color.Y, 1);
    ClampFloat(0, Color.Z, 1);
  }

  if(Raycast.Success && g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
  {
    if(ClosestActiveT != -FLT_MAX && Raycast.t < ClosestActiveT)
    {
      SetActive(0, NULL); // Clear out the further sphere's selection
    }
    SetHot(ID);
    if(ID == g.HotID)
    {
      ClosestActiveT = Raycast.t;
      Result = true;
      SetActive(ID, NULL);
    }
  }
  else if(ID == g.ActiveID)
  {
    SetActive(0, NULL);
    ClosestActiveT = -FLT_MAX;
  }

  Debug::PushWireframeSphere(*Position, Radius, Color);

  return Result;
}

void
UI::MoveGizmo(vec3* Position)
{
  vec3 Axes[3];
  Axes[0] = { 1, 0, 0 };
  Axes[1] = { 0, 1, 0 };
  Axes[2] = { 0, 0, 1 };

  _MovePlanes(Position, Axes);
  _MoveAxes(Position, Axes);
}

void
UI::MoveGizmo(transform* Transform, bool UseLocalAxes)
{
  assert(!UseLocalAxes);

  vec3 Axes[3];
  if(!UseLocalAxes)
  {
    Axes[0] = { 1, 0, 0 };
    Axes[1] = { 0, 1, 0 };
    Axes[2] = { 0, 0, 1 };
  }
  else
  {
    assert(0 && "PleaseImplementLocalMoveGizmoAxes");
  }

  _MovePlanes(&Transform->T, Axes);
  _MoveAxes(&Transform->T, Axes);
}

