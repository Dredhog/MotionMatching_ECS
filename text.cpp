#include <GL/glew.h>

#include <assert.h>
#include "linear_math/vector.h"

#include "text.h"
#include "string.h"
#include "misc.h"

#define TEXTURE_CACHE_LINE_COUNT 256

struct text_texture
{
  char*             Text;
  Text::sized_font* SizedFont;
  int32_t           Dimensions[2];
  vec4              Color;
  uint32_t          TextureID;
};

struct text_line
{
  char Chars[TEXT_LINE_MAX_LENGTH];
};

text_line    g_TextLineCache[TEXTURE_CACHE_LINE_COUNT];
text_texture g_TextTextureCache[TEXTURE_CACHE_LINE_COUNT];
int32_t      g_CachedTextureCount;

int32_t g_HitCounts[TEXTURE_CACHE_LINE_COUNT];
int32_t g_RequestsPerFrame[TEXTURE_CACHE_LINE_COUNT];

Text::sized_font
Text::LoadSizedFont(const char* FontName, int FontSize)
{
  sized_font Result = {};
  TTF_Font*  Font   = TTF_OpenFont(FontName, FontSize);
  if(!Font)
  {
    printf("error: failed to load font: %s\n", SDL_GetError());
    return Result;
  }
  Result.Font = Font;
  Result.Size = FontSize;
  return Result;
}

Text::font
Text::LoadFont(const char* FontName, int MinSize, int SizeCount, int DeltaSize)
{
  assert(0 < SizeCount && SizeCount <= MAX_FONT_SIZE_COUNT);
  assert(0 < DeltaSize);
  assert(0 < MinSize);
  assert(FontName);

  int32_t NameLength = (int32_t)strlen(FontName);
  assert(NameLength < FONT_NAME_MAX_SIZE);

  font Result = {};
  strcpy(Result.Name, FontName);
  for(int i = 0; i < SizeCount; i++)
  {
    Result.SizedFonts[i] = LoadSizedFont(FontName, MinSize + i * DeltaSize);
  }
  Result.SizeCount = SizeCount;
  return Result;
}

