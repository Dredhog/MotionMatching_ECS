#include <float.h>
#include <string.h>

#include "debug_drawing.h"
#include "basic_data_structures.h"

#define CONTEXT_CHECKSUM 12345

// Forward declarations
struct ui_id;
struct rect;
struct gui_window;
struct gui_context;

// Internal API
uint32_t IDHash(const void* data, int data_size, uint32_t seed);
bool     AreUI_IDsEqual(ui_id A, ui_id B);
bool     IsActive(ui_id ID);
void     SetActive(ui_id ID);
bool     IsHot(ui_id ID);
void     SetHot(ui_id ID);
void     UnsetHot(ui_id ID);

void PushSize(const vec3& Size);
bool PushForClipping(const rect& Rect);

void         Create(gui_context* Context);
int          Destroy(gui_context* Context);
gui_context* GetContext();
gui_window*  GetCurrentWindow();

void DrawText(vec3 TopLeft, float Width, float Height, const char* InputText);
void DrawBox(vec3 TopLeft, float Width, float Height, vec4 InnerColor, vec4 BorderColor);
//-------------

struct ui_id
{
  uint32_t Value;

  bool
  operator==(ui_id Other)
  {
    return this->Value == Other.Value;
  }

  bool
  operator!=(ui_id Other)
  {
    return this->Value != Other.Value;
  }
};

ui_id NOT_ACTIVE = {};

struct rect
{
  vec3 MinP;
  vec3 MaxP;

  vec3
  GetSize() const
  {
    return MaxP - MinP;
  }

  bool
  Encloses(vec3 Point) const
  {
    return (MinP.X <= Point.X && Point.X <= MaxP.X && MinP.Y <= Point.Y && Point.Y <= MaxP.Y) ? true : false;
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
  path  Name;
  ui_id ID;

  gui_window* RootWindow;
  gui_window* ParentWindow;

  vec3 StartPos;
  vec3 Size;

  vec3  CurrentPos;
  float CurrentItemWidth;
  float CurrentItemHeight;

  vec3 ContentsSize;
  rect ClipRect;

  ui_id
  GetID(const char* Label)
  {
    return { IDHash(Label, (int)strlen(Label), this->ID.Value) };
  }
};

struct gui_context
{
  int           InitChecksum;
  UI::gui_style Style;
  Text::font*   Font;
  ui_id         ActiveID;
  ui_id         HotID;

  fixed_array<gui_window, 10> Windows;

  const game_input* Input;
  gui_window*       CurrentWindow;
};

// GLOBAL CONTEXT
static gui_context g_Context;

void
DrawText(vec3 TopLeft, float Width, float Height, const char* InputText)
{
  const gui_context& G = *GetContext();

  float   TextPaddingX = 0.001f;
  float   TextPaddingY = 0.005f;
  int32_t TextureWidth;
  int32_t TextureHeight;
  char    Text[100];

  int32_t FontSize = (int32_t)((Height + 2.0f * TextPaddingY) * SCREEN_HEIGHT / 3.0f);

  float LengthDiff = Width - ((float)(strlen(InputText) * G.Font->AverageSymbolWidth) / SCREEN_WIDTH) - 2 * TextPaddingX;
  if(LengthDiff < 0)
  {
    float   SymbolWidth = (float)G.Font->AverageSymbolWidth / SCREEN_WIDTH;
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
  uint32_t TextureID = Text::GetTextTextureID(G.Font, FontSize, Text, G.Style.Colors[UI::GUI_COLOR_Text], &TextureWidth, &TextureHeight);
  Debug::PushTopLeftTexturedQuad(TextureID, vec3{ TopLeft.X + ((Width - ((float)TextureWidth / SCREEN_WIDTH)) / 2.0f), TopLeft.Y - TextPaddingY, TopLeft.Z }, (float)TextureWidth / SCREEN_WIDTH,
                                 Height - 2 * TextPaddingY);
}

void
DrawBox(vec3 TopLeft, float Width, float Height, vec4 InnerColor, vec4 BorderColor)
{
  float ButtonBorder = 0.002f;
  Debug::UIPushQuad(TopLeft, { Width, Height }, BorderColor);
  Debug::UIPushQuad(vec3{ TopLeft.X + ButtonBorder, TopLeft.Y - ButtonBorder, TopLeft.Z }, { Width - 2 * ButtonBorder, Height - 2 * ButtonBorder }, InnerColor);
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
  DrawBox(TopLeft, Size.X, Size.Y, InnerColor, BorderColor);
  DrawText(TopLeft, Size.X, Size.Y, Text);
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

bool
IsHot(ui_id ID)
{
  return ID == GetContext()->HotID;
}

bool
IsActive(ui_id ID)
{
  return ID == GetContext()->ActiveID;
}

void
SetActive(ui_id ID)
{
  GetContext()->ActiveID = ID;
}

void
SetHot(ui_id ID)
{
  if(IsActive(NOT_ACTIVE) || IsActive(ID))
  {
    GetContext()->HotID = ID;
  }
}

void
UnsetHot(ui_id ID)
{
  if(IsActive(NOT_ACTIVE) || IsActive(ID))
  {
    GetContext()->HotID = NOT_ACTIVE;
  }
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
PushSize(const vec3& Size)
{
  gui_window& Window = *GetCurrentWindow();
  Window.CurrentPos.Y += Size.Y;
}

bool
PushForClipping(const rect& Rect)
{
  gui_window& Window = *GetCurrentWindow();
  return Window.ClipRect.Intersects(Rect) ? true : false;
}

void
Create(gui_context* Context, game_state* GameState)
{
  Context->Style.Colors[UI::GUI_COLOR_Border]           = { 0.1f, 0.1f, 0.1f, 1 };
  Context->Style.Colors[UI::GUI_COLOR_ButtonNormal]     = { 0.4f, 0.4f, 0.4f, 1 };
  Context->Style.Colors[UI::GUI_COLOR_ButtonHover]      = { 0.5f, 0.5f, 0.5f, 1 };
  Context->Style.Colors[UI::GUI_COLOR_ButtonPressed]    = { 0.3f, 0.3f, 0.3f, 1 };
  Context->Style.Colors[UI::GUI_COLOR_CheckboxNormal]   = { 0.3f, 0.3f, 0.3f, 1 };
  Context->Style.Colors[UI::GUI_COLOR_CheckboxPressed]  = { 0.2f, 0.2f, 0.4f, 1 };
  Context->Style.Colors[UI::GUI_COLOR_CheckboxHover]    = { 0.3f, 0.3f, 0.5f, 1 };
  Context->Style.Colors[UI::GUI_COLOR_WindowBackground] = { 0.5f, 0.1f, 0.1f, 0.5f };
  Context->Style.Colors[UI::GUI_COLOR_WindowBorder]     = { 0.4f, 0.4f, 0.4f, 1 };
  Context->Style.Colors[UI::GUI_COLOR_Text]             = { 1.0f, 1.0f, 1.0f, 1 };

  Context->InitChecksum = CONTEXT_CHECKSUM;
  Context->Font         = &GameState->Font;
  Context->Windows.HardClear();
  Context->CurrentWindow = NULL;
}

int
Destroy(gui_context* Context)
{
  return 0;
}
