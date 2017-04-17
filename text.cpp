#include <GL/glew.h>

#include <assert.h>
#include "linear_math/vector.h"

#include "text.h"
#include "string.h"
#include "misc.h"

#define TEXTURE_CACHE_LINE_COUNT 64

struct text_texture
{
  char*             Text;
  Text::sized_font* SizedFont;
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

Text::sized_font
Text::LoadSizedFont(const char* FontName, int FontSize)
{
  sized_font Result = {};
  TTF_Font*  Font   = TTF_OpenFont(FontName, FontSize);
  if(!Font)
  {
    printf("error: fialed to load font: %s\n", SDL_GetError());
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
FindCacheLineToOccupy(int32_t* HitCount, int32_t CacheLineCount)
{
  int32_t CandidateLineIndex = 0;
  int32_t MinHitCount        = 1000000000;
  for(int i = 0; i < CacheLineCount; i++)
  {
    if(HitCount[i] < MinHitCount)
    {
      MinHitCount        = HitCount[i];
      CandidateLineIndex = i;
      if(HitCount == 0)
      {
        break;
      }
    }
  }
  return CandidateLineIndex;
}

Text::sized_font
GetBestMatchingSizedFont(const Text::font* Font, int32_t FontSize)
{
  Text::sized_font Result;

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
Text::GetTextTextureID(font* Font, int32_t FontSize, const char* Text, vec4 Color)
{
  sized_font SizedFont = GetBestMatchingSizedFont(Font, FontSize);
  int32_t    TextTextureIndex =
    SearchForDesiredTextureID(g_TextTextureCache, &SizedFont, Text, Color, g_CachedTextureCount);
  if(TextTextureIndex < 0)
  {
    int32_t NewIndex = FindCacheLineToOccupy(g_HitCounts, TEXTURE_CACHE_LINE_COUNT);

    if(g_TextTextureCache[NewIndex].TextureID)
    {
      glDeleteTextures(1, &g_TextTextureCache[NewIndex].TextureID);
      g_HitCounts[TextTextureIndex] = 0;
    }

    g_TextTextureCache[NewIndex].TextureID = LoadTextTexture(SizedFont.Font, Text, Color);
    g_TextTextureCache[NewIndex].Text =
      WriteTextToLineBufferAtIndex(g_TextLineCache, Text, NewIndex);
    g_TextTextureCache[NewIndex].SizedFont = &SizedFont;
    g_TextTextureCache[NewIndex].Color     = Color;
    if(NewIndex == g_CachedTextureCount)
    {
      ++g_CachedTextureCount;
    }
    TextTextureIndex = NewIndex;
  }
  g_HitCounts[TextTextureIndex]++;
  return g_TextTextureCache[TextTextureIndex].TextureID;
}
