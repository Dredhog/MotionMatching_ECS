#pragma once

#include "load_shader.h"

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "game.h"
#include "mesh.h"
#include "model.h"
#include "file_io.h"
#include "asset.h"
#include "builder/pack.h"
#include "load_texture.h"
#include "misc.h"
#include "intersection_testing.h"
#include "editor_ui.h"
#include "load_data.h"

#include "debug_drawing.h"
#include "camera.h"
#define demo 1

static const vec3 g_BoneColors[] = {
  { 0.41f, 0.93f, 0.23f }, { 0.14f, 0.11f, 0.80f }, { 0.35f, 0.40f, 0.77f },
  { 0.96f, 0.24f, 0.15f }, { 0.20f, 0.34f, 0.44f }, { 0.37f, 0.34f, 0.14f },
  { 0.22f, 0.99f, 0.77f }, { 0.80f, 0.70f, 0.11f }, { 0.81f, 0.92f, 0.18f },
  { 0.51f, 0.86f, 0.13f }, { 0.80f, 0.94f, 0.10f }, { 0.70f, 0.42f, 0.52f },
  { 0.26f, 0.50f, 0.61f }, { 0.10f, 0.21f, 0.81f }, { 0.96f, 0.22f, 0.63f },
  { 0.77f, 0.22f, 0.79f }, { 0.30f, 0.00f, 0.07f }, { 0.98f, 0.28f, 0.02f },
  { 0.92f, 0.42f, 0.14f }, { 0.47f, 0.31f, 0.72f },
};

