#include <GL/glew.h>

#include "text.h"

TTF_Font*
Text::OpenFont(const char* FontName, int FontSize)
{
  TTF_Font* Font;
  Font = TTF_OpenFont(FontName, FontSize);
  if(!Font)
  {
    printf("Font was not loaded!\nError: %s\n", SDL_GetError());
    return 0;
  }
  return Font;
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
    free(TextSurface);

    uint32_t Texture;

    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DestSurface->w, DestSurface->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, DestSurface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(DestSurface->pixels);
    free(DestSurface);

    return Texture;
  }
  else
  {
    printf("Text surface was not created!\nError: %s\n", SDL_GetError());
    return 0;
  }
}

bool
AreTexturesEqual(const char* TargetText, int32_t TargetFontSize, vec4 Color, text_texture* Texture)
{
  if(TargetFontSize == Texture->FontSize && Color == Texture->Color)
  {
    int   i           = 0;
    char* TextureText = Texture->Text;
    while(TargetText[i] != '\0')
    {
      if(TargetText[i] != TextureText[i])
      {
        return false;
      }
      i++;
    }
    if(TextureText[i] == TargetText[i])
    {
      return true;
    }
  }

  return false;
}

int32_t
FindTextTexture(const char* Text, int32_t FontSize, vec4 Color, int32_t TextureCount,
                text_texture* Textures)
{
  int32_t TextureIndex = -1;
  for(int i = 0; i < TextureCount; i++)
  {
    if(AreTexturesEqual(Text, FontSize, Color, &Textures[i]))
    {
      TextureIndex = i;
      break;
    }
  }

  return TextureIndex;
}

void
AppendTextBuffer(char* TextBuffer, int32_t* CurrentCharIndex, const char* Text)
{
  int i = 0;
  while(Text[i] != '\0')
  {
    TextBuffer[(*CurrentCharIndex)++] = Text[i++];
  }
  TextBuffer[(*CurrentCharIndex)++] = '\0';
}

int32_t
FindSizedFont(sized_font* Fonts, int32_t FontCount, int32_t FontSize)
{
  int Index   = 0;
  int MinDiff = Fonts[0].Size - FontSize < 0 ? FontSize - Fonts[0].Size : Fonts[0].Size - FontSize;

  for(int i = 1; i < FontCount; i++)
  {
    int Difference =
      Fonts[i].Size - FontSize < 0 ? FontSize - Fonts[i].Size : Fonts[i].Size - FontSize;
    if(Difference < MinDiff)
    {
      MinDiff = Difference;
      Index   = i;
    }
  }
  return Index;
}

int32_t
Text::CacheTextTexture(game_state* GameState, int32_t FontSize, const char* Text, vec4 Color)
{
  int32_t MatchingFontIndex = FindSizedFont(GameState->Fonts, GameState->FontCount, FontSize);
  int32_t TextureIndex      = FindTextTexture(Text, GameState->Fonts[MatchingFontIndex].Size, Color,
                                         GameState->TextTextureCount, GameState->TextTextures);

  if(TextureIndex == -1)
  {
    TextureIndex = GameState->TextTextureCount++;

    GameState->TextTextures[TextureIndex].Texture =
      LoadTextTexture(GameState->Fonts[MatchingFontIndex].Font, Text, Color);
    GameState->TextTextures[TextureIndex].Text =
      &GameState->TextBuffer[GameState->CurrentCharIndex];
    AppendTextBuffer(GameState->TextBuffer, &GameState->CurrentCharIndex, Text);
    GameState->TextTextures[TextureIndex].FontSize = GameState->Fonts[MatchingFontIndex].Size;
    GameState->TextTextures[TextureIndex].Color    = Color;
  }

  return TextureIndex;
}
