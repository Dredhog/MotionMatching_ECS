#pragma once

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "mesh.h"
#include "model.h"
#include "file_io.h"
#include "asset.h"
#include "builder/pack.h"

static float
ClampFloat(float Min, float T, float Max)
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
  if(GameState->MagicChecksum != 123456)
  {
    GameState->MagicChecksum = 123456;
    GameState->TemporaryMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory, Mibibytes(20));
    GameState->PersistentMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.PersistentMemory, Mibibytes(40));

    // -------BEGIN ASSET LOADING
    Memory::stack_allocator* TemporaryMemStack  = GameState->TemporaryMemStack;
    Memory::stack_allocator* PersistentMemStack = GameState->PersistentMemStack;

    debug_read_file_result AssetReadResult =
      ReadEntireFile(PersistentMemStack, "data/nanosuit.model");

    assert(AssetReadResult.Contents);
    Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;
    assert(AssetHeader->Checksum == 12345);

    GameState->Model = (Render::model*)((uint8_t*)AssetHeader + AssetHeader->HeaderOffset);
    UnpackModel(GameState->Model);
    PrintModel(GameState->Model);

    for(int i = 0; i < GameState->Model->MeshCount; i++)
    {
      SetUpMesh(GameState->Model->Meshes[i]);
    }

    Memory::marker LoadStart = TemporaryMemStack->GetMarker();
    GameState->ShaderVertexColor              = LoadShader(TemporaryMemStack, "./shaders/diffuse");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderVertexColor < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    // -------END ASSET LOADING
    // ======Set GL state
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // -------InitGameState
    GameState->MeshEulerAngles = { 0, 0, 0 };
    GameState->MeshScale       = { 1.0f, 1.0f, 1.0f };
    GameState->Camera.P        = { 0, 1, 2 };
    GameState->Camera.Up       = { 0, 1, 0 };
    GameState->Camera.Forward  = { 0, 0, -1 };
    GameState->Camera.Right    = { 1, 0, 0 };
    GameState->Camera.Rotation = {};
  }

  // Update

  GameState->Camera.Rotation.X -= 0.05f * (float)Input->dMouseY;
  GameState->Camera.Rotation.Y -= 0.05f * (float)Input->dMouseX;

  GameState->Camera.Rotation.X = ClampFloat(-90.0f, GameState->Camera.Rotation.X, 90.0f);

  GameState->Camera.Forward =
    Math::MulMat3Vec3(Math::Mat4ToMat3(Math::Mat4Rotate(GameState->Camera.Rotation)), { 0, 0, -1 });
  GameState->Camera.Right   = Math::Cross(GameState->Camera.Forward, GameState->Camera.Up);
  GameState->Camera.Forward = Math::Normalized(GameState->Camera.Forward);
  GameState->Camera.Right   = Math::Normalized(GameState->Camera.Right);
  GameState->Camera.Up      = Math::Normalized(GameState->Camera.Up);

  GameState->Camera.Speed = 2.0f;
  if(Input->LeftCtrl.EndedDown)
  {
    GameState->Camera.Speed = 0.2f;
  }
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
    Math::MulMat4(Math::Mat4Rotate(GameState->MeshEulerAngles), Math::Mat4Scale(0.3f));
  mat4 CameraMatrix =
    Math::Mat4Camera(GameState->Camera.P, GameState->Camera.Forward, GameState->Camera.Up);
  mat4 ProjectMatrix =
    Math::Mat4Perspective(90.0f, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.001f, 1000.0f);
  mat4 MVPMatrix = Math::MulMat4(ProjectMatrix, Math::MulMat4(CameraMatrix, ModelMatrix));

  // Rendering
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 1
  // Regular Shader
  glUseProgram(GameState->ShaderVertexColor);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // Draw model
  for(int i = 0; i < GameState->Model->MeshCount; i++)
  {
    glBindVertexArray(GameState->Model->Meshes[i]->VAO);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderVertexColor, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    glDrawElements(GL_TRIANGLES, GameState->Model->Meshes[i]->IndiceCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }

#endif

  glBindVertexArray(0);
}

