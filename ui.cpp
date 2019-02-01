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
  vec3  Size     = Window.GetDefaultItemSize();

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
UI::SliderInt(const char* Label, int32_t* Value, int32_t MinValue, int32_t MaxValue, bool Vertical)
{
  assert(Label);
  assert(Value);
  assert(MinValue < MaxValue);

  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  float DragSize = g.Style.Vars[UI::VAR_DragMinSize];
  vec3  Size     = Window.GetDefaultItemSize();

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
    NewWindow.ID            = ID;
    NewWindow.MoveID        = NewWindow.GetID("##Move");
    NewWindow.Flags         = Flags;
    NewWindow.Size          = Size;
    NewWindow.Position      = InitialPosition;
    NewWindow.UsedThisFrame = false;
    NewWindow.UsedLastFrame = false;
    Window                  = g.Windows.Append(NewWindow);
  }
  g.CurrentWindow = Window;

  Window->UsedThisFrame = true;
  if(!Window->UsedLastFrame && Window->UsedThisFrame)
  {
    g.OrderedWindows.Push(Window);
  }

  // Find parent and root windows
  Window->ParentWindow = (0 < g.CurrentWindowStack.Count) ? g.CurrentWindowStack.Back() : NULL;
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
  Window->ClippedSizeRect.Clip(g.ClipRectStack.Back());                         // used for hovering
  DrawBox(Window->Position, Window->Size, _GetGUIColor(WindowBackground),
          _GetGUIColor(WindowBorder));

  // Order matters
  //#1
  Window->SizeNoScroll = Window->Size;
  Window->SizeNoScroll -=
    vec3{ (Window->Flags & UI::WINDOW_UseVerticalScrollbar) ? g.Style.Vars[UI::VAR_ScrollbarSize]
                                                            : 0,
          (Window->Flags & UI::WINDOW_UseHorizontalScrollbar) ? g.Style.Vars[UI::VAR_ScrollbarSize]
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
  Window->SizeNoScroll = Window->Size;
  Window->SizeNoScroll -=
    vec3{ (Window->Flags & UI::WINDOW_UseVerticalScrollbar) ? g.Style.Vars[UI::VAR_ScrollbarSize]
                                                            : 0,
          (Window->Flags & UI::WINDOW_UseHorizontalScrollbar) ? g.Style.Vars[UI::VAR_ScrollbarSize]
                                                              : 0 };

  // Used to preserve scroll position accross content size changes
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

  //#3
  Window->StartPos = Window->MaxPos = Window->CurrentPos =
    Window->Position - vec3{ Window->ScrollNorm.X * Window->ScrollRange.X,
                             Window->ScrollNorm.Y * Window->ScrollRange.Y };
  PushClipQuad(Window, Window->Position, Window->SizeNoScroll);

  Window->DefaultItemWidth = (Window->Flags & WINDOW_Popup ? 1 : 0.65f) * Window->SizeNoScroll.X;
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

  g.CurrentWindowStack.Pop();
  g.CurrentWindow = (0 < g.CurrentWindowStack.Count) ? g.CurrentWindowStack.Back() : NULL;
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
  UI::BeginWindow(Name, {}, Size,
                  Flags | WINDOW_Popup | WINDOW_IsNotMovable | WINDOW_IsNotResizable);
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
UI::Combo(const char* Label, int* CurrentItem, const char** Items, int ItemCount, int HeightInItems,
          float Width)
{
  UI::Combo(Label, CurrentItem, Items, ItemCount, StringArrayToString, HeightInItems, Width);
}

void
UI::Combo(const char* Label, int* CurrentItem, void* Data, int ItemCount,
          char* (*DataToText)(void*, int), int HeightInItems, float Width)
{
  gui_context& g      = *GetContext();
  gui_window*  Window = GetCurrentWindow();

  const ui_id ID = Window->GetID(Label);

  vec3 ButtonSize = Window->GetDefaultItemSize();

  assert(0 <= Width);
  if(0 < Width)
  {
    ButtonSize.X = Width;
  }

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
    BeginPopupWindow("Combo", PopupBB.GetSize(), WINDOW_Combo);
    gui_window* PopupWindow = GetCurrentWindow();

    PushStyleVar(UI::VAR_SpacingY, ItemSpacing);
    bool SelectedSomething = false;
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
    PopStyleVar();
    EndPopupWindow();
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

  DrawBox(ButtonTextBB.MinP, ButtonTextBB.GetSize(),
          (Hovered) ? _GetGUIColor(ButtonHovered) : _GetGUIColor(ButtonNormal),
          _GetGUIColor(Border));
  DrawText(vec3{ ButtonBB.MinP.X, ButtonBB.MinP.Y + ItemHeight } +
             vec3{ g.Style.Vars[UI::VAR_BoxPaddingX], -g.Style.Vars[UI::VAR_BoxPaddingY] },
           ActiveText);
  PushTexturedQuad(Window, IconBB.MinP, IconBB.GetSize(), g.GameState->ExpandedTextureID);
  DrawText(ButtonBB.MaxP +
             vec3{ g.Style.Vars[UI::VAR_InternalSpacing], -g.Style.Vars[UI::VAR_BoxPaddingY] },
           Label);
}

bool
UI::Button(const char* Label, float Width)
{
  assert(0 <= Width);
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

  float CheckboxHeight = Window.GetDefaultItemSize().Y;

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
UI::CollapsingHeader(const char* Label, bool* IsExpanded)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Label);

  const float Height = Window.GetDefaultItemSize().Y;
  const vec3  Size   = { Window.SizeNoScroll.X, Height };
  const rect& Rect   = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);

  if(!TestIfVisible(Rect))
  {
    return *IsExpanded;
  }

  bool Result = ButtonBehavior(Rect, ID);
  *IsExpanded = (Result) ? !*IsExpanded : *IsExpanded;

  // draw square icon
  int32_t IconTextureID =
    (*IsExpanded) ? g.GameState->ExpandedTextureID : g.GameState->CollapsedTextureID;
  PushTexturedQuad(&Window, Rect.MinP, { Size.Y, Size.Y }, IconTextureID);

  vec4 InnerColor = (ID == g.ActiveID) ? _GetGUIColor(HeaderPressed)
                                       : ((ID == g.HotID) ? _GetGUIColor(HeaderHovered)
                                                          : _GetGUIColor(HeaderNormal));
  DrawBox({ Rect.MinP.X + Size.Y, Rect.MinP.Y }, { Size.X - Size.Y, Size.Y }, InnerColor,
          _GetGUIColor(Border));
  DrawText(vec3{ Rect.MinP.X, Rect.MinP.Y } + vec3{ Size.Y + g.Style.Vars[UI::VAR_BoxPaddingX],
                                                    Size.Y - g.Style.Vars[UI::VAR_BoxPaddingY] },
           Label);

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
UI::DragFloat(const char* Label, float* Value, float MinValue, float MaxValue, float ScreenDelta,
              float Width)
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

  vec3 Size = Window.GetDefaultItemSize();
  if(Width != 0)
  {
    Size.X = Width;
  }

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
UI::SameLine()
{
  gui_window& Window = *GetCurrentWindow();
  Window.CurrentPos  = Window.PreviousPos;
}

