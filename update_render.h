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
#include "load_bmp.h"
#include "misc.h"
#include "ui.h"

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
static const float EDITOR_BONE_ROTATION_SPEED_DEG = 45.0f;

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

    // -------BEGIN ASSETS
    Memory::stack_allocator* TemporaryMemStack  = GameState->TemporaryMemStack;
    Memory::stack_allocator* PersistentMemStack = GameState->PersistentMemStack;

    // Set Up Gizmo
    debug_read_file_result AssetReadResult =
      ReadEntireFile(PersistentMemStack, "./data/gizmo.model");

    assert(AssetReadResult.Contents);
    Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

    UnpackAsset(AssetHeader);
    GameState->GizmoModel = (Render::model*)AssetHeader->Model;
    for(int i = 0; i < GameState->GizmoModel->MeshCount; i++)
    {
      SetUpMesh(GameState->GizmoModel->Meshes[i]);
    }

    // Set Up Gizmo
    AssetReadResult = ReadEntireFile(PersistentMemStack, "./data/debug_meshes.model");

    assert(AssetReadResult.Contents);
    AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

    UnpackAsset(AssetHeader);
    GameState->QuadModel = (Render::model*)AssetHeader->Model;
    for(int i = 0; i < GameState->QuadModel->MeshCount; i++)
    {
      SetUpMesh(GameState->QuadModel->Meshes[i]);
    }

// Set Up Model
#if demo
    AssetReadResult = ReadEntireFile(PersistentMemStack, "./data/soldier_test0.actor");
#else
    AssetReadResult = ReadEntireFile(PersistentMemStack, "./data/crysis_soldier.actor");
#endif

    assert(AssetReadResult.Contents);
    AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;
    UnpackAsset(AssetHeader);
    GameState->CharacterModel = (Render::model*)AssetHeader->Model;
    for(int i = 0; i < GameState->CharacterModel->MeshCount; i++)
    {
      SetUpMesh(GameState->CharacterModel->Meshes[i]);
    }
    Render::PrintModelHeader(GameState->CharacterModel);

    GameState->Skeleton            = (Anim::skeleton*)AssetHeader->Skeleton;
    GameState->AnimEditor.Skeleton = (Anim::skeleton*)AssetHeader->Skeleton;

    // Set Up Cubemap
    AssetReadResult = ReadEntireFile(PersistentMemStack, "./data/cube.model");
    assert(AssetReadResult.Contents);
    AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;
    UnpackAsset(AssetHeader);
    GameState->Cubemap = (Render::model*)AssetHeader->Model;
    for(int i = 0; i < GameState->Cubemap->MeshCount; i++)
    {
      SetUpMesh(GameState->Cubemap->Meshes[i]);
    }

    SDL_Color Color;
    Color.a = 255;
    Color.r = 255;
    Color.g = 255;
    Color.b = 255;
    GameState->TextTexture =
      Texture::LoadTextTexture("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
                               32, "Testing Text Texture", Color);
    GameState->CubemapTexture = Texture::LoadCubemap(TemporaryMemStack, "./data/iceflats", "tga");
// Set Up Texture
#if demo
    AddTexture(GameState, Texture::LoadModelTexture("./data/hand_dif.png"));
    AddTexture(GameState, Texture::LoadModelTexture("./data/helmet_diff.png"));
    AddTexture(GameState, Texture::LoadModelTexture("./data/glass_dif.png"));
    AddTexture(GameState, Texture::LoadModelTexture("./data/body_dif.png"));
    AddTexture(GameState, Texture::LoadModelTexture("./data/leg_dif.png"));
    AddTexture(GameState, Texture::LoadModelTexture("./data/arm_dif.png"));
#else
    AddTexture(GameState, Texture::LoadModelTexture("./data/body_dif.png"));
