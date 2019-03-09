#include <float.h>
#include <string.h>

#include "debug_drawing.h"
#include "basic_data_structures.h"

#define CONTEXT_CHECKSUM 12345
#define _GetGUIColor(Index) (g.Style.Colors[UI::COLOR_##Index])

//-----------------------------
// Forward declarations
//-----------------------------
typedef uint32_t ui_id;
struct rect;
struct gui_window;
struct gui_context;

//-----------------------------
// Internal API
//-----------------------------
uint32_t IDHash(const void* data, int data_size, uint32_t seed);
void     SetActive(ui_id ID);
void     SetHot(ui_id ID);
void     FocusWindow(gui_window* Window);

bool IsMouseInsideRect(const vec3& MinP, const vec3& MaxP);
bool IsMouseInsideRect(const rect& BB);
bool IsWindowHoverable(const gui_window* Window);
bool IsHovered(const rect& BB, ui_id ID);

void AddSize(const vec3& Size);
bool TestIfVisible(const rect& Rect);

bool IsPopupOpen();
void ClosePopup(ui_id ID);
void CloseInactivePopups();

void         Init(gui_context* Context, game_state* GameState);
int          Destroy(gui_context* Context);
gui_context* GetContext();
gui_window*  GetCurrentWindow();

void DrawText(const vec3& BottomLeft, const char* Text);
void DrawBox(vec3 TopLeft, float Width, float Height, vec4 InnerColor, vec4 BorderColor);
void DrawBox(vec3 TopLeft, vec3 Size, vec4 InnerColor, vec4 BorderColor);

// TODO(Lukas) Fix quad submission api, reduce levels of abstraction up to shader minimize getters
// these currently store quads with faulty quad data, which is currently resubmitted to other API
void PushClipQuad(gui_window* Window, const vec3& TopLeft, const vec3& Size,
                  bool IntersectWithPrevious = true);
void PushColoredQuad(gui_window* Window, const vec3& TopLeft, const vec3& Size, const vec4& Color);
void PushTexturedQuad(gui_window* Window, const vec3& TopLeft, const vec3& Size, int32_t TextureID);
//-----------------------------

struct rect
{
  vec3 MinP;
  vec3 MaxP;

  vec3
  GetSize() const
  {
    return MaxP - MinP;
  }

  float
  GetWidth() const
  {
    return MaxP.X - MinP.X;
  }

  float
  GetHeight() const
  {
    return MaxP.Y - MinP.Y;
  }

  bool
  Encloses(vec3 Point) const
  {
    return (MinP.X < Point.X && Point.X <= MaxP.X && MinP.Y < Point.Y && Point.Y <= MaxP.Y) ? true
                                                                                            : false;
  }

  bool
  Intersects(const rect& B) const
  {
    const rect& A = *this;
    if(A.MaxP.X < B.MinP.X || B.MaxP.X < A.MinP.X || A.MaxP.Y < B.MinP.Y || B.MaxP.Y < A.MinP.Y)
    {
      return false;
    }
    return true;
  }

  void
  Clip(const rect ClipRect)
  {
    if(MinP.X < ClipRect.MinP.X)
      MinP.X = ClipRect.MinP.X;
    if(MinP.Y < ClipRect.MinP.Y)
      MinP.Y = ClipRect.MinP.Y;
    if(MaxP.X > ClipRect.MaxP.X)
      MaxP.X = ClipRect.MaxP.X;
    if(MaxP.Y > ClipRect.MaxP.Y)
      MaxP.Y = ClipRect.MaxP.Y;
  }

  rect
  GetIntersection(const rect& BB) const
  {
    rect Result = *this;
    Result.Clip(BB);
    return Result;
  }
};

rect
NewRect(vec3 MinP, vec3 MaxP)
{
  rect NewRect;
  NewRect.MinP = MinP;
  NewRect.MaxP = MaxP;
  return NewRect;
}

rect
NewRect(float MinX, float MinY, float MaxX, float MaxY)
{
  rect NewRect;
  NewRect.MinP = { MinX, MinY };
  NewRect.MaxP = { MaxX, MaxY };
  return NewRect;
}

struct style_var_memo
{
  int32_t Index;
  float   Value;
};

struct style_color_memo
{
  vec4    Color;
  int32_t Index;
};

struct gui_window
{
  path               Name;
  ui_id              ID;
  ui_id              MoveID;
  UI::window_flags_t Flags;

