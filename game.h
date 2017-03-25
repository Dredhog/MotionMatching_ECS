#pragma once

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "mesh.h"
#include "model.h"
#include "file_io.h"
#include "asset.h"
#include "builder/pack.h"
#include "camera_gizmo.h"

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  assert(GameMemory.HasBeenInitialized);
  if(GameState->MagicChecksum != 123456)
  {
    GameState->MagicChecksum = 123456;
    GameState->TemporaryMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory,
                                          GameMemory.TemporaryMemorySize);
    GameState->PersistentMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.PersistentMemory,
                                          GameMemory.PersistentMemorySize);

    // -------BEGIN ASSET MODELS AND ACTORS
    Memory::stack_allocator* TemporaryMemStack  = GameState->TemporaryMemStack;
    Memory::stack_allocator* PersistentMemStack = GameState->PersistentMemStack;

    // Set Up Gizmo
    debug_read_file_result AssetReadResult = ReadEntireFile(PersistentMemStack, "data/gizmo.model");

    assert(AssetReadResult.Contents);
    Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

    // assert(0);
    UnpackAsset(AssetHeader);
    GameState->GizmoModel = (Render::model*)AssetHeader->Model;
    for(int i = 0; i < GameState->GizmoModel->MeshCount; i++)
    {
      SetUpMesh(GameState->GizmoModel->Meshes[i]);
    }

    // Set Up Model
    AssetReadResult = ReadEntireFile(PersistentMemStack, "data/crysis_soldier.actor");

    assert(AssetReadResult.Contents);
    AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;
    UnpackAsset(AssetHeader);
    GameState->Model = (Render::model*)AssetHeader->Model;
    PrintModel(GameState->Model);
    for(int i = 0; i < GameState->Model->MeshCount; i++)
    {
      SetUpMesh(GameState->Model->Meshes[i]);
    }

    GameState->Skeleton = (Anim::skeleton*)AssetHeader->Skeleton;

    // -------BEGIN LOADING SHADERS
    // Set Up Sahders
    Memory::marker LoadStart = TemporaryMemStack->GetMarker();
    GameState->ShaderDiffuse = LoadShader(TemporaryMemStack, "shaders/diffuse");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderDiffuse < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    // Set Up Sahders
    LoadStart                  = TemporaryMemStack->GetMarker();
    GameState->ShaderWireframe = LoadShader(TemporaryMemStack, "shaders/wireframe");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderWireframe < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    LoadStart              = TemporaryMemStack->GetMarker();
    GameState->ShaderGizmo = LoadShader(TemporaryMemStack, "./shaders/gizmo");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderGizmo < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    // -------END ASSET LOADING
    // ======Set GL state
    glClearColor(0.2f, 0.3f, 0.5f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // -------InitGameState
    GameState->MeshEulerAngles      = { 0, 0, 0 };
    GameState->MeshScale            = { 1.0f, 1.0f, 1.0f };
    GameState->Camera.P             = { 0, 1, 5 };
    GameState->Camera.Up            = { 0, 1, 0 };
    GameState->Camera.Forward       = { 0, 0, -1 };
    GameState->Camera.Right         = { 1, 0, 0 };
    GameState->Camera.Rotation      = {};
    GameState->Camera.FieldOfView   = 90.0f;
    GameState->Camera.NearClipPlane = 0.001f;
    GameState->Camera.FarClipPlane  = 100.0f;
    GameState->Camera.MaxTiltAngle  = 90.0f;
  }

  // Update
  UpdateCamera(&GameState->Camera, Input);
  // GameState->MeshEulerAngles.Y += 2.0f * Input->dt;
  mat4 ModelMatrix =
    Math::MulMat4(Math::Mat4Rotate(GameState->MeshEulerAngles), Math::Mat4Scale(1));
  mat4 MVPMatrix = Math::MulMat4(GameState->Camera.VPMatrix, ModelMatrix);

  // Rendering
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 1
  // Regular Shader
  glUseProgram(GameState->ShaderDiffuse);
  for(int i = 0; i < GameState->Model->MeshCount; i++)
  {
    glBindVertexArray(GameState->Model->Meshes[i]->VAO);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderDiffuse, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    glDrawElements(GL_TRIANGLES, GameState->Model->Meshes[i]->IndiceCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }
#endif

#if 1
  // WireframeShader Shader
  glUseProgram(GameState->ShaderWireframe);
  glDisable(GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  for(int i = 0; i < GameState->Model->MeshCount; i++)
  {
    glBindVertexArray(GameState->Model->Meshes[i]->VAO);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderWireframe, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    glDrawElements(GL_TRIANGLES, GameState->Model->Meshes[i]->IndiceCount, GL_UNSIGNED_INT, 0);
  }
  glBindVertexArray(0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST);
#endif

  if(GameState->Skeleton)
  {
    mat4 BoneGizmos[SKELETON_MAX_BONE_COUNT];
    for(int i = 0; i < GameState->Skeleton->BoneCount; i++)
    {
      BoneGizmos[i] = Math::MulMat4(ModelMatrix, GameState->Skeleton->Bones[i].BindPose);
    }

    DEBUGDrawGizmo(GameState, &ModelMatrix, 1);
    DEBUGDrawGizmo(GameState, BoneGizmos, GameState->Skeleton->BoneCount);
  }
}