void
AddTexture(game_state* GameState, int32_t TextureID)
{
  assert(TextureID);
  assert(0 <= GameState->TextureCount && GameState->TextureCount < TEXTURE_MAX_COUNT);
  GameState->Textures[GameState->TextureCount++] = TextureID;
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  game_state* GameState = (game_state*)GameMemory.PersistentMemory;
  assert(GameMemory.HasBeenInitialized);
  //---------------------BEGIN INIT -------------------------
  if(GameState->MagicChecksum != 123456)
  {
    GameState->WAVLoaded     = false;
    GameState->MagicChecksum = 123456;
    GameState->PersistentMemStack =
      Memory::CreateStackAllocatorInPlace((uint8_t*)GameMemory.PersistentMemory +
                                            sizeof(game_state),
                                          GameMemory.PersistentMemorySize - sizeof(game_state));
    GameState->TemporaryMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory,
                                          GameMemory.TemporaryMemorySize);
    Memory::stack_allocator* TemporaryMemStack  = GameState->TemporaryMemStack;
    Memory::stack_allocator* PersistentMemStack = GameState->PersistentMemStack;

    // --------LOAD MODELS/ACTORS--------
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/gizmo1.model",
                             &GameState->GizmoModel, NULL);
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/debug_meshes.model",
                             &GameState->QuadModel, NULL);
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/multimesh_soldier.actor",
                             &GameState->CharacterModel, &GameState->AnimEditor.Skeleton);
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/inverse_cube.model",
                             &GameState->Cubemap, NULL);
    CheckedLoadAndSetUpAsset(PersistentMemStack, "./data/built/sphere.model",
                             &GameState->SphereModel, NULL);
    // -----------LOAD SHADERS------------
    GameState->ShaderPhong   = CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/phong");
    GameState->ShaderCubemap = CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/cubemap");
    GameState->ShaderGizmo   = CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/gizmo");
    GameState->ShaderQuad = CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/debug_quad");
    GameState->ShaderColor = CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/color");
    GameState->ShaderSkeletalBoneColor =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/skeletal_bone_color");
    GameState->ShaderSkeletalPhong =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/skeletal_phong");
    GameState->ShaderWireframe =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/wireframe");
    GameState->ShaderTexturedQuad =
      CheckedLoadCompileFreeShader(TemporaryMemStack, "./shaders/debug_textured_quad");
    //------------LOAD TEXTURES-----------
    AddTexture(GameState, Texture::LoadTexture("./data/textures/hand_dif.png"));
    AddTexture(GameState, Texture::LoadTexture("./data/textures/helmet_diff.png"));
    AddTexture(GameState, Texture::LoadTexture("./data/textures/glass_dif.png"));
    AddTexture(GameState, Texture::LoadTexture("./data/textures/body_dif.png"));
    AddTexture(GameState, Texture::LoadTexture("./data/textures/leg_dif.png"));
    AddTexture(GameState, Texture::LoadTexture("./data/textures/arm_dif.png"));
    AddTexture(GameState, Texture::LoadTexture("./data/textures/body_dif.png"));
    GameState->CollapsedTexture = Texture::LoadTexture("./data/textures/collapsed.bmp");
    GameState->ExpandedTexture  = Texture::LoadTexture("./data/textures/expanded.bmp");
    assert(GameState->CollapsedTexture);
    assert(GameState->ExpandedTexture);

    SDL_Color Color;
    Color.a = 255;
    Color.r = 0;
    Color.g = 0;
    Color.b = 0;
    GameState->TextTexture =
      Texture::LoadTextTexture("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
                               32, "Testing Text", Color);
    GameState->CubemapTexture =
      Texture::LoadCubemap(TemporaryMemStack, "./data/textures/iceflats", "tga");
    // -------END ASSET LOADING

    // ======Set GL state
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    // -------InitGameState
    GameState->ModelTransform.Rotation = { 0, 0, 0 };
    GameState->ModelTransform.Scale    = { 1, 1, 1 };
    GameState->Camera.Position         = { 0, 1.6f, 2 };
    GameState->Camera.Up               = { 0, 1, 0 };
    GameState->Camera.Forward          = { 0, 0, -1 };
    GameState->Camera.Right            = { 1, 0, 0 };
    GameState->Camera.Rotation         = { -20 };
    GameState->Camera.NearClipPlane    = 0.001f;
    GameState->Camera.FarClipPlane     = 1000.0f;
    GameState->Camera.FieldOfView      = 70.0f;
    GameState->Camera.MaxTiltAngle     = 90.0f;

    GameState->LightPosition    = { 2.25f, 1.0f, 1.0f };
    GameState->LightColor       = { 0.7f, 0.7f, 0.75f };
    GameState->AmbientStrength  = 0.8f;
    GameState->SpecularStrength = 0.6f;

    GameState->DrawWireframe           = false;
    GameState->DrawCubemap             = true;
    GameState->DrawBoneWeights         = false;
    GameState->DrawTimeline            = true;
    GameState->DrawGizmos              = true;
    GameState->IsModelSpinning         = false;
    GameState->IsAnimationPlaying      = false;
    GameState->EditorBoneRotationSpeed = 45.0f;

    if(GameState->AnimEditor.Skeleton)
    {
      EditAnimation::InsertBlendedKeyframeAtTime(&GameState->AnimEditor,
                                                 GameState->AnimEditor.PlayHeadTime);
    }
  }
  //---------------------END INIT -------------------------

  //----------------------UPDATE------------------------
  GameState->GameTime += Input->dt;

  UpdateCamera(&GameState->Camera, Input);

  if(GameState->IsModelSpinning)
  {
    GameState->ModelTransform.Rotation.Y += 45.0f * Input->dt;
  }
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

  mat4 ModelMatrix =
    Math::MulMat4(Math::Mat4Translate(GameState->ModelTransform.Translation),
                  Math::MulMat4(Math::Mat4Rotate(GameState->ModelTransform.Rotation),
                                Math::Mat4Scale(GameState->ModelTransform.Scale)));
  mat4 MVPMatrix = Math::MulMat4(GameState->Camera.VPMatrix, ModelMatrix);

  //---------------ANIMATION EDITOR UPDATE-----------------
  {
    if(Input->Space.EndedDown && Input->Space.Changed)
    {
      GameState->IsAnimationPlaying = !GameState->IsAnimationPlaying;
    }
    if(GameState->IsAnimationPlaying)
    {
      EditAnimation::PlayAnimation(&GameState->AnimEditor, Input->dt);
    }
    if(Input->i.EndedDown && Input->i.Changed)
    {
      InsertBlendedKeyframeAtTime(&GameState->AnimEditor, GameState->AnimEditor.PlayHeadTime);
    }
    if(Input->LeftShift.EndedDown)
    {
      if(Input->n.EndedDown && Input->n.Changed)
      {
        EditAnimation::EditPreviousBone(&GameState->AnimEditor);
      }
      if(Input->ArrowLeft.EndedDown && Input->ArrowLeft.Changed)
      {
        EditAnimation::JumpToPreviousKeyframe(&GameState->AnimEditor);
      }
      if(Input->ArrowRight.EndedDown && Input->ArrowRight.Changed)
      {
        EditAnimation::JumpToNextKeyframe(&GameState->AnimEditor);
      }
    }
    else
    {
      if(Input->n.EndedDown && Input->n.Changed)
      {
        EditAnimation::EditNextBone(&GameState->AnimEditor);
      }
      if(Input->ArrowLeft.EndedDown)
      {
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, -1 * Input->dt);
      }
      if(Input->ArrowRight.EndedDown)
      {
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, 1 * Input->dt);
      }
    }
    if(Input->LeftCtrl.EndedDown)
    {
      if(Input->c.EndedDown && Input->c.Changed)
      {
        EditAnimation::CopyKeyframeToClipboard(&GameState->AnimEditor,
                                               GameState->AnimEditor.CurrentKeyframe);
      }
      else if(Input->x.EndedDown && Input->x.Changed)
      {
        EditAnimation::CopyKeyframeToClipboard(&GameState->AnimEditor,
                                               GameState->AnimEditor.CurrentKeyframe);
        DeleteCurrentKeyframe(&GameState->AnimEditor);
      }
      else if(Input->v.EndedDown && Input->v.Changed)
      {
        EditAnimation::InsertKeyframeFromClipboardAtTime(&GameState->AnimEditor,
                                                         GameState->AnimEditor.PlayHeadTime);
      }
    }
    if(Input->Delete.EndedDown && Input->Delete.Changed)
    {
      EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
    }
    if(GameState->AnimEditor.KeyframeCount > 0)
    {
      EditAnimation::CalculateHierarchicalmatricesAtTime(&GameState->AnimEditor);
    }
  }

  //---------------------RENDERING----------------------------

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  if(GameState->DrawCubemap)
  {
    glDepthFunc(GL_LEQUAL);
    glUseProgram(GameState->ShaderCubemap);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderCubemap, "mat_projection"), 1,
                       GL_FALSE, GameState->Camera.ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderCubemap, "mat_view"), 1, GL_FALSE,
                       Math::Mat3ToMat4(Math::Mat4ToMat3(GameState->Camera.ViewMatrix)).e);
    glBindTexture(GL_TEXTURE_CUBE_MAP, GameState->CubemapTexture);
    for(int i = 0; i < GameState->Cubemap->MeshCount; i++)
    {
      glBindVertexArray(GameState->Cubemap->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->Cubemap->Meshes[i]->IndiceCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDepthFunc(GL_LESS);
  }
  if(GameState->AnimEditor.Skeleton)
  {
    if(GameState->DrawBoneWeights)
    {
      // Bone Color Shader
      glUseProgram(GameState->ShaderSkeletalBoneColor);
      glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "mat_mvp"), 1,
                         GL_FALSE, MVPMatrix.e);
      glUniform3fv(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "g_bone_colors"), 20,
                   (float*)&g_BoneColors);
      glUniform1i(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "g_selected_bone_index"),
                  GameState->AnimEditor.CurrentBone);
      glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalBoneColor,
                                              "g_bone_matrices"),
                         20, GL_FALSE,
                         (float*)GameState->AnimEditor.HierarchicalModelSpaceMatrices);
      for(int i = 0; i < GameState->CharacterModel->MeshCount; i++)
      {
        glBindVertexArray(GameState->CharacterModel->Meshes[i]->VAO);
        glDrawElements(GL_TRIANGLES, GameState->CharacterModel->Meshes[i]->IndiceCount,
                       GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
      }
    }
    else
    {
      // Regular Shader
      glUseProgram(GameState->ShaderSkeletalPhong);
      glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "g_bone_matrices"),
                         20, GL_FALSE,
                         (float*)GameState->AnimEditor.HierarchicalModelSpaceMatrices);
      glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "mat_mvp"), 1,
                         GL_FALSE, MVPMatrix.e);
      glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "mat_model"), 1,
                         GL_FALSE, ModelMatrix.e);
      glUniform1f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "ambient_strength"),
                  GameState->AmbientStrength);
      glUniform1f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "specular_strength"),
                  GameState->SpecularStrength);
      glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "light_position"),
                  GameState->LightPosition.X, GameState->LightPosition.Y,
                  GameState->LightPosition.Z);
      glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "light_color"),
                  GameState->LightColor.X, GameState->LightColor.Y, GameState->LightColor.Z);
      glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "camera_position"),
                  GameState->Camera.Position.X, GameState->Camera.Position.Y,
                  GameState->Camera.Position.Z);
      for(int i = 0; i < GameState->CharacterModel->MeshCount; i++)
      {
        glBindTexture(GL_TEXTURE_2D, GameState->Textures[i]);
        glBindVertexArray(GameState->CharacterModel->Meshes[i]->VAO);
        glDrawElements(GL_TRIANGLES, GameState->CharacterModel->Meshes[i]->IndiceCount,
                       GL_UNSIGNED_INT, 0);
      }
      glBindTexture(GL_TEXTURE_2D, 0);
      glBindVertexArray(0);
    }
  }
  else
  {
    // Regular Shader
    glUseProgram(GameState->ShaderPhong);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderPhong, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderPhong, "mat_model"), 1, GL_FALSE,
                       ModelMatrix.e);
    glUniform1f(glGetUniformLocation(GameState->ShaderPhong, "ambient_strength"),
                GameState->AmbientStrength);
    glUniform1f(glGetUniformLocation(GameState->ShaderPhong, "specular_strength"),
                GameState->SpecularStrength);
    glUniform3f(glGetUniformLocation(GameState->ShaderPhong, "light_position"),
                GameState->LightPosition.X, GameState->LightPosition.Y, GameState->LightPosition.Z);
    glUniform3f(glGetUniformLocation(GameState->ShaderPhong, "light_color"),
                GameState->LightColor.X, GameState->LightColor.Y, GameState->LightColor.Z);
    glUniform3f(glGetUniformLocation(GameState->ShaderPhong, "camera_position"),
                GameState->Camera.Position.X, GameState->Camera.Position.Y,
                GameState->Camera.Position.Z);
    for(int i = 0; i < GameState->CharacterModel->MeshCount; i++)
    {
      int32_t TextureId = i % GameState->TextureCount;
      glBindTexture(GL_TEXTURE_2D, GameState->Textures[TextureId]);
      glBindVertexArray(GameState->CharacterModel->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->CharacterModel->Meshes[i]->IndiceCount,
                     GL_UNSIGNED_INT, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
  }
  if(GameState->DrawGizmos && GameState->AnimEditor.Skeleton)
  {
    mat4        BoneGizmos[SKELETON_MAX_BONE_COUNT];
    const float BoneSphereRadius = 0.1f;
    for(int i = 0; i < GameState->AnimEditor.Skeleton->BoneCount; i++)
    {
      BoneGizmos[i] =
        Math::MulMat4(ModelMatrix,
                      Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices[i],
                                    GameState->AnimEditor.Skeleton->Bones[i].BindPose));
      if(Input->MouseRight.EndedDown && Input->MouseRight.Changed)
      {
        vec3 Position = Math::GetMat4Translation(BoneGizmos[i]);
        vec3 RayDir =
          GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                               GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
        raycast_result RaycastResult =
          RayIntersectSphere(GameState->Camera.Position, RayDir, Position, BoneSphereRadius);
        if(RaycastResult.Success)
        {
          EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, i);
        }
      }
    }

    for(int i = 0; i < GameState->AnimEditor.Skeleton->BoneCount; i++)
    {
      vec3 Position = Math::GetMat4Translation(BoneGizmos[i]);
      if(GameState->DrawWireframe)
      {
        DEBUGPushWireframeSphere(&GameState->Camera, Position, BoneSphereRadius);
      }
      if(GameState->DrawGizmos)
      {
        DEBUGPushGizmo(&GameState->Camera, &BoneGizmos[GameState->AnimEditor.CurrentBone]);
      }
    }
  }

  DEBUGDrawWireframeSpheres(GameState);
  glClear(GL_DEPTH_BUFFER_BIT);
  DEBUGDrawGizmos(GameState);

  if(Input->IsMouseInEditorMode)
  {
    // ANIMATION TIMELINE
    if(GameState->DrawTimeline && GameState->AnimEditor.Skeleton)
    {
      const int KeyframeCount = GameState->AnimEditor.KeyframeCount;
      if(KeyframeCount > 0)
      {
        const EditAnimation::animation_editor* Editor = &GameState->AnimEditor;

        const float TimelineStartX = 0.20f;
        const float TimelineStartY = 0.1f;
        const float TimelineWidth  = 0.6f;
        const float TimelineHeight = 0.05f;
        const float FirstTime      = MinFloat(Editor->PlayHeadTime, Editor->SampleTimes[0]);
        const float LastTime =
          MaxFloat(Editor->PlayHeadTime, Editor->SampleTimes[Editor->KeyframeCount - 1]);
        float KeyframeSpacing = KEYFRAME_MIN_TIME_DIFFERENCE_APART * 0.5f; // seconds

        float TimeDiff = MaxFloat((LastTime - FirstTime), 1.0f);

        float KeyframeWidth = KeyframeSpacing / TimeDiff;

        DEBUGDrawQuad(GameState, vec3{ TimelineStartX, TimelineStartY }, TimelineWidth,
                      TimelineHeight);
        for(int i = 0; i < KeyframeCount; i++)
        {
          float PosPercentage = (Editor->SampleTimes[i] - FirstTime) / TimeDiff;
          DEBUGDrawCenteredQuad(GameState,
                                vec3{ TimelineStartX + PosPercentage * TimelineWidth,
                                      TimelineStartY + TimelineHeight * 0.5f },
                                KeyframeWidth, 0.05f, { 0.5f, 0.3f, 0.3f });
        }
        float PlayheadPercentage = (Editor->PlayHeadTime - FirstTime) / TimeDiff;
        DEBUGDrawCenteredQuad(GameState,
                              vec3{ TimelineStartX + PlayheadPercentage * TimelineWidth,
                                    TimelineStartY + TimelineHeight * 0.5f },
                              KeyframeWidth, 0.05f, { 1, 0, 0 });
        float CurrentPercentage =
          (Editor->SampleTimes[Editor->CurrentKeyframe] - FirstTime) / TimeDiff;
        DEBUGDrawCenteredQuad(GameState,
                              vec3{ TimelineStartX + CurrentPercentage * TimelineWidth,
                                    TimelineStartY + TimelineHeight * 0.5f },
                              0.003f, 0.05f, { 0.5f, 0.5f, 0.5f });
      }
    }
    // GUI
    DrawAndInteractWithEditorUI(GameState, Input);
  }
}
