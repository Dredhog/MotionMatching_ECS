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
  static bool    g_ShowAnimSetings        = false;
  static bool    g_ShowScrollSection      = false;
  static float   g_ScrollK                = 0.0f;

  UI::im_layout Layout =
    UI::NewLayout(StartX, StartY, LayoutWidth, RowHeight, AspectRatio, SliderWidth);

  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Rendering", &g_ShowDisplaySet))
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
      if(UI::_HoldButton(&Layout, Input, "+"))
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