  gui_window*                  RootWindow;
  gui_window*                  ParentWindow;
  fixed_array<gui_window*, 10> ChildWindows;
  int32_t                      IndexWithinParent;

  bool UsedLastFrame;
  bool UsedThisFrame;

  vec3 Position;
  vec3 StartPos;
  vec3 Size;
  vec3 SizeNoScroll; // Entire window except scrollbars

  vec3 CurrentPos;
  vec3 PreviousPos;
  vec3 MaxPos;

  rect  ClippedSizeRect; // Used for hovered window calculations
  float DefaultItemWidth;

  vec3 ContentsSize;

  vec3 ScrollNorm;
  vec3 ScrollRange;

  fixed_array<quad_instance, 300> DrawArray;

  ui_id
  GetID(const char* Label) const
  {
    return IDHash(Label, (int)strlen(Label), this->ID);
  }
  ui_id
  GetID(const uintptr_t Pointer) const
  {
    return IDHash(&Pointer, (int)sizeof(uintptr_t*), this->ID);
  }
  vec3 GetDefaultItemSize() const;
};

struct gui_popup
{
  gui_window* Window;
  ui_id       ID;
};

struct gui_context
{
  game_state*   GameState;
  int           InitChecksum;
  UI::gui_style Style;
  Text::font*   Font;
  ui_id         ActiveID;
  ui_id         HotID;

  int32_t                           LatestClipRectIndex;
  fixed_stack<rect, 20>             ClipRectStack;
  fixed_array<gui_window, 10>       Windows;
  fixed_stack<gui_window*, 10>      CurrentWindowStack;
  fixed_stack<gui_window*, 10>      OrderedWindows;
  fixed_stack<gui_popup, 10>        CurrentPopupStack;
  fixed_stack<gui_popup, 10>        OpenPopupStack;
  fixed_stack<style_var_memo, 10>   StyleVarStack;
  fixed_stack<style_color_memo, 10> StyleColorStack;

  const game_input* Input;
  gui_window*       CurrentWindow;
  gui_window*       HoveredWindow;
  gui_window*       ActiveIDWindow;
  gui_window*       FocusedWindow;
};

vec3
gui_window::GetDefaultItemSize() const
{
  gui_context& g      = *GetContext();
  vec3         Result = { this->DefaultItemWidth,
                  g.Style.Vars[UI::VAR_FontSize] + 2 * g.Style.Vars[UI::VAR_BoxPaddingY] };
  return Result;
}

// GLOBAL CONTEXT
static gui_context g_Context;

void
PushClipQuad(gui_window* Window, const vec3& Position, const vec3& Size, bool IntersectWithPrevious)
{
  gui_context& g        = *GetContext();
  rect         ClipRect = NewRect(Position, Position + Size);
  if(0 < g.ClipRectStack.Count && IntersectWithPrevious)
  {
    ClipRect.Clip(g.ClipRectStack.Back());
  }
  g.ClipRectStack.Push(ClipRect);

  quad_instance Quad = {};
  Quad.Type          = QuadType_Clip;
  Quad.LowerLeft     = ClipRect.MinP;
  Quad.Dimensions    = ClipRect.GetSize();
  Quad.Color         = { 1, 0, 1, (float)g.LatestClipRectIndex++ };
  Window->DrawArray.Append(Quad);
}

void
PushColoredQuad(gui_window* Window, const vec3& Position, const vec3& Size, const vec4& Color)
{
  quad_instance Quad = {};
  Quad.Type          = QuadType_Colored;
  Quad.LowerLeft     = Position;
  Quad.Dimensions    = Size;
  Quad.Color         = Color;
  Window->DrawArray.Append(Quad);
}

void
PushTexturedQuad(gui_window* Window, const vec3& Position, const vec3& Size, int32_t TextureID)
{
  quad_instance Quad = {};
  Quad.Type          = QuadType_Textured;
  Quad.LowerLeft     = Position;
  Quad.Dimensions    = Size;
  Quad.TextureID     = TextureID;
  Window->DrawArray.Append(Quad);
}

void
FocusWindow(gui_window* Window)
{
  gui_context& g  = *GetContext();
  g.FocusedWindow = Window;

  if(Window)
  {
    gui_window* RootWindow = Window->RootWindow;
    assert(RootWindow);

    int Index = -1;
    for(int i = 0; i < g.OrderedWindows.Count; i++)
    {
      if(g.OrderedWindows[i] == RootWindow)
      {
        Index = i;
        break;
      }
    }

    assert(0 <= Index);
    g.OrderedWindows.Delete(Index);
    g.OrderedWindows.Push(RootWindow);
  }
}

