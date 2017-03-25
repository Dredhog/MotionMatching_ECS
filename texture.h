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
      Result.Texels = ImageSurface->pixels;
      Result.Width  = ImageSurface->w;
      Result.Height = ImageSurface->h;

      assert(Result.Width > 0 && Result.Height > 0);
    }
    else
    {
      printf("Platform: bitmap load error: %s\n", SDL_GetError());
    }

    return Result;
  }

  uint32_t
  LoadTexture(char* FileName)
  {
    loaded_bitmap Bitmap = LoadBitmapFromFile(FileName);
    uint32_t      Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Bitmap.Width, Bitmap.Height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 Bitmap.Texels);
    glGenerateMipmap(GL_TEXTURE_2D);
    free(Bitmap.Texels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return Texture;
  }
}
