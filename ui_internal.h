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
void     UnsetHot(ui_id ID);

bool IsMouseInsideRect(const vec3& MinP, const vec3& MaxP);
bool IsMouseInsideRect(const rect& BB);
bool IsWindowHoverable(const gui_window* Window);
bool IsHovered(const rect& BB, ui_id ID);
void FocusWindow(gui_window* Window);

void AddSize(const vec3& Size);
bool TestIfVisible(const rect& Rect);

void         Create(gui_context* Context);
int          Destroy(gui_context* Context);
gui_context* GetContext();
gui_window*  GetCurrentWindow();

void DrawText(vec3 TopLeft, float Width, float Height, const char* Text);
void DrawText(vec3 Position, const char* Text);
void DrawBox(vec3 TopLeft, float Width, float Height, vec4 InnerColor, vec4 BorderColor);

// TODO(Lukas) Fix quad submission api, reduce levels of abstraction up to shader minimize getters
// these currently store quads with faulty quad data, which is currently resubmitted to other API
void PushClipQuad(gui_window* Window, const vec3& TopLeft, const vec3& Size, bool IntersectWithPrevious = true);
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
    return (MinP.X < Point.X && Point.X <= MaxP.X && MinP.Y < Point.Y && Point.Y <= MaxP.Y) ? true : false;
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

  vec3 Position;
  vec3 Size;
  vec3 SizeNoScroll; // Entire window except scrollbars

  vec3 CurrentPos;
  vec3 MaxPos;

  rect ClippedSizeRect; // Used for hovered window calculations

  vec3 ContentsSize;

  vec3 ScrollNorm;
  vec3 ScrollRange;

  fixed_array<quad_instance, 80> DrawArray;

  ui_id
  GetID(const char* Label) const
  {
    return IDHash(Label, (int)strlen(Label), this->ID);
  }
};

struct gui_context
{
  game_state*   GameState;
  int           InitChecksum;
  UI::gui_style Style;
  Text::font*   Font;
  ui_id         ActiveID;
  ui_id         HotID;
  ui_id         MoveWindowMoveID;

  int32_t                      LatestClipRectIndex;
  fixed_stack<rect, 20>        ClipRectStack;
  fixed_array<gui_window, 10>  Windows;
  fixed_stack<gui_window*, 10> CurrentWindowStack;
  fixed_stack<gui_window*, 10> OrderedWindows;

  const game_input* Input;
  gui_window*       CurrentWindow;
  gui_window*       HoveredWindow;
  gui_window*       ActiveIDWindow;
  gui_window*       FocusedWindow;
};

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

  int Index = -1;
  for(int i = 0; i < g.OrderedWindows.Count; i++)
  {
    if(g.OrderedWindows[i] == Window)
    {
      Index = i;
      break;
    }
  }

  if(Window)
  {
    assert(0 <= Index);
    g.OrderedWindows.Delete(Index);
    g.OrderedWindows.Push(Window);
  }
}

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

void
DrawText(vec3 BottomLeft, const char* Text)
{
  gui_context& g      = *GetContext();
  gui_window*  Window = GetCurrentWindow();

  int32_t  TextureWidth;
  int32_t  TextureHeight;
  uint32_t TextureID = Text::GetTextTextureID(g.Font, (int32_t)g.Style.StyleVars[UI::VAR_FontSize].X, Text, _GetGUIColor(Text), &TextureWidth, &TextureHeight);
  PushTexturedQuad(Window, { BottomLeft.X, BottomLeft.Y - (float)TextureHeight }, { (float)TextureWidth, (float)TextureHeight }, TextureID);
}

void
DrawBox(vec3 TopLeft, float Width, float Height, vec4 InnerColor, vec4 BorderColor)
{
  gui_context& g            = *GetContext();
  gui_window*  Window       = GetCurrentWindow();
  float        ButtonBorder = g.Style.StyleVars[UI::VAR_BorderWidth].X;
  PushColoredQuad(Window, TopLeft, { Width, Height }, BorderColor);
  PushColoredQuad(Window, vec3{ TopLeft.X + ButtonBorder, TopLeft.Y + ButtonBorder, TopLeft.Z }, { Width - 2 * ButtonBorder, Height - 2 * ButtonBorder }, InnerColor);
}

void
DrawBox(vec3 TopLeft, vec3 Size, vec4 InnerColor, vec4 BorderColor)
{
  DrawBox(TopLeft, Size.X, Size.Y, InnerColor, BorderColor);
}