#if 0
void
DrawText(vec3 TopLeft, float Width, float Height, const char* InputText)
{
  const gui_context& g      = *GetContext();
  gui_window*        Window = GetCurrentWindow();

  float   TextPaddingX = 0.001f;
  float   TextPaddingY = 0.005f;
  int32_t TextureWidth;
  int32_t TextureHeight;
  char    Text[100];

  int32_t FontSize = (int32_t)((Height + 2.0f * TextPaddingY) / 3.0f);

  float LengthDiff = Width - ((float)(strlen(InputText) * g.Font->AverageSymbolWidth)) - 2 * TextPaddingX;
  if(LengthDiff < 0)
  {
    float   SymbolWidth = (float)g.Font->AverageSymbolWidth;
    int32_t Count       = 0;
    while(LengthDiff < 0)
    {
      LengthDiff += SymbolWidth;
      ++Count;
    }
    Count += 3;
    int32_t NewLength = (int32_t)strlen(InputText) - Count;
    if(NewLength > 0)
    {
      strncpy(Text, InputText, NewLength);
      strcpy(&Text[NewLength], "...\0");
    }
    else
    {
      strcpy(Text, "...\0");
    }
  }
  else
  {
    strcpy(Text, InputText);
  }
  uint32_t TextureID = Text::GetTextTextureID(g.Font, FontSize, Text, _GetGUIColor(Text), &TextureWidth, &TextureHeight);
  PushTexturedQuad(Window, vec3{ TopLeft.X + ((Width - (float)TextureWidth) / 2), TopLeft.Y - TextPaddingY, TopLeft.Z }, { (float)TextureWidth, Height - 2 * TextPaddingY }, TextureID);
}
#endif

void
DrawText(const vec3& BottomLeft, const char* Text)
{
  gui_context& g      = *GetContext();
  gui_window*  Window = GetCurrentWindow();

  int32_t  TextureWidth;
  int32_t  TextureHeight;
  uint32_t TextureID = Text::GetTextTextureID(g.Font, (int32_t)g.Style.Vars[UI::VAR_FontSize], Text,
                                              _GetGUIColor(Text), &TextureWidth, &TextureHeight);
  PushTexturedQuad(Window, { BottomLeft.X, BottomLeft.Y - (float)TextureHeight },
                   { (float)TextureWidth, (float)TextureHeight }, TextureID);
}

void
DrawBox(vec3 TopLeft, float Width, float Height, vec4 InnerColor, vec4 BorderColor)
{
  gui_context& g      = *GetContext();
  gui_window*  Window = GetCurrentWindow();
  float        Border = g.Style.Vars[UI::VAR_BorderThickness];
  // PushColoredQuad(Window, TopLeft, { Width, Height }, BorderColor);
  // PushColoredQuad(Window, vec3{ TopLeft.X + Border, TopLeft.Y + Border, TopLeft.Z }, { Width - 2
  // * Border, Height - 2 * Border }, InnerColor);
  PushColoredQuad(Window, TopLeft, { Width, Height }, InnerColor);
}

void
DrawBox(vec3 TopLeft, vec3 Size, vec4 InnerColor, vec4 BorderColor)
{
  DrawBox(TopLeft, Size.X, Size.Y, InnerColor, BorderColor);
}

uint32_t
IDHash(const void* data, int data_size, uint32_t seed)
{
  static uint32_t crc32_lut[256] = { 0 };
  if(!crc32_lut[1])
  {
    const uint32_t polynomial = 0xEDB88320;
    for(int32_t i = 0; i < 256; i++)
    {
      uint32_t crc = i;
      for(uint32_t j = 0; j < 8; j++)
        crc = (crc >> 1) ^ (int32_t(-int(crc & 1)) & polynomial);
      crc32_lut[i] = crc;
    }
  }

  seed                         = ~seed;
  uint32_t             crc     = seed;
  const unsigned char* current = (const unsigned char*)data;

  if(data_size > 0)
  {
    // Known size
    while(data_size--)
      crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *current++];
  }
  else
  {
    // Zero-terminated string
    while(unsigned char c = *current++)
    {
      // We support a syntax of "label###id" where only "###id" is included in the hash, and only
      // "label" gets displayed. Because this syntax is rarely used we are optimizing for the common
      // case.
      // - If we reach ### in the string we discard the hash so far and reset to the seed.
      // - We don't do 'current += 2; continue;' after handling ### to keep the code smaller.
      if(c == '#' && current[0] == '#' && current[1] == '#')
        crc = seed;
      crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
    }
  }
  return ~crc;
}

