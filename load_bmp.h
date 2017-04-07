#pragma once

namespace Texture
{
  struct loaded_bitmap
  {
    void*   Texels;
    int32_t Width;
    int32_t Height;
  };

  loaded_bitmap
  LoadBitmapFromFile(char* FileName)
  {
    loaded_bitmap Result       = {};
    SDL_Surface*  ImageSurface = SDL_LoadBMP(FileName);
    if(ImageSurface)
    {
      SDL_Surface* DestSurface =
        SDL_ConvertSurfaceFormat(ImageSurface, SDL_PIXELFORMAT_ABGR8888, 0);
      Result.Texels = DestSurface->pixels;
      Result.Width  = DestSurface->w;
      Result.Height = DestSurface->h;
      free(DestSurface);

      assert(Result.Width > 0 && Result.Height > 0);
      free(ImageSurface);
    }
    else
    {
      printf("Platform: bitmap load error: %s\n", SDL_GetError());
      Result = {};
    }

    return Result;
  }

  uint32_t
  LoadBMPTexture(char* FileName)
  {
    loaded_bitmap Bitmap = LoadBitmapFromFile(FileName);
    if(Bitmap.Texels)
    {
      uint32_t Texture;
      glGenTextures(1, &Texture);
      glBindTexture(GL_TEXTURE_2D, Texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Bitmap.Width, Bitmap.Height, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, Bitmap.Texels);
      glGenerateMipmap(GL_TEXTURE_2D);
      free(Bitmap.Texels);
      glBindTexture(GL_TEXTURE_2D, 0);
      return Texture;
    }
    return 0;
  }

  uint32_t
  LoadTextTexture(const char* FontName, int FontSize, const char* Text, SDL_Color Color)
  {
    TTF_Font* Font;
    Font = TTF_OpenFont(FontName, FontSize);
    if(!Font)
    {
      printf("Font was not loaded!\nError: %s\n", SDL_GetError());
      return 0;
    }

    SDL_Surface* TextSurface = TTF_RenderUTF8_Blended(Font, Text, Color);
    if(TextSurface)
    {
      free(Font);
      SDL_Surface* DestSurface = SDL_ConvertSurfaceFormat(TextSurface, SDL_PIXELFORMAT_ABGR8888, 0);
      free(TextSurface);

      uint32_t Texture;

      glGenTextures(1, &Texture);
      glBindTexture(GL_TEXTURE_2D, Texture);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DestSurface->w, DestSurface->h, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, DestSurface->pixels);
      glGenerateMipmap(GL_TEXTURE_2D);

      free(DestSurface->pixels);
      free(DestSurface);
      glBindTexture(GL_TEXTURE_2D, 0);

      return Texture;
    }
    else
    {
      printf("Text surface was not created!\nError: %s\n", SDL_GetError());
      return 0;
    }
  }
}
