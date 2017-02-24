#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>

int main(int argc, char *argv[]) {
  //Init SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		printf("SDL: init failed. %s", SDL_GetError());
		return -1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GLContext GraphicsContext;

  SDL_Window *Window = SDL_CreateWindow("ngpe - Non general-purpose engine", 0,
                                        0, 640, 480, SDL_WINDOW_OPENGL);
  if (Window) {
    GraphicsContext = SDL_GL_CreateContext(Window);
  } else {
    printf("SDL: failed to load window. %s\n", SDL_GetError());
    return -1;
  }

  // glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    printf("GLEW: failed to load OpenGL functions.\n");
    return -1;
  }

  SDL_Event Event;
  // Main loop
  while (true) {
    SDL_PollEvent(&Event);
    if (Event.type == SDL_QUIT) {
      break;
    } else if (Event.type == SDL_KEYDOWN &&
               Event.key.keysym.sym == SDLK_ESCAPE) {
      break;
    }
    SDL_GL_SwapWindow(Window);
    SDL_Delay(16);
  }

  SDL_DestroyWindow(Window);
  return 0;
}