void
SetActive(ui_id ID, gui_window* Window)
{
  gui_context& g   = *GetContext();
  g.ActiveID       = ID;
  g.ActiveIDWindow = Window;
}

void
SetHot(ui_id ID)
{
  gui_context& g = *GetContext();
  if(g.ActiveID == 0 || g.ActiveID == ID)
  {
    g.HotID = ID;
  }
}

bool
IsWindowHoverable(const gui_window* Window)
{
  gui_context& g = *GetContext();
  if(g.FocusedWindow)
  {
    gui_window* FocusedRootWindow = g.FocusedWindow->RootWindow;
    if(FocusedRootWindow)
    {
      if((FocusedRootWindow->Flags & UI::WINDOW_Popup) && FocusedRootWindow != Window->RootWindow)
      {
        return false;
      }
    }
  }
  return true;
}

bool
IsHovered(const rect& BB, ui_id ID)
{
  gui_context& g = *GetContext();
  if(g.HotID == 0 || g.HotID == ID)
  {
    gui_window* Window = GetCurrentWindow();
    if(Window == g.HoveredWindow && IsWindowHoverable(Window))
    {
      rect TestRect = BB.GetIntersection(g.ClipRectStack.Back());
      if((g.ActiveID == 0 || g.ActiveID == ID) && IsMouseInsideRect(TestRect))
      {
        return true;
      }
    }
  }
  return false;
}

bool
IsMouseInsideRect(const vec3& MinP, const vec3& MaxP)
{
  gui_context& g     = *GetContext();
  vec3         Point = { (float)g.Input->MouseScreenX, (float)g.Input->MouseScreenY };
  return (MinP.X < Point.X && Point.X <= MaxP.X && MinP.Y < Point.Y && Point.Y <= MaxP.Y);
}

bool
IsMouseInsideRect(const rect& BB)
{
  return IsMouseInsideRect(BB.MinP, BB.MaxP);
}

gui_context*
GetContext()
{
  assert(g_Context.InitChecksum == CONTEXT_CHECKSUM);
  return &g_Context;
}

gui_window*
GetCurrentWindow()
{
  gui_context& G = *GetContext();
  assert(G.CurrentWindow);
  return G.CurrentWindow;
}

void
AddSize(const vec3& Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  Window.MaxPos.X =
    MaxFloat(Window.MaxPos.X, Window.CurrentPos.X + Size.X /* + g.Style.Vars[UI::VAR_SpacingX]*/);
  Window.MaxPos.Y =
    MaxFloat(Window.MaxPos.Y, Window.CurrentPos.Y + Size.Y /*+ g.Style.Vars[UI::VAR_SpacingY]*/);

  Window.PreviousPos = Window.CurrentPos + vec3{ Size.X + g.Style.Vars[UI::VAR_SpacingX], 0 };
  Window.CurrentPos.Y += Size.Y + g.Style.Vars[UI::VAR_SpacingY];
}

bool
TestIfVisible(const rect& Rect)
{
  // gui_window& Window   = *GetCurrentWindow();
  gui_context& g        = *GetContext();
  rect         ClipRect = g.ClipRectStack.Back();
  return Rect.Intersects(ClipRect);
}

vec3
GetItemSize(vec3 Size, float DefaultWidth, float DefaultHeight)
{
  if(Size.X == 0)
  {
    Size.X = DefaultWidth;
  }
  if(Size.Y == 0)
  {
    Size.Y = DefaultHeight;
  }
  return Size;
}

bool
IsPopupOpen(ui_id ID)
{
  gui_context& g = *GetContext();
  return (g.CurrentPopupStack.Count < g.OpenPopupStack.Count &&
          g.OpenPopupStack[g.CurrentPopupStack.Count].ID == ID);
}

void
ClosePopup(ui_id ID)
{
  gui_context& g = *GetContext();
  assert(IsPopupOpen(ID));
  g.OpenPopupStack.Pop();
}

void
OpenPopup(ui_id ID)
{
  gui_context& g = *GetContext();

  gui_popup NewPopup = {};
  NewPopup.ID        = ID;
  NewPopup.Window    = NULL;

  if(g.OpenPopupStack.Count <= g.CurrentPopupStack.Count)
  {
    g.OpenPopupStack.Push(NewPopup);
  }
  else if(g.OpenPopupStack[g.CurrentPopupStack.Count].ID != ID)
  {
    int CurrentPopupStackSize = g.CurrentPopupStack.Count;
    g.OpenPopupStack.Resize(CurrentPopupStackSize + 1);
    g.OpenPopupStack[CurrentPopupStackSize] = NewPopup;
  }
}

