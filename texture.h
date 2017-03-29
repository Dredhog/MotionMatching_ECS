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
        SDL_ConvertSurfaceFormat(ImageSurface, SDL_PIXELFORMAT_ARGB8888, 0);
      Result.Texels = DestSurface->pixels;
      Result.Width  = DestSurface->w;
      Result.Height = DestSurface->h;
      free(DestSurface);

      assert(Result.Width > 0 && Result.Height > 0);
    }
    else
    {
      printf("Platform: bitmap load error: %s\n", SDL_GetError());
    }
    free(ImageSurface);

    return Result;
  }

  uint32_t
  LoadTexture(char* FileName)
  {
    loaded_bitmap Bitmap = LoadBitmapFromFile(FileName);
    uint32_t      Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Bitmap.Width, Bitmap.Height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, Bitmap.Texels);
    glGenerateMipmap(GL_TEXTURE_2D);
    free(Bitmap.Texels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return Texture;
  }
}
