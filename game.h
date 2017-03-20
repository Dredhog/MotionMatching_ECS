#pragma once

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

void SetUpMesh(Mesh::mesh* Mesh);

float
CapFloat(float Min, float T, float Max)
{
  if(T < Min)
  {
    return Min;
  }
  if(T > Max)
  {
    return Max;
  }
  return T;
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  assert(GameMemory.HasBeenInitialized);
  if(GameState->MagicChecksum != 732932)
  {
    GameState->TemporaryMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory, Mibibytes(20));
    GameState->PersistentMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.PersistentMemory, Mibibytes(20));

    // -------BEGIN ASSET LOADING
    Memory::stack_allocator* TemporaryMemStack  = GameState->TemporaryMemStack;
    Memory::stack_allocator* PersistentMemStack = GameState->PersistentMemStack;
    Memory::marker           LoadStart          = TemporaryMemStack->GetMarker();
    GameState->Mesh = Loader::LoadOBJMesh(TemporaryMemStack, PersistentMemStack,
                                          "./data/armadillo.obj", Mesh::MAM_UseNormals);
    if(!GameState->Mesh.VerticeCount)
    {
      printf("ReadOBJ error: no vertices read\n");
      assert(false);
    }
    else
    {
      SetUpMesh(&GameState->Mesh);
    }
    TemporaryMemStack->FreeToMarker(LoadStart);

    LoadStart                    = TemporaryMemStack->GetMarker();
    GameState->ShaderVertexColor = LoadShader(TemporaryMemStack, "./shaders/color");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderVertexColor < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    LoadStart                  = TemporaryMemStack->GetMarker();
    GameState->ShaderWireframe = LoadShader(TemporaryMemStack, "./shaders/wireframe");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderVertexColor < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }
    // -------END ASSET LOADING
    // -------InitGameState
    GameState->MeshEulerAngles = {0, 180, 0};
    GameState->MeshScale       = { 1.0f, 1.0f, 1.0f };
    GameState->MagicChecksum   = 732932;
    GameState->Camera.P        = { 0, 0, 1 };
    GameState->Camera.Up       = { 0, 1, 0 };
    GameState->Camera.Forward  = { 0, 0, -1 };
    GameState->Camera.Right    = { 1, 0, 0 };
    GameState->Camera.Rotation = {};
    GameState->Camera.Speed    = 1.0f;
    *AssetsHaveLoaded          = true;
  }

  // Update

  GameState->Camera.Rotation.X -= 0.05f * (float)Input->dMouseY;
  GameState->Camera.Rotation.Y -= 0.05f * (float)Input->dMouseX;

  GameState->Camera.Rotation.X = CapFloat(-70.0f, GameState->Camera.Rotation.X, 70.0f);

  GameState->Camera.Forward =
    Math::MulMat3Vec3(Math::Mat4ToMat3(Math::Mat4Rotate(GameState->Camera.Rotation)), { 0, 0, -1 });
  GameState->Camera.Right = Math::Cross(GameState->Camera.Forward, GameState->Camera.Up);
	GameState->Camera.Forward = Math::Normalized(GameState->Camera.Forward);
	GameState->Camera.Right = Math::Normalized(GameState->Camera.Right);
	GameState->Camera.Up = Math::Normalized(GameState->Camera.Up);

  if(Input->LeftShift.EndedDown)
  {
    if(Input->w.EndedDown)
    {
      GameState->Camera.P += Input->dt * GameState->Camera.Speed * GameState->Camera.Up;
    }
    if(Input->s.EndedDown)
    {
      GameState->Camera.P -= Input->dt * GameState->Camera.Speed * GameState->Camera.Up;
    }
  }
  else
  {
    if(Input->w.EndedDown)
    {
      GameState->Camera.P += Input->dt * GameState->Camera.Speed * GameState->Camera.Forward;
    }
    if(Input->s.EndedDown)
    {
      GameState->Camera.P -= Input->dt * GameState->Camera.Speed * GameState->Camera.Forward;
    }
  }
  if(Input->a.EndedDown)
  {
    GameState->Camera.P -= Input->dt * GameState->Camera.Speed * GameState->Camera.Right;
  }
  if(Input->d.EndedDown)
  {
    GameState->Camera.P += Input->dt * GameState->Camera.Speed * GameState->Camera.Right;
  }

  mat4 ModelMatrix =
    Math::MulMat4(Math::Mat4Rotate(GameState->MeshEulerAngles), Math::Mat4Scale(0.25f));
  mat4 CameraMatrix =
    Math::Mat4Camera(GameState->Camera.P, GameState->Camera.Forward, GameState->Camera.Up);
  mat4 ProjectMatrix = Math::Mat4Perspective(60.f, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
  mat4 MVPMatrix     = Math::MulMat4(ProjectMatrix, Math::MulMat4(CameraMatrix, ModelMatrix));

  // Rendering
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindVertexArray(GameState->Mesh.VAO);

  // Switch to normal color shader
  glUseProgram(GameState->ShaderVertexColor);
#if 1
  glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderVertexColor, "mat_mvp"), 1, GL_FALSE,
                     &MVPMatrix.e[0]);

  // draw model
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDrawElements(GL_TRIANGLES, GameState->Mesh.IndiceCount, GL_UNSIGNED_INT, 0);
#endif

#if 0
  // Switch to wireframe shader
  glUseProgram(GameState->ShaderWireframe);
  glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderVertexColor, "mat_mvp"), 1, GL_FALSE,
                     &MVPMatrix.e[0]);

  // draw wireframe
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawElements(GL_TRIANGLES, GameState->Mesh.IndiceCount, GL_UNSIGNED_INT, 0);
#endif
  glBindVertexArray(0);
}

void
SetUpMesh(Mesh::mesh* Mesh)
{
  // Setting up vertex array object
  glGenVertexArrays(1, &Mesh->VAO);
  glBindVertexArray(Mesh->VAO);

  // Setting up vertex buffer object
  glGenBuffers(1, &Mesh->VBO);
  glBindBuffer(GL_ARRAY_BUFFER, Mesh->VBO);
  glBufferData(GL_ARRAY_BUFFER, Mesh->VerticeCount * Mesh->FloatsPerVertex * sizeof(float),
               Mesh->Floats, GL_STATIC_DRAW);

  // Setting up element buffer object
  glGenBuffers(1, &Mesh->EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, Mesh->IndiceCount * sizeof(uint32_t), Mesh->Indices,
               GL_STATIC_DRAW);

  // Setting vertex attribute pointers
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Mesh->FloatsPerVertex * sizeof(float),
                        (GLvoid*)(uint64_t)(Mesh->Offsets[0] * sizeof(float)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, Mesh->FloatsPerVertex * sizeof(float),
                        (GLvoid*)(uint64_t)(Mesh->Offsets[2] * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Unbind VAO
  glBindVertexArray(0);
}
