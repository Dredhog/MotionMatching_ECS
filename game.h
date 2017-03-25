#pragma once

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "mesh.h"
#include "model.h"
#include "file_io.h"
#include "asset.h"
#include "builder/pack.h"
#include "camera_gizmo.h"

static const vec3 g_BoneColors[] = {
  { 0.41f, 0.93f, 0.23f }, { 0.14f, 0.11f, 0.80f }, { 0.35f, 0.40f, 0.53f },
  { 0.96f, 0.24f, 0.15f }, { 0.20f, 0.34f, 0.44f }, { 0.37f, 0.34f, 0.14f },
  { 0.22f, 0.99f, 0.77f }, { 0.80f, 0.70f, 0.11f }, { 0.81f, 0.92f, 0.18f },
  { 0.51f, 0.86f, 0.13f }, { 0.80f, 0.94f, 0.10f }, { 0.70f, 0.42f, 0.52f },
  { 0.26f, 0.50f, 0.61f }, { 0.10f, 0.21f, 0.81f }, { 0.96f, 0.22f, 0.63f },
  { 0.77f, 0.22f, 0.79f }, { 0.30f, 0.00f, 0.07f }, { 0.98f, 0.28f, 0.02f },
  { 0.92f, 0.42f, 0.14f }, { 0.47f, 0.31f, 0.72f },
};

static mat4 g_BoneSpaceMatrices[SKELETON_MAX_BONE_COUNT]         = {};
static mat4 g_ModelSpaceMatrices[SKELETON_MAX_BONE_COUNT]        = {};
static mat4 g_FinalHierarchicalMatrices[SKELETON_MAX_BONE_COUNT] = {};

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  assert(GameMemory.HasBeenInitialized);
  //---------------------BEGIN INIT -------------------------
  if(GameState->MagicChecksum != 123456)
  {
    GameState->MagicChecksum = 123456;
    GameState->TemporaryMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory,
                                          GameMemory.TemporaryMemorySize);
    GameState->PersistentMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.PersistentMemory,
                                          GameMemory.PersistentMemorySize);

    // -------BEGIN ASSETS
    Memory::stack_allocator* TemporaryMemStack  = GameState->TemporaryMemStack;
    Memory::stack_allocator* PersistentMemStack = GameState->PersistentMemStack;

    // Set Up Gizmo
    debug_read_file_result AssetReadResult = ReadEntireFile(PersistentMemStack, "data/gizmo.model");

    assert(AssetReadResult.Contents);
    Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

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
    // Diffuse
    Memory::marker LoadStart = TemporaryMemStack->GetMarker();
    GameState->ShaderDiffuse = LoadShader(TemporaryMemStack, "shaders/diffuse");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderDiffuse < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }
    // Bone Color
    LoadStart                  = TemporaryMemStack->GetMarker();
    GameState->ShaderBoneColor = LoadShader(TemporaryMemStack, "shaders/bone_color");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderBoneColor < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    // Wireframe
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
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // -------InitGameState
    GameState->MeshEulerAngles      = { 0, 0, 0 };
    GameState->MeshScale            = { 1.0f, 1.0f, 1.0f };
    GameState->Camera.P             = { 0, 0.5f, 1 };
    GameState->Camera.Up            = { 0, 1, 0 };
    GameState->Camera.Forward       = { 0, 0, -1 };
    GameState->Camera.Right         = { 1, 0, 0 };
    GameState->Camera.Rotation      = {};
    GameState->Camera.FieldOfView   = 90.0f;
    GameState->Camera.NearClipPlane = 0.001f;
    GameState->Camera.FarClipPlane  = 100.0f;
    GameState->Camera.MaxTiltAngle  = 90.0f;

    GameState->DrawWireframe   = false;
    GameState->DrawBoneWeights = false;
    GameState->DrawGizmos      = false;
  }
  GameState->SkeletonPoseKeyframe.Transforms[4].Rotation.X +=  cosf(Input->dt);

  //---------------------END INIT -------------------------

  //----------------------UPDATE------------------------
  UpdateCamera(&GameState->Camera, Input);
  if(Input->b.EndedDown && Input->b.Changed)
  {
    GameState->DrawBoneWeights = !GameState->DrawBoneWeights;
  }
  if(Input->g.EndedDown && Input->g.Changed)
  {
    GameState->DrawGizmos = !GameState->DrawGizmos;
  }
  if(Input->f.EndedDown && Input->f.Changed)
  {
    GameState->DrawWireframe = !GameState->DrawWireframe;
  }
  // GameState->MeshEulerAngles.Y += 2.0f * Input->dt;
  mat4 ModelMatrix =
    Math::MulMat4(Math::Mat4Rotate(GameState->MeshEulerAngles), Math::Mat4Scale(1));
  mat4 MVPMatrix = Math::MulMat4(GameState->Camera.VPMatrix, ModelMatrix);

  // Update animation
  Anim::ComputeBoneSpacePoses(g_BoneSpaceMatrices, GameState->SkeletonPoseKeyframe.Transforms,
                              GameState->Skeleton->BoneCount);
  Anim::ComputeModelSpacePoses(g_ModelSpaceMatrices, g_BoneSpaceMatrices, GameState->Skeleton);
  Anim::ComputeFinalHierarchicalPoses(g_FinalHierarchicalMatrices, g_ModelSpaceMatrices,
                                      GameState->Skeleton);

  //---------------------RENDERING----------------------------
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if(GameState->DrawBoneWeights)
  {
    // Bone Color Shader
    glUseProgram(GameState->ShaderBoneColor);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderBoneColor, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    glUniform3fv(glGetUniformLocation(GameState->ShaderBoneColor, "g_bone_colors"), 20,
                 (float*)&g_BoneColors);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderBoneColor, "g_bone_matrices"), 20,
                       GL_FALSE, (float*)g_FinalHierarchicalMatrices);
    for(int i = 0; i < GameState->Model->MeshCount; i++)
    {
      glBindVertexArray(GameState->Model->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->Model->Meshes[i]->IndiceCount, GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
    }
  }
  else
  {
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
  }

  if(GameState->DrawWireframe)
  {
    // WireframeShader Shader
    glUseProgram(GameState->ShaderWireframe);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // glClear(GL_DEPTH_BUFFER_BIT);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderWireframe, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    for(int i = 0; i < GameState->Model->MeshCount; i++)
    {
      glBindVertexArray(GameState->Model->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->Model->Meshes[i]->IndiceCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  if(GameState->DrawGizmos && GameState->Skeleton)
  {
    mat4 BoneGizmos[SKELETON_MAX_BONE_COUNT];
    for(int i = 0; i < GameState->Skeleton->BoneCount; i++)
    {
      BoneGizmos[i] =
        Math::MulMat4(ModelMatrix, Math::MulMat4(g_FinalHierarchicalMatrices[i],
                                                 GameState->Skeleton->Bones[i].BindPose));
    }

    DEBUGDrawGizmo(GameState, &ModelMatrix, 1);
    DEBUGDrawGizmo(GameState, BoneGizmos, GameState->Skeleton->BoneCount);
  }
}

