#include "load_texture.h"

#include <GL/glew.h>
#include <SDL2/SDL_image.h>

namespace Texture
{
  uint32_t
  LoadTexture(const char* FileName)
  {
    SDL_Surface* ImageSurface = IMG_Load(FileName);
    if(ImageSurface)
    {
      SDL_Surface* DestSurface =
        SDL_ConvertSurfaceFormat(ImageSurface, SDL_PIXELFORMAT_ABGR8888, 0);
      SDL_FreeSurface(ImageSurface);

      uint32_t Texture;
      glGenTextures(1, &Texture);
      glBindTexture(GL_TEXTURE_2D, Texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DestSurface->w, DestSurface->h, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, DestSurface->pixels);
      glGenerateMipmap(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);

      SDL_FreeSurface(DestSurface);

      return Texture;
    }
    else
    {
      printf("Platform: texture load from image error: %s\n", SDL_GetError());
      assert(0);
    }
  }
}