void
DrawTextBox(vec3 TopLeft, float Width, float Height, const char* Text, vec4 InnerColor, vec4 BorderColor)
{
  DrawBox(TopLeft, Width, Height, InnerColor, BorderColor);
  DrawText(TopLeft, Width, Height, Text);
}

void
DrawTextBox(vec3 TopLeft, vec3 Size, const char* Text, vec4 InnerColor, vec4 BorderColor)
{
  DrawTextBox(TopLeft, Size.X, Size.Y, Text, InnerColor, BorderColor);
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
      // We support a syntax of "label###id" where only "###id" is included in the hash, and only "label" gets displayed.
      // Because this syntax is rarely used we are optimizing for the common case.
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

void
UnsetHot(ui_id ID)
{
  gui_context& g = *GetContext();
  if(g.ActiveID == 0 || g.ActiveID == ID)
  {
    g.HotID = 0;
  }
}

bool
IsWindowHoverable(const gui_window* Window)
{
  gui_context& g             = *GetContext();
  gui_window*  FocusedWindow = g.FocusedWindow;
  if(FocusedWindow)
  {
    gui_window* FocusedRootWindow = FocusedWindow->RootWindow;
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
  return (MinP.X < Point.X && Point.X <= MaxP.X && MinP.Y < Point.Y && Point.Y <= MaxP.Y) ? true : false;
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
  gui_window& Window = *GetCurrentWindow();
  Window.MaxPos.X    = MaxFloat(Window.MaxPos.X, Window.CurrentPos.X + Size.X);
  Window.MaxPos.Y    = MaxFloat(Window.MaxPos.Y, Window.CurrentPos.Y + Size.Y);

  Window.CurrentPos.Y += Size.Y;
}

bool
TestIfVisible(const rect& Rect)
{
  // gui_window& Window   = *GetCurrentWindow();
  gui_context& g        = *GetContext();
  rect         ClipRect = g.ClipRectStack.Back();
  return Rect.Intersects(ClipRect) ? true : false;
}

void
Create(gui_context* Context, game_state* GameState)
{
  Context->Style.Colors[UI::COLOR_Border]           = { 0.1f, 0.1f, 0.1f, 0.5f };
  Context->Style.Colors[UI::COLOR_ButtonNormal]     = { 0.4f, 0.4f, 0.4f, 1 };
  Context->Style.Colors[UI::COLOR_ButtonHovered]    = { 0.5f, 0.5f, 0.5f, 1 };
  Context->Style.Colors[UI::COLOR_ButtonPressed]    = { 0.3f, 0.3f, 0.3f, 1 };
  Context->Style.Colors[UI::COLOR_HeaderNormal]     = { 0.2f, 0.4f, 0.4f, 1 };
  Context->Style.Colors[UI::COLOR_HeaderHover]      = { 0.3f, 0.5f, 0.5f, 1 };
  Context->Style.Colors[UI::COLOR_HeaderPressed]    = { 0.1f, 0.3f, 0.3f, 1 };
  Context->Style.Colors[UI::COLOR_CheckboxNormal]   = { 0.3f, 0.3f, 0.3f, 1 };
  Context->Style.Colors[UI::COLOR_CheckboxPressed]  = { 0.2f, 0.2f, 0.4f, 1 };
  Context->Style.Colors[UI::COLOR_CheckboxHover]    = { 0.3f, 0.3f, 0.5f, 1 };
  Context->Style.Colors[UI::COLOR_ScrollbarBox]     = { 0.3f, 0.3f, 0.5f, 0.5f };
  Context->Style.Colors[UI::COLOR_ScrollbarDrag]    = { 0.2f, 0.2f, 0.4f, 0.5f };
  Context->Style.Colors[UI::COLOR_WindowBackground] = { 0.5f, 0.1f, 0.1f, 0.5f };
  Context->Style.Colors[UI::COLOR_WindowBorder]     = { 0.4f, 0.4f, 0.4f, 0.5f };
  Context->Style.Colors[UI::COLOR_Text]             = { 1.0f, 1.0f, 1.0f, 1 };

  Context->Style.StyleVars[UI::VAR_BorderWidth]   = { 1 };
  Context->Style.StyleVars[UI::VAR_ScrollbarSize] = { 20 };
  Context->Style.StyleVars[UI::VAR_DragMinSize]   = { 10 };
  Context->Style.StyleVars[UI::VAR_FontSize]      = { (float)GameState->Font.SizedFonts[0].Size };

  Context->InitChecksum = CONTEXT_CHECKSUM;
  Context->Font         = &GameState->Font;
  Context->Windows.HardClear();
  Context->CurrentWindow = NULL;
  Context->GameState     = GameState;
}

int
Destroy(gui_context* Context)
{
  return 0;
}