#endif
    GameState->CollapsedTexture = Texture::LoadBMPTexture("./data/collapsed.bmp");
    GameState->ExpandedTexture  = Texture::LoadBMPTexture("./data/expanded.bmp");
    assert(GameState->CollapsedTexture);
    assert(GameState->ExpandedTexture);

    // -------BEGIN LOADING SHADERS
    // Diffuse
    Memory::marker LoadStart = TemporaryMemStack->GetMarker();
    GameState->ShaderSkeletalPhong =
      Shader::LoadShader(TemporaryMemStack, "./shaders/skeletal_phong");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderSkeletalPhong < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }
    // Bone Color
    LoadStart = TemporaryMemStack->GetMarker();
    GameState->ShaderSkeletalBoneColor =
      Shader::LoadShader(TemporaryMemStack, "./shaders/skeletal_bone_color");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderSkeletalBoneColor < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }
    // Cubemap
    LoadStart                = TemporaryMemStack->GetMarker();
    GameState->ShaderCubemap = Shader::LoadShader(TemporaryMemStack, "./shaders/cubemap");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderCubemap < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    // Wireframe
    LoadStart                  = TemporaryMemStack->GetMarker();
    GameState->ShaderWireframe = Shader::LoadShader(TemporaryMemStack, "./shaders/wireframe");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderWireframe < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    LoadStart              = TemporaryMemStack->GetMarker();
    GameState->ShaderGizmo = Shader::LoadShader(TemporaryMemStack, "./shaders/gizmo");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderGizmo < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }
    //
    // debug_ui
    LoadStart             = TemporaryMemStack->GetMarker();
    GameState->ShaderQuad = Shader::LoadShader(TemporaryMemStack, "./shaders/debug_quad");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderQuad < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }
    LoadStart = TemporaryMemStack->GetMarker();
    GameState->ShaderTexturedQuad =
      Shader::LoadShader(TemporaryMemStack, "./shaders/debug_textured_quad");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderTexturedQuad < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    // -------END ASSET LOADING
    // ======Set GL state
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // -------InitGameState
    GameState->IsModelSpinning         = true;
    GameState->ModelTransform.Rotation = { 0, 0, 0 };
    GameState->ModelTransform.Scale    = { 1, 1, 1 };
    GameState->Camera.Position         = { 0, 1.3f, 1.3f };
    GameState->Camera.Up               = { 0, 1, 0 };
    GameState->Camera.Forward          = { 0, 0, -1 };
    GameState->Camera.Right            = { 1, 0, 0 };
    GameState->Camera.Rotation         = { -20 };
    GameState->Camera.FieldOfView      = 70.0f;
    GameState->Camera.NearClipPlane    = 0.001f;
    GameState->Camera.FarClipPlane     = 100.0f;
    GameState->Camera.MaxTiltAngle     = 90.0f;

    GameState->LightPosition    = { 2.25f, 1.0f, 1.0f };
    GameState->LightColor       = { 0.7f, 0.7f, 0.75f };
    GameState->AmbientStrength  = 0.7f;
    GameState->SpecularStrength = 0.6f;

    GameState->DrawWireframe   = false;
    GameState->DrawBoneWeights = false;
    GameState->DrawGizmos      = false;
    GameState->DisplayText     = false;

    EditAnimation::InsertBlendedKeyframeAtTime(&GameState->AnimEditor,
                                               GameState->AnimEditor.PlayHeadTime);
  }
  //---------------------END INIT -------------------------

  //----------------------UPDATE------------------------
  GameState->GameTime += Input->dt;

  UpdateCamera(&GameState->Camera, Input);
  if(GameState->IsModelSpinning)
  {
    GameState->ModelTransform.Rotation.Y += 45.0f * Input->dt;
  }
  mat4 ModelMatrix = Math::MulMat4(Math::Mat4Rotate(GameState->ModelTransform.Rotation),
                                   Math::Mat4Scale(GameState->ModelTransform.Scale));
  mat4 MVPMatrix = Math::MulMat4(GameState->Camera.VPMatrix, ModelMatrix);
  //--------------ANIMAITION UPDATE

  if(Input->i.EndedDown && Input->i.Changed)
  {
    InsertBlendedKeyframeAtTime(&GameState->AnimEditor, GameState->AnimEditor.PlayHeadTime);
  }
  if(Input->LeftShift.EndedDown)
  {
    if(Input->x.EndedDown)
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.X -= EDITOR_BONE_ROTATION_SPEED_DEG * Input->dt;
    }
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
    if(Input->x.EndedDown)
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.X += EDITOR_BONE_ROTATION_SPEED_DEG * Input->dt;
    }
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
  if(Input->m.EndedDown && Input->m.Changed)
  {
    EditAnimation::MoveCurrentKeyframeToPlayHead(&GameState->AnimEditor);
  }
  if(Input->Delete.EndedDown && Input->Delete.Changed)
  {
    EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
  }

  PrintAnimEditorState(&GameState->AnimEditor);

  if(GameState->AnimEditor.KeyframeCount > 0)
  {
    EditAnimation::CalculateHierarchicalmatricesAtTime(&GameState->AnimEditor);
  }