void
CloseInactivePopups()
{
  gui_context& g = *GetContext();
  if(g.OpenPopupStack.Count == 0)
  {
    return;
  }

  int n = 0;
  if(g.FocusedWindow)
  {
    for(n = 0; n < g.OpenPopupStack.Count; n++)
    {
      gui_popup& Popup = g.OpenPopupStack[n];
      assert(Popup.Window);
      assert(Popup.Window->Flags & UI::WINDOW_Popup);
      if(Popup.Window->Flags & UI::WINDOW_IsChildWindow)
      {
        continue;
      }

      bool HasFocus = false;
      for(int m = n; m < g.OpenPopupStack.Count && !HasFocus; m++)
      {
        HasFocus =
          (g.OpenPopupStack[m].Window && g.OpenPopupStack[m].Window->RootWindow == g.FocusedWindow);
      }
      if(!HasFocus)
      {
        break;
      }
    }
  }
  g.OpenPopupStack.Resize(n);
}

void
Init(gui_context* Context, game_state* GameState)
{
  Context->Style.Colors[UI::COLOR_Border]           = { 0.1f, 0.1f, 0.1f, 0.5f };
  Context->Style.Colors[UI::COLOR_ButtonNormal]     = { 0.4f, 0.4f, 0.4f, 1 };
  Context->Style.Colors[UI::COLOR_ButtonHovered]    = { 0.5f, 0.5f, 0.5f, 1 };
  Context->Style.Colors[UI::COLOR_ButtonPressed]    = { 0.3f, 0.3f, 0.3f, 1 };
  Context->Style.Colors[UI::COLOR_HeaderNormal]     = { 0.2f, 0.35f, 0.55f, 1 };
  Context->Style.Colors[UI::COLOR_HeaderHovered]    = { 0.3f, 0.5f, 0.7f, 1 };
  Context->Style.Colors[UI::COLOR_HeaderPressed]    = { 0.1f, 0.3f, 0.5f, 1 };
  Context->Style.Colors[UI::COLOR_CheckboxNormal]   = { 0.4f, 0.4f, 0.4f, 1 };
  Context->Style.Colors[UI::COLOR_CheckboxHovered]  = { 0.5f, 0.5f, 0.5f, 1 };
  Context->Style.Colors[UI::COLOR_CheckboxPressed]  = { 0.7f, 0.7f, 0.7f, 1 };
  Context->Style.Colors[UI::COLOR_ScrollbarBox]     = { 0.3f, 0.3f, 0.5f, 0.5f };
  Context->Style.Colors[UI::COLOR_ScrollbarDrag]    = { 0.2f, 0.2f, 0.4f, 0.5f };
  Context->Style.Colors[UI::COLOR_WindowBackground] = { 0.2f, 0.25f, 0.25f, 0.85f };
  Context->Style.Colors[UI::COLOR_WindowBorder]     = { 0.4f, 0.4f, 0.4f, 0.5f };
  Context->Style.Colors[UI::COLOR_Text]             = { 1.0f, 1.0f, 1.0f, 1 };

  Context->Style.Vars[UI::VAR_FontSize]        = (float)GameState->Font.SizedFonts[0].Size;
  Context->Style.Vars[UI::VAR_BorderThickness] = 1;
  Context->Style.Vars[UI::VAR_ScrollbarSize]   = 15;
  Context->Style.Vars[UI::VAR_DragMinSize]     = 10;
  Context->Style.Vars[UI::VAR_BoxPaddingX]     = 5;
  Context->Style.Vars[UI::VAR_BoxPaddingY]     = 0;
  Context->Style.Vars[UI::VAR_SpacingX]        = 5;
  Context->Style.Vars[UI::VAR_SpacingY]        = 5;
  Context->Style.Vars[UI::VAR_InternalSpacing] = 5;

  Context->InitChecksum = CONTEXT_CHECKSUM;
  Context->Font         = &GameState->Font;
  Context->Windows.HardClear();
  Context->CurrentWindowStack.Clear();
  Context->CurrentPopupStack.Clear();
  Context->OpenPopupStack.Clear();
  Context->StyleVarStack.Clear();
  Context->StyleColorStack.Clear();

  Context->CurrentWindow = NULL;
  Context->GameState     = GameState;
}
