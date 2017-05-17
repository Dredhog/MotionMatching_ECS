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
      free(ImageSurface);

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
      printf("Platform: texture load from image error: %s\n", SDL_GetError());
      return 0;
    }
  }

  uint32_t
  LoadCubemap(Memory::stack_allocator* const Allocator, char* CubemapPath, char* FileFormat)
  {
    char* CubemapFaces[6];
    CubemapFaces[0] =
      (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_right.") + 1));
    CubemapFaces[1] =
      (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_left.") + 1));
    CubemapFaces[2] =
      (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_top.") + 1));
    CubemapFaces[3] =
      (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_bottom.") + 1));
    CubemapFaces[4] =
      (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_back.") + 1));
    CubemapFaces[5] =
      (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_front.") + 1));
    strcpy(CubemapFaces[0], "_right.\0");
    strcpy(CubemapFaces[1], "_left.\0");
    strcpy(CubemapFaces[2], "_top.\0");
    strcpy(CubemapFaces[3], "_bottom.\0");
    strcpy(CubemapFaces[4], "_back.\0");
    strcpy(CubemapFaces[5], "_front.\0");

    char* FileNames[6];
    FileNames[0] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
      (strlen(CubemapPath) + strlen(CubemapFaces[0]) + strlen(FileFormat) + 1)));
    FileNames[1] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
      (strlen(CubemapPath) + strlen(CubemapFaces[1]) + strlen(FileFormat) + 1)));
    FileNames[2] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
      (strlen(CubemapPath) + strlen(CubemapFaces[2]) + strlen(FileFormat) + 1)));
    FileNames[3] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
      (strlen(CubemapPath) + strlen(CubemapFaces[3]) + strlen(FileFormat) + 1)));
    FileNames[4] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
      (strlen(CubemapPath) + strlen(CubemapFaces[4]) + strlen(FileFormat) + 1)));
    FileNames[5] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
      (strlen(CubemapPath) + strlen(CubemapFaces[5]) + strlen(FileFormat) + 1)));

    for(int i = 0; i < 6; i++)
    {
      strcpy(FileNames[i], CubemapPath);
      strcat(FileNames[i], CubemapFaces[i]);
      strcat(FileNames[i], FileFormat);
      strcat(FileNames[i], "\0");
    }

    uint32_t Texture;
    glGenTextures(1, &Texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Texture);

    for(int i = 0; i < 6; i++)
    {
      SDL_Surface* ImageSurface = IMG_Load(FileNames[i]);
      if(ImageSurface)
      {
        SDL_Surface* DestSurface =
          SDL_ConvertSurfaceFormat(ImageSurface, SDL_PIXELFORMAT_ABGR8888, 0);
        free(ImageSurface);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, DestSurface->w, DestSurface->h,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, DestSurface->pixels);
        free(DestSurface->pixels);
        free(DestSurface);
      }
      else
      {
        printf("Platform: texture load from image error: %s\n", SDL_GetError());
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        return 0;
      }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return Texture;
  }
}
