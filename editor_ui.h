#include "game.h"
#include "ui.h"
#include "debug_drawing.h"
#include "render_data.h"

#include <limits>

void
DrawAndInteractWithEditorUI(game_state* GameState, const game_input* Input)
{
  // Humble beginnings of the editor GUI system
  const float   TEXT_HEIGHT    = 0.03f;
  const float   StartX         = 0.75f;
  const float   StartY         = 0.95f;
  const float   YPadding       = 0.02f;
  const float   AspectRatio    = ((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
  const float   LayoutWidth    = 0.15f;
  const float   RowHeight      = 0.04f;
  const float   SliderWidth    = 0.05f;
  const int32_t ScrollRowCount = 2;

  static bool  g_ShowDisplaySet         = false;
  static bool  g_ShowTransformSettign   = false;
  static bool  g_ShowTranslationButtons = false;
  static bool  g_ShowEntityDrowpown     = false;
  static bool  g_ShowAnimSetings        = false;
  static bool  g_ShowScrollSection      = false;
  static bool  g_ShowMaterialEditor     = false;
  static bool  g_ShowCameraSettings     = false;
  static float g_ScrollK                = 0.0f;

  UI::im_layout Layout =
    UI::NewLayout(StartX, StartY, LayoutWidth, RowHeight, AspectRatio, SliderWidth);

  // UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Render Settings", &g_ShowDisplaySet))
  {
    UI::_Row(&Layout, 6, "Toggleables");
    UI::_BoolButton(&Layout, Input, "Toggle Timeline", &GameState->DrawTimeline);
    UI::_BoolButton(&Layout, Input, "Toggle Cubemap", &GameState->DrawCubemap);
    UI::_BoolButton(&Layout, Input, "Toggle Wireframe", &GameState->DrawWireframe);
    UI::_BoolButton(&Layout, Input, "Toggle Gizmos", &GameState->DrawGizmos);
    UI::_BoolButton(&Layout, Input, "Toggle BWeights", &GameState->DrawBoneWeights);
    UI::_BoolButton(&Layout, Input, "Toggle Spinning", &GameState->IsModelSpinning);
  }
  entity* SelectedEntity = {};
  if(GetSelectedEntity(GameState, &SelectedEntity))
  {
    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Transform", &g_ShowTranslationButtons))
    {

      Anim::transform* Transform =
        &GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
           .Transforms[GameState->AnimEditor.CurrentBone];
      UI::_Row(&Layout, 3, "Translation");
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Translation.X, -INFINITY,
                      INFINITY, 20.0f);
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Translation.Y, -INFINITY,
                      INFINITY, 20.0f);
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Translation.Z, -INFINITY,
                      INFINITY, 20.0f);
      UI::_Row(&Layout, 3, "Rotation");
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Rotation.X, -360, 360.0f,
                      720.0f);
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Rotation.Y, -360, 360.0f,
                      720.0f);
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Rotation.Z, -360, 360.0f,
                      720.0f);
      UI::_Row(&Layout, 3, "Scale  ");
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Scale.X, 0, 100,
                      10.0f * ClampFloat(0.01f, AbsFloat(Transform->Scale.X), 10));
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Scale.Y, 0, 100,
                      10.0f * ClampFloat(0.01f, AbsFloat(Transform->Scale.X), 10));
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Scale.Z, 0, 100,
                      10.0f * ClampFloat(0.01f, AbsFloat(Transform->Scale.X), 10));
    }
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Camera     ", &g_ShowCameraSettings))
  {
    UI::_Row(&Layout, 1, "FOV    ");
    UI::SliderFloat(GameState, &Layout, Input, "FieldOfView", &GameState->Camera.FieldOfView, 0,
                    200.0f, 50.0f);
    UI::_Row(&Layout, 1, "Near  ");
    UI::SliderFloat(GameState, &Layout, Input, "Near Clip Plane", &GameState->Camera.NearClipPlane,
                    0.001f, 1000, 50.0f);
    UI::_Row(&Layout, 1, "Far   ");
    UI::SliderFloat(GameState, &Layout, Input, "Far Clip Plane", &GameState->Camera.FarClipPlane,
                    GameState->Camera.NearClipPlane, 1000, 50.0f);
    UI::_Row(&Layout, 1, "Speed");
    UI::SliderFloat(GameState, &Layout, Input, "Far Clip Plane", &GameState->Camera.Speed, 0, 200,
                    50.0f);
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Entity Creation", &g_ShowEntityDrowpown))
  {
    UI::Row(&Layout, 2);
    if(UI::_PushButton(&Layout, Input, "Previous Model"))
    {
      if(0 < GameState->CurrentModel)
      {
        --GameState->CurrentModel;
      }
    }
    if(UI::_PushButton(&Layout, Input, "Next Model"))
    {
      if(GameState->CurrentModel < GameState->R.ModelCount - 1)
      {
        ++GameState->CurrentModel;
      }
    }
    UI::Row(&Layout);
    if(UI::_PushButton(&Layout, Input, "Create Entity"))
    {
      GameState->IsEntityCreationMode = !GameState->IsEntityCreationMode;
    }
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Material Editor", &g_ShowMaterialEditor))
  {
    UI::DrawSquareQuad(GameState, &Layout, GameState->IDTexture);
    UI::_Row(&Layout, 2, "Material");
    if(UI::_PushButton(&Layout, Input, "Previous"))
    {
      if(0 < GameState->CurrentMaterial)
      {
        --GameState->CurrentMaterial;
      }
    }
    if(UI::_PushButton(&Layout, Input, "  Next  "))
    {
      if(GameState->CurrentMaterial < GameState->R.MaterialCount - 1)
      {
        ++GameState->CurrentMaterial;
      }
    }

    assert(GameState->R.MaterialCount > 0);
    material* CurrentMaterial = &GameState->R.Materials[GameState->CurrentMaterial];

    UI::Row(&Layout);
    if(UI::_PushButton(&Layout, Input, "Apply To Selected"))
    {
      if(GameState->R.ModelCount > 0)
      {
        if(0 <= GameState->SelectedEntityIndex &&
           GameState->SelectedEntityIndex < GameState->EntityCount)
        {
          entity* Entity = &GameState->Entities[GameState->SelectedEntityIndex];
          if(0 <= GameState->SelectedMeshIndex &&
             GameState->SelectedMeshIndex < Entity->Model->MeshCount)
          {
            if(0 <= GameState->CurrentMaterial &&
               GameState->CurrentMaterial < GameState->R.MaterialCount)
            {
              Entity->MaterialIndices[GameState->SelectedMeshIndex] = GameState->CurrentMaterial;
            }
          }
        }
      }
    }

    UI::Row(&Layout);
    if(UI::_PushButton(&Layout, Input, "Reset Material"))
    {
      uint32_t ShaderType                 = CurrentMaterial->Common.ShaderType;
      *CurrentMaterial                    = {};
      CurrentMaterial->Common.ShaderType = ShaderType;
    }
    UI::_Row(&Layout, 2, "Shader Type");
    if(UI::_PushButton(&Layout, Input, "Previous"))
    {
      if(CurrentMaterial->Common.ShaderType > 0)
      {
        --CurrentMaterial->Common.ShaderType;
      }
    }
    if(UI::_PushButton(&Layout, Input, "  Next  "))
    {
      CurrentMaterial->Common.ShaderType =
        ClampInt32InIn(0, CurrentMaterial->Common.ShaderType + 1, SHADER_EnumCount - 1);
    }

    material* Material = &GameState->R.Materials[GameState->CurrentMaterial];
    UI::Row(&Layout);
    UI::_BoolButton(&Layout, Input, "Enable Blending", &CurrentMaterial->Common.UseBlending);

    switch(Material->Common.ShaderType)
    {
      case SHADER_Phong:
      {
        UI::_Row(&Layout, 2, "Albedo");
        UI::DrawTextBox(GameState, &Layout, "Number", { 0.4f, 0.4f, 0.4f, 1 });
        if(UI::_PushButton(&Layout, Input, "Prev"))
        {
          if(CurrentMaterial->Phong.TextureIndex0 > 0)
          {
            --CurrentMaterial->Phong.TextureIndex0;
          }
        }
        if(UI::_PushButton(&Layout, Input, "Next  "))
        {
          CurrentMaterial->Phong.TextureIndex0 =
            (CurrentMaterial->Phong.TextureIndex0 + 1) % GameState->R.TextureCount;
        }
        UI::_Row(&Layout, 1, "Ambient");
        UI::SliderFloat(GameState, &Layout, Input, "Amb", &Material->Phong.AmbientStrength, 0, 1.0f,
                        5.0f);
        UI::_Row(&Layout, 1, "Specular");
        UI::SliderFloat(GameState, &Layout, Input, "Dif", &Material->Phong.SpecularStrength, 0,
                        1.0f, 5.0f);
      }
      break;
      case SHADER_Color:
      {
        UI::_Row(&Layout, 4, "Color");
        UI::SliderFloat(GameState, &Layout, Input, "R", &Material->Color.Color.R, 0, 1.0f, 5.0f);
        UI::SliderFloat(GameState, &Layout, Input, "G", &Material->Color.Color.G, 0, 1.0f, 5.0f);
        UI::SliderFloat(GameState, &Layout, Input, "B", &Material->Color.Color.B, 0, 1.0f, 5.0f);
        UI::SliderFloat(GameState, &Layout, Input, "A", &Material->Color.Color.A, 0, 1.0f, 5.0f);
      }
      break;
    }
    UI::Row(&Layout);
    if(UI::_PushButton(&Layout, Input, "Create New"))
    {
      AddMaterial(&GameState->R, {});
      GameState->CurrentMaterial = GameState->R.MaterialCount - 1;
    }
    UI::Row(&Layout);
    if(UI::_PushButton(&Layout, Input, "Crete From Current"))
    {
      AddMaterial(&GameState->R, *CurrentMaterial);
      GameState->CurrentMaterial = GameState->R.MaterialCount - 1;
    }
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Animation Settings", &g_ShowAnimSetings))
  {
    UI::Row(&Layout, 2);
    if(UI::_PushButton(&Layout, Input, "Previous Bone"))
    {
      EditAnimation::EditPreviousBone(&GameState->AnimEditor);
    }
    if(UI::_PushButton(&Layout, Input, "Next Bone"))
    {
      EditAnimation::EditNextBone(&GameState->AnimEditor);
    }
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
    UI::Row(&Layout);
    UI::_BoolButton(&Layout, Input, "Play Animation", &GameState->IsAnimationPlaying);
  }
#if 0
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
#endif
}

void
VisualizeTimeline(game_state* GameState)
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
    float TimeDiff        = MaxFloat((LastTime - FirstTime), 1.0f);
    float KeyframeWidth   = KeyframeSpacing / TimeDiff;

    DEBUGDrawQuad(GameState, vec3{ TimelineStartX, TimelineStartY }, TimelineWidth, TimelineHeight);
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
    float CurrentPercentage = (Editor->SampleTimes[Editor->CurrentKeyframe] - FirstTime) / TimeDiff;
    DEBUGDrawCenteredQuad(GameState,
                          vec3{ TimelineStartX + CurrentPercentage * TimelineWidth,
                                TimelineStartY + TimelineHeight * 0.5f },
                          0.003f, 0.05f, { 0.5f, 0.5f, 0.5f });
  }
}