#if 1
#endif

  //---------------------RENDERING----------------------------
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

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "g_bone_matrices"),
                       20, GL_FALSE, (float*)GameState->AnimEditor.HierarchicalModelSpaceMatrices);
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
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "g_bone_matrices"), 20,
                       GL_FALSE, (float*)GameState->AnimEditor.HierarchicalModelSpaceMatrices);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "mat_model"), 1,
                       GL_FALSE, ModelMatrix.e);
    glUniform1f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "ambient_strength"),
                GameState->AmbientStrength);
    glUniform1f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "specular_strength"),
                GameState->SpecularStrength);
    glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "light_position"),
                GameState->LightPosition.X, GameState->LightPosition.Y, GameState->LightPosition.Z);
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
  if(GameState->DisplayText)
  {
    DEBUGDrawTexturedQuad(GameState, GameState->TextTexture, vec3{ 0.0f, 0.0f, 0.4f }, 0.3f, 0.05f);
  }
  if(GameState->DrawWireframe)
  {
    // WireframeShader Shader
    glUseProgram(GameState->ShaderWireframe);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderWireframe, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    for(int i = 0; i < GameState->CharacterModel->MeshCount; i++)
    {
      glBindVertexArray(GameState->CharacterModel->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->CharacterModel->Meshes[i]->IndiceCount,
                     GL_UNSIGNED_INT, 0);
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
        Math::MulMat4(ModelMatrix,
                      Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices[i],
                                    GameState->Skeleton->Bones[i].BindPose));
    }

    DEBUGDrawGizmos(GameState, &ModelMatrix, 1);
    DEBUGDrawGizmos(GameState, BoneGizmos, GameState->Skeleton->BoneCount);
  }

  glDepthFunc(GL_LEQUAL);
  glUseProgram(GameState->ShaderCubemap);
  glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderCubemap, "mat_projection"), 1, GL_FALSE,
                     GameState->Camera.ProjectionMatrix.e);
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

  // Drawing animation editor timeline
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
  if(Input->IsMouseInEditorMode)
  {
    // Humble beginnings of the UI system
    const float   TEXT_HEIGHT    = 0.03f;
    const float   StartX         = 0.6f;
    const float   StartY         = 0.9f;
    const float   YPadding       = 0.02f;
    const float   LayoutWidth    = 0.35f;
    const float   RowHeight      = 0.05f;
    const float   SliderWidth    = 0.05f;
    const int32_t ScrollRowCount = 4;

    static int32_t g_TotalRowCount     = 4;
    static bool    g_ShowDisplaySet    = false;
    static bool    g_ShowAnimSetings   = false;
    static bool    g_ShowScrollSection = false;
    static float   g_ScrollK           = 0.0f;

    UI::im_layout Layout = UI::NewLayout(StartX, StartY, LayoutWidth, RowHeight);

    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Display Settings", &g_ShowDisplaySet))
    {
      UI::Row(&Layout, 2);
      if(UI::_HoldButton(&Layout, Input, "PlayHead CW"))
      {
        GameState->ModelTransform.Rotation.Y -= 110.0f * Input->dt;
      }
      if(UI::_HoldButton(&Layout, Input, "Rotate CCW"))
      {
        GameState->ModelTransform.Rotation.Y += 110.0f * Input->dt;
      }
      UI::_Row(&Layout, 5, "Toggleables");
      UI::_BoolButton(&Layout, Input, "Toggle Text Display", &GameState->DisplayText);
      UI::_BoolButton(&Layout, Input, "Toggle Wireframe", &GameState->DrawWireframe);
      UI::_BoolButton(&Layout, Input, "Toggle Gizmos", &GameState->DrawGizmos);
      UI::_BoolButton(&Layout, Input, "Toggle BWeights", &GameState->DrawBoneWeights);
      UI::_BoolButton(&Layout, Input, "Toggle Spinning", &GameState->IsModelSpinning);
    }
    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Animation Settings", &g_ShowAnimSetings))
    {
      UI::Row(&Layout);
      if(UI::PushButton(GameState, &Layout, Input, "Delete keyframe", { 0.6f, 0.4f, 0.4f, 1.0f }))
      {
        EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
      }
      UI::Row(&Layout);
      if(UI::_PushButton(&Layout, Input, "Insert keyframe"))
      {
        EditAnimation::InsertBlendedKeyframeAtTime(&GameState->AnimEditor,
                                                   GameState->AnimEditor.PlayHeadTime);
      }
      UI::Row(&Layout, 2);
      if(UI::_HoldButton(&Layout, Input, "PlayHead Left"))
      {
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, -1 * Input->dt);
      }
      if(UI::_HoldButton(&Layout, Input, "PlayHead Right"))
      {
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, +1 * Input->dt);
      }
    }
    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Scrollbar Section", &g_ShowScrollSection))
    {
      UI::Row(&Layout);
      if(UI::_PushButton(&Layout, Input, "Add Entry"))
      {
        ++g_TotalRowCount;
      }
      UI::Row(&Layout);
      if(UI::PushButton(GameState, &Layout, Input, "Delete Last Entry", { 0.6f, 0.4f, 0.4f, 1.0f }))
      {
        --g_TotalRowCount;
      }
      UI::Row(&Layout, 2);
      if(UI::_HoldButton(&Layout, Input, "Scroll Up"))
      {
        g_ScrollK -= 0.5f * Input->dt;
      }
      if(UI::_HoldButton(&Layout, Input, "Scroll Down"))
      {
        g_ScrollK += 0.5f * Input->dt;
      }
      g_ScrollK = ClampFloat(0.0f, g_ScrollK, 1.0f);
      int StartRow =
        UI::_BeginScrollableList(&Layout, Input, g_TotalRowCount, ScrollRowCount, g_ScrollK);
      {
        for(int i = StartRow; i < StartRow + ScrollRowCount; i++)
        {
          UI::Row(&Layout);
          g_TotalRowCount      = ClampMinInt32(0, g_TotalRowCount);
          float RowCoefficient = (float)i / (float)g_TotalRowCount;
          UI::PushButton(GameState, &Layout, Input, "Name",
                         { 0.8f + 0.2f * cosf((3 + RowCoefficient) * 315),
                           0.8f + 0.2f * cosf((3 + RowCoefficient) * 250),
                           0.8f + 0.2f * cosf((3 + RowCoefficient) * 480), 1.0f });
        }
      }
      UI::EndScrollableList(&Layout);
    }
  }
}