void
UI::NewLine()
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
  Window.CurrentPos.X = Window.StartPos.X;
  Window.CurrentPos.Y = Window.MaxPos.Y + g.Style.Vars[UI::VAR_SpacingY];
}

void
UI::Dummy(float Width, float Height)
{
  AddSize(vec3{ Width, Height, 0 });
}

float
UI::GetWindowWidth()
{
  gui_window& Window = *GetCurrentWindow();
  return Window.Size.X;
}

// TODO(Lukas): Subtract various padding values
float
UI::GetAvailableWidth()
{
  gui_window& Window = *GetCurrentWindow();
  return Window.SizeNoScroll.X;
}

void
UI::PushStyleVar(int32_t Index, float Value)
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
UI::PopStyleVar()
{
  gui_context& g = *GetContext();

  assert(0 < g.StyleVarStack.Count);
  style_var_memo Memo      = *g.StyleVarStack.Pop();
  g.Style.Vars[Memo.Index] = Memo.Value;
}

void
UI::PushStyleColor(int32_t Index, vec4 Color)
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
UI::PopStyleColor()
{
  gui_context& g = *GetContext();

  assert(0 < g.StyleColorStack.Count);
  style_color_memo Memo      = *g.StyleColorStack.Pop();
  g.Style.Colors[Memo.Index] = Memo.Color;
}

void
UI::DragFloat3(const char* Label, float Value[3], float MinValue, float MaxValue, float ScreenDelta)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  vec3 Size = Window.GetDefaultItemSize();
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

  PushStyleVar(UI::VAR_SpacingX, 0);
  UI::DragFloat(NULL, &Value[0], MinValue, MaxValue, ScreenDelta, SingleSliderWidth);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[1], MinValue, MaxValue, ScreenDelta, SingleSliderWidth);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[2], MinValue, MaxValue, ScreenDelta, SingleSliderWidth);
  UI::SameLine();
  UI::NewLine();
  PopStyleVar();

  DrawText(TextPosition, Label);
}

void
UI::DragFloat4(const char* Label, float Value[4], float MinValue, float MaxValue, float ScreenDelta)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  vec3 Size = Window.GetDefaultItemSize();
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

  PushStyleVar(UI::VAR_SpacingX, 0);
  UI::DragFloat(NULL, &Value[0], MinValue, MaxValue, ScreenDelta, SingleSliderWidth);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[1], MinValue, MaxValue, ScreenDelta, SingleSliderWidth);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[2], MinValue, MaxValue, ScreenDelta, SingleSliderWidth);
  UI::SameLine();
  UI::DragFloat(NULL, &Value[3], MinValue, MaxValue, ScreenDelta, SingleSliderWidth);
  UI::SameLine();
  UI::NewLine();
  PopStyleVar();

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