uint32_t
LoadTextTexture(TTF_Font* Font, const char* Text, vec4 Color)
{
  SDL_Color FontColor;
  FontColor.a = (uint8_t)(255.0f * Color.A);
  FontColor.r = (uint8_t)(255.0f * Color.R);
  FontColor.g = (uint8_t)(255.0f * Color.G);
  FontColor.b = (uint8_t)(255.0f * Color.B);

  SDL_Surface* TextSurface = TTF_RenderUTF8_Blended(Font, Text, FontColor);
  if(TextSurface)
  {
    SDL_Surface* DestSurface = SDL_ConvertSurfaceFormat(TextSurface, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(TextSurface);

    uint32_t Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DestSurface->w, DestSurface->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, DestSurface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // free(DestSurface->pixels);
    SDL_FreeSurface(DestSurface);

    return Texture;
  }
  printf("error: text surface was not created!\nError: %s\n", SDL_GetError());
  assert(0 && "assert: failed to load text surface");
}

bool
IsDesiredTexture(const text_texture* Texture, const char* TargetText, Text::sized_font* SizedFont,
                 vec4 Color)
{
  if((SizedFont == Texture->SizedFont) && (Color == Texture->Color))
  {
    if(strcmp(TargetText, Texture->Text) == 0)
    {
      return true;
    }
  }
  return false;
}

void ReplaceTextTexture();

int32_t
SearchForDesiredTextureID(text_texture* Textures, Text::sized_font* SizedFont, const char* Text,
                          vec4 Color, int32_t TextureCount)
{
  for(int i = 0; i < TextureCount; i++)
  {
    if(IsDesiredTexture(&Textures[i], Text, SizedFont, Color))
    {
      return i;
    }
  }
  return -1;
}

char*
WriteTextToLineBufferAtIndex(text_line* LineBuffer, const char* Text, int32_t BufferLineCount)
{
  assert(0 <= BufferLineCount && BufferLineCount < TEXTURE_CACHE_LINE_COUNT);

  int32_t TextLength = (int32_t)strlen(Text) + 1; // strlen doesn't account for '\0'
  assert(TextLength <= TEXT_LINE_MAX_LENGTH);

  strcpy(LineBuffer[BufferLineCount].Chars, Text);
  return LineBuffer[BufferLineCount].Chars;
}

int32_t
FindCacheLineToOccupy(const int32_t* HitCount, int32_t CacheLineCount)
{
  int32_t CandidateLineIndex = -1;
  int32_t MinHitCount        = 1000000000;
  for(int i = 0; i < CacheLineCount; i++)
  {
    if(HitCount[i] < MinHitCount && g_RequestsPerFrame[i] == 0)
    {
      MinHitCount        = HitCount[i];
      CandidateLineIndex = i;
      if(HitCount == 0)
      {
        break;
      }
    }
  }
  assert(CandidateLineIndex >= 0);
  return CandidateLineIndex;
}

Text::sized_font
GetBestMatchingSizedFont(const Text::font* Font, int32_t FontSize)
{
  int MinDiff        = AbsInt32(Font->SizedFonts[0].Size - FontSize);
  int BestMatchIndex = 0;

  for(int i = 1; i < Font->SizeCount; i++)
  {
    int Diff = AbsInt32(Font->SizedFonts[i].Size - FontSize);
    if(Diff < MinDiff)
    {
      MinDiff        = Diff;
      BestMatchIndex = i;
    }
  }
  return Font->SizedFonts[BestMatchIndex];
}

uint32_t
Text::GetTextTextureID(font* Font, int32_t FontSize, const char* Text, vec4 Color, int32_t* Width,
                       int32_t* Height)
{
  sized_font SizedFont = GetBestMatchingSizedFont(Font, FontSize);
  int32_t    TextTextureIndex =
    SearchForDesiredTextureID(g_TextTextureCache, &SizedFont, Text, Color, g_CachedTextureCount);
  if(TextTextureIndex < 0)
  {
    int32_t NewIndex = FindCacheLineToOccupy(g_HitCounts, TEXTURE_CACHE_LINE_COUNT);

    if(g_TextTextureCache[NewIndex].TextureID)
    {
      // printf("evicting at: %d, texture: %d with %d hits\n", NewIndex,
      // g_TextTextureCache[NewIndex].TextureID, g_HitCounts[NewIndex]);
      glDeleteTextures(1, &g_TextTextureCache[NewIndex].TextureID);
      g_HitCounts[NewIndex] = 0;
    }

    g_TextTextureCache[NewIndex].TextureID = LoadTextTexture(SizedFont.Font, Text, Color);
    g_TextTextureCache[NewIndex].Text =
      WriteTextToLineBufferAtIndex(g_TextLineCache, Text, NewIndex);
    g_TextTextureCache[NewIndex].SizedFont = &SizedFont;
    g_TextTextureCache[NewIndex].Color     = Color;
    TTF_SizeText(SizedFont.Font, g_TextTextureCache[NewIndex].Text,
                 &g_TextTextureCache[NewIndex].Dimensions[0],
                 &g_TextTextureCache[NewIndex].Dimensions[1]);
    if(NewIndex == g_CachedTextureCount)
    {
      ++g_CachedTextureCount;
    }
    TextTextureIndex = NewIndex;
  }
  g_HitCounts[TextTextureIndex]++;
  g_RequestsPerFrame[TextTextureIndex]++;

  if(Width)
  {
    *Width = g_TextTextureCache[TextTextureIndex].Dimensions[0];
  }
  if(Height)
  {
    *Height = g_TextTextureCache[TextTextureIndex].Dimensions[1];
  }
  return g_TextTextureCache[TextTextureIndex].TextureID;
}
#if CACHE_TEST
void
ResetCache()
{
  for(int i = 0; i < g_CachedTextureCount; i++)
  {
    glDeleteTextures(1, &g_TextTextureCache[i].TextureID);
    g_TextLineCache[i]    = {};
    g_TextTextureCache[i] = {};

    g_HitCounts[i]        = 0;
    g_RequestsPerFrame[i] = 0;
  }
  g_CachedTextureCount = 0;
}

void
CacheTests(void (*CacheTestFunctions[])(Text::font*), Text::font* Font, int** ExpectedOutputs,
           int* Length, int Count)
{
  for(int i = 0; i < Count; i++)
  {
    ResetCache();
    (*CacheTestFunctions[i])(Font);
    if(Length[i] != g_CachedTextureCount)
    {
      printf("Expected Test %d sample size is not equal to actual sample size!\n", i);
    }
    printf(
      "Test%d\n--------------------------------------------\nExpected\t--\tActual\t--\tDiff\n--"
      "------------------------------------------\n",
      i);
    for(int j = 0; j < Length[i]; j++)
    {
      printf("Texture %d\n%d\t\t--\t%d\t--\t%d\n", j, ExpectedOutputs[i][j], g_HitCounts[j],
             ExpectedOutputs[i][j] - g_HitCounts[j]);
    }
    printf("============================================\n");
  }
  ResetCache();
}

void
CacheTest0(Text::font* Font)
{
  int   ParamCount = 1;
  vec4  Color[1]   = { { 1.0f, 1.0f, 1.0f, 1.0f } };
  char* Text[]     = { "Test1" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 20; i++)
  {
    float TextWidth  = 0.3f;
    float TextHeight = 0.1f;

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
CacheTest1(Text::font* Font)
{
  int  ParamCount = 3;
  vec4 Color[3]   = { { 1.0f, 1.0f, 1.0f, 1.0f },
                    { 1.0f, 0.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 0.0f, 1.0f } };
  char* Text[] = { "Testas", "Sec", "Third" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 40; i++)
  {
    float TextWidth  = 0.5f;
    float TextHeight = 0.2f;

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
CacheTest2(Text::font* Font)
{
  int  ParamCount = TEXTURE_CACHE_LINE_COUNT;
  vec4 Color;
  char Text[100];

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 2 * TEXTURE_CACHE_LINE_COUNT; i++)
  {
    float TextWidth  = 0.4f;
    float TextHeight = 0.1f;

    Color = { 1.0f, 1.0f - (float)(i % ParamCount) * 0.001f, (float)(i % ParamCount) * 0.002f,
              1.0f };
    sprintf(Text, "Texture %d", (i % ParamCount));

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)), Text, Color,
                       &TextureWidth, &TextureHeight);
  }
}

void
CacheTest3(Text::font* Font)
{
  int   ParamCount = 1;
  vec4  Color[1]   = { { 1.0f, 1.0f, 1.0f, 1.0f } };
  char* Text[]     = { "a" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 5; i++)
  {
    float TextWidth  = 0.3f;
    float TextHeight = 0.1f;

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
CacheTest4(Text::font* Font)
{
  int  ParamCount = 3;
  vec4 Color[3]   = { { 0.0f, 0.0f, 0.0f, 1.0f },
                    { 0.0f, 0.0f, 0.0f, 0.0f },
                    { 1.0f, 0.5f, 0.0f, 0.5f } };
  char* Text[] = { "White and visible", "Black and invisible", "Orange-ish and transparent" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 100; i++)
  {
    float TextWidth  = 0.3f;
    float TextHeight = 0.1f;

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
CacheTest5(Text::font* Font)
{
  int   ParamCount = 1;
  vec4  Color[1]   = { { 0.75f, 0.75f, 0.75f, 1.0f } };
  char* Text[]     = { "Hmmmmm" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 256; i++)
  {
    float TextWidth  = 0.3f;
    float TextHeight = 0.1f;

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
Text::SetupAndCallCacheTests(font* Font)
{
  const int Count = 6;
  void (*TestFunctions[Count])(Text::font*);

  TestFunctions[0] = CacheTest0;
  TestFunctions[1] = CacheTest1;
  TestFunctions[2] = CacheTest2;
  TestFunctions[3] = CacheTest3;
  TestFunctions[4] = CacheTest4;
  TestFunctions[5] = CacheTest5;

  int** ExpectedResults = (int**)malloc(sizeof(int*) * Count);
  ExpectedResults[0]    = (int*)malloc(sizeof(int) * 1);
  ExpectedResults[0][0] = 20;
  ExpectedResults[1]    = (int*)malloc(sizeof(int) * 3);
  ExpectedResults[1][0] = 14;
  ExpectedResults[1][1] = 13;
  ExpectedResults[1][2] = 13;
  ExpectedResults[2]    = (int*)malloc(sizeof(int) * 256);
  ExpectedResults[3]    = (int*)malloc(sizeof(int) * 1);
  ExpectedResults[3][0] = 5;
  ExpectedResults[4]    = (int*)malloc(sizeof(int) * 3);
  ExpectedResults[4][0] = 34;
  ExpectedResults[4][1] = 33;
  ExpectedResults[4][2] = 33;
  ExpectedResults[5]    = (int*)malloc(sizeof(int) * 1);
  ExpectedResults[5][0] = 256;
  for(int i = 0; i < TEXTURE_CACHE_LINE_COUNT; i++)
  {
    ExpectedResults[2][i] = 2;
  }
  int Length[Count] = { 1, 3, TEXTURE_CACHE_LINE_COUNT, 1, 3, 1 };

  CacheTests(TestFunctions, Font, ExpectedResults, Length, Count);
  for(int i = 0; i < 6; i++)
  {
    free(ExpectedResults[i]);
  }
  free(ExpectedResults);
}
#endif

void
Text::ClearTextRequestCounts()
{
  for(int i = 0; i < TEXTURE_CACHE_LINE_COUNT; i++)
  {
    g_RequestsPerFrame[i] = 0;
  }
}
