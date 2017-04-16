#include "game.h"
#include "ui.h"
#include "debug_drawing.h"

void
DrawAndInteractWithEditorUI(game_state* GameState, const game_input* Input)
{
  // Humble beginnings of the editor GUI system
  const float   TEXT_HEIGHT    = 0.03f;
  const float   StartX         = 0.7f;
  const float   StartY         = 0.9f;
  const float   YPadding       = 0.02f;
  const float   AspectRatio    = ((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
  const float   LayoutWidth    = 0.2f;
  const float   RowHeight      = 0.05f;
  const float   SliderWidth    = 0.05f;
  const int32_t ScrollRowCount = 2;

  static int32_t g_TotalRowCount          = 4;
  static bool    g_ShowDisplaySet         = false;
  static bool    g_ShowTransformSettign   = false;
  static bool    g_ShowTranslationButtons = false;
  static bool    g_ShowEntityDrowpown     = false;
  static bool    g_ShowAnimSetings        = false;
  static bool    g_ShowScrollSection      = false;
  static float   g_ScrollK                = 0.0f;

  UI::im_layout Layout =
    UI::NewLayout(StartX, StartY, LayoutWidth, RowHeight, AspectRatio, SliderWidth);

  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Rendering", &g_ShowDisplaySet))
  {
#if 0
    if(UI::_HoldButton(&Layout, Input, "PlayHead CW"))
    {
      // GetSelectedEntity(GameState)->Transform.Rotation.Y -= 110.0f * Input->dt;
    }
    if(UI::_HoldButton(&Layout, Input, "Rotate CCW"))
    {
      // GetSelectedEntity(GameState)->Transform.Rotation.Y += 110.0f * Input->dt;
    }
#else
    entity* SelectedEntity = {};
    if(GetSelectedEntity(GameState, &SelectedEntity))
    {
      UI::Row(&Layout);
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &SelectedEntity->Transform.Rotation.Y,
                      0, 360.0f, 720.0f);
    }
#endif
    UI::_Row(&Layout, 6, "Toggleables");
    UI::_BoolButton(&Layout, Input, "Toggle Timeline", &GameState->DrawTimeline);
    UI::_BoolButton(&Layout, Input, "Toggle Cubemap", &GameState->DrawCubemap);
    UI::_BoolButton(&Layout, Input, "Toggle Wireframe", &GameState->DrawWireframe);
    UI::_BoolButton(&Layout, Input, "Toggle Gizmos", &GameState->DrawGizmos);
    UI::_BoolButton(&Layout, Input, "Toggle BWeights", &GameState->DrawBoneWeights);
    UI::_BoolButton(&Layout, Input, "Toggle Spinning", &GameState->IsModelSpinning);
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Transform", &g_ShowTranslationButtons))
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
    UI::_Row(&Layout, 2, "X rotation");
    if(UI::_HoldButton(&Layout, Input, "PlayHead CW"))
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.X -= GameState->EditorBoneRotationSpeed * Input->dt;
    }
    if(UI::_HoldButton(&Layout, Input, "Rotate CCW"))
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.X += GameState->EditorBoneRotationSpeed * Input->dt;
    }
    UI::_Row(&Layout, 2, "Y rotation");
    if(UI::_HoldButton(&Layout, Input, "Rotate CW"))
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.Y -= GameState->EditorBoneRotationSpeed * Input->dt;
    }
    if(UI::_HoldButton(&Layout, Input, "Rotate CCW"))
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.Y += GameState->EditorBoneRotationSpeed * Input->dt;
    }
    UI::_Row(&Layout, 2, "Z rotation");
    if(UI::_HoldButton(&Layout, Input, "Rotate CW"))
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.Z -= GameState->EditorBoneRotationSpeed * Input->dt;
    }
    if(UI::_HoldButton(&Layout, Input, "Rotate CCW"))
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.Z += GameState->EditorBoneRotationSpeed * Input->dt;
    }
    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Position", &g_ShowTransformSettign))
    {
      UI::_Row(&Layout, 2, "X Position");
      if(UI::_HoldButton(&Layout, Input, "-"))
      {
        GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
          .Transforms[GameState->AnimEditor.CurrentBone]
          .Translation.X -= Input->dt;
      }
      if(UI::_HoldButton(&Layout, Input, "+"))
      {
        GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
          .Transforms[GameState->AnimEditor.CurrentBone]
          .Translation.X += Input->dt;
      }
      UI::_Row(&Layout, 2, "Y Position");
      if(UI::_HoldButton(&Layout, Input, "-"))
      {
        GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
          .Transforms[GameState->AnimEditor.CurrentBone]
          .Translation.Y -= Input->dt;
      }
      if(UI::_HoldButton(&Layout, Input, "  +  "))
      {
        GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
          .Transforms[GameState->AnimEditor.CurrentBone]
          .Translation.Y += Input->dt;
      }
      UI::_Row(&Layout, 2, "Z Position");
      if(UI::_HoldButton(&Layout, Input, "-"))
      {
        GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
          .Transforms[GameState->AnimEditor.CurrentBone]
          .Translation.Z -= Input->dt;
      }
      if(UI::_HoldButton(&Layout, Input, "+"))
      {
        GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
          .Transforms[GameState->AnimEditor.CurrentBone]
          .Translation.Z += Input->dt;
      }
    }
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Entity Creation", &g_ShowEntityDrowpown))
  {
    UI::Row(&Layout, 2);
    if(UI::_PushButton(&Layout, Input, "Previous Model"))
    {
      if(GameState->R.ModelCount > 0)
      {
        GameState->CurrentModel =
          (GameState->CurrentModel + GameState->R.ModelCount - 1) % GameState->R.ModelCount;
      }
    }
    if(UI::_PushButton(&Layout, Input, "Next Model"))
    {
      if(GameState->R.ModelCount > 0)
      {
        GameState->CurrentModel = (GameState->CurrentModel + 1) % GameState->R.ModelCount;
      }
    }
    UI::Row(&Layout);
    if(UI::_PushButton(&Layout, Input, "Create Entity"))
    {
      GameState->IsEntityCreationMode = !GameState->IsEntityCreationMode;
    }

    UI::DrawSquareQuad(GameState, &Layout, GameState->IDTexture);
    UI::Row(&Layout, 2);
    if(UI::_PushButton(&Layout, Input, "Previous Material"))
    {
      if(GameState->R.MaterialCount > 0)
      {
        GameState->CurrentMaterial = (GameState->CurrentMaterial + GameState->R.MaterialCount - 1) %
                                     GameState->R.MaterialCount;
      }
    }
    if(UI::_PushButton(&Layout, Input, "Next Material"))
    {
      if(GameState->R.MaterialCount > 0)
      {
        GameState->CurrentMaterial = (GameState->CurrentMaterial + 1) % GameState->R.MaterialCount;
      }
    }
    UI::Row(&Layout);
    if(UI::_PushButton(&Layout, Input, "Set maerial"))
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
    UI::Row(&Layout);
    UI::_BoolButton(&Layout, Input, "Play Animation", &GameState->IsAnimationPlaying);
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
