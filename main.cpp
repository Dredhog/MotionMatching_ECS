#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

bool
Init(SDL_Window** Window)
{
  bool Success = true; // Init SDL
  if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    printf("SDL error: init failed. %s\n", SDL_GetError());
    Success = false;
  }
  else
  {
    // Set Opengl contet version to 3.3 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create an SDL window
    *Window =
      SDL_CreateWindow("ngpe - Non general-purpose engine", 0, 0, 640, 480, SDL_WINDOW_OPENGL);
    if(!Window)
    {
      printf("SDL error: failed to load window. %s\n", SDL_GetError());
      Success = false;
    }
    else
    {
      // Establish an OpenGL context with SDL
      SDL_GLContext GraphicsContext = SDL_GL_CreateContext(*Window);
      if(!GraphicsContext)
      {
        printf("SDL error: failed to establish an opengl context. %s\n", SDL_GetError());
        Success = false;
      }
      else
      {
        glewExperimental = GL_TRUE;
        GLenum GlewError = glewInit();
        if(GlewError != GLEW_OK)
        {
          printf("GLEW error: initialization failed. %s\n", glewGetErrorString(GlewError));
          Success = false;
        }

        if(SDL_GL_SetSwapInterval(1) < 0)
        {
          printf("SDL error: unable to set swap interval. %s\n", glewGetErrorString(GlewError));
          Success = false;
        }
      }
    }
  }
  return Success;
}

void
setupDrawSquare(GLuint* VAO, GLuint* shaderProgram)
{
  // Square vertices
  GLfloat vertices[] = {
    -0.5f, 0.5f,  0.0f, // Top left
    -0.5f, -0.5f, 0.0f, // Bottom left
    0.5f,  -0.5f, 0.0f, // Bottom right
    0.5f,  0.5f,  0.0f  // Top right
  };
  GLuint indices[] = { 0, 1, 2, 0, 2, 3 };

  // Vertex shader source code
  const GLchar* vertexShaderSource = "#version 330 core\n\n \
                                      layout (location = 0) in vec3 position;\n\n \
                                      void main()\n \
                                      {\n \
                                        gl_Position = vec4(position, 1.0f);\n \
                                      }";

  // Fragment shader source code
  const GLchar* fragmentShaderSource = "#version 330 core\n\n \
                                        out vec4 color;\n\n \
                                        uniform vec4 ourColor;\n\n \
                                        void main()\n \
                                        {\n \
                                          color = ourColor;\n \
                                        }";

  // Setting up vertex array object
  glGenVertexArrays(1, VAO);
  glBindVertexArray(*VAO);

  // Setting up vertex buffer object
  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Setting up element buffer object
  GLuint EBO;
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Setting up vertex shader
  GLuint vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // Checking if vertex shader compiled successfully
  GLint  success;
  GLchar infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    printf("Shader compilation failed!\n");
  }

  // Setting up fragment shader
  GLuint fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  // Checking if fragment shader compiled successfully
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    printf("Shader compilation failed!\n");
  }

  // Setting up shader program
  *shaderProgram = glCreateProgram();
  glAttachShader(*shaderProgram, vertexShader);
  glAttachShader(*shaderProgram, fragmentShader);
  glLinkProgram(*shaderProgram);

  // Deleting linked shaders
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Setting vertex attribute pointers
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  // Unbind VAO
  glBindVertexArray(0);
}

int
main(int argc, char* argv[])
{
  SDL_Window* Window = nullptr;
  if(!Init(&Window))
  {
    return -1;
  }
  printf("Everything went well...!\n");
  glViewport(0, 0, 640, 480);

  SDL_Event Event;

  GLuint VAO;
  GLuint shaderProgram;
  setupDrawSquare(&VAO, &shaderProgram);

  // Main loop
  while(true)
  {
    SDL_PollEvent(&Event);
    if(Event.type == SDL_QUIT)
    {
      break;
    }
    else if(Event.type == SDL_KEYDOWN && Event.key.keysym.sym == SDLK_ESCAPE)
    {
      break;
    }
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    unsigned int timeValue           = SDL_GetTicks();
    float        greenValue          = 0.5f * sinf((float)timeValue / 250.0f) + 0.5f;
    GLint        vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
    glUniform4f(vertexColorLocation, 0.4f, greenValue, 1.0f - greenValue, 1.0f);

    // Drawing square
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    SDL_GL_SwapWindow(Window);
    SDL_Delay(16);
  }

  SDL_DestroyWindow(Window);
  SDL_Quit();
  return 0;
}
