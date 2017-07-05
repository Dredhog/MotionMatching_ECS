#include "ui.h"

void EntityGUI(game_state* GameState);

namespace UI
{
  void
  TestGui(game_state* GameState, const game_input* Input)
  {
    UI::BeginFrame(GameState, Input);

    static int         s_CurrentItem = -1;
    static const char* s_Items[]     = { "Cat", "Rat", "Hat", "Pat", "meet", "with", "dad" };
    UI::BeginWindow("window A", { 800, 300 }, { 500, 400 });
    {
      static bool s_HeaderExpanded = true;
      if(UI::CollapsingHeader("Demo", &s_HeaderExpanded))
      {
        static bool s_Checkbox0 = false;
        static bool s_Checkbox1 = false;

        {
          gui_style& Style     = *UI::GetStyle();
          int32_t    Thickness = (int32_t)Style.Vars[UI::VAR_BorderThickness];
          UI::SliderInt("Border Thickness ", &Thickness, 0, 10);
          Style.Vars[UI::VAR_BorderThickness] = Thickness;

          // UI::DragFloat("Window Opacity", &Style.Colors[UI::COLOR_WindowBackground].X, 0, 1, 5);
					UI::Text("Hold ctrl when dragging to snap to whole values");
          UI::DragFloat4("Window background", (float*)&Style.Colors[UI::COLOR_WindowBackground], 0, 1, 5);
          UI::SliderFloat("Horizontal Padding", &Style.Vars[UI::VAR_BoxPaddingX], 0, 10);
          UI::SliderFloat("Vertical Padding", &Style.Vars[UI::VAR_BoxPaddingY], 0, 10);
          UI::SliderFloat("Horizontal Spacing", &Style.Vars[UI::VAR_SpacingX], 0, 10);
          UI::SliderFloat("Vertical Spacing", &Style.Vars[UI::VAR_SpacingY], 0, 10);
          UI::SliderFloat("Internal Spacing", &Style.Vars[UI::VAR_InternalSpacing], 0, 10);
        }
        UI::Combo("Combo test", &s_CurrentItem, s_Items, ARRAY_SIZE(s_Items), 5, 150);
        int StartIndex = 3;
        UI::Combo("Combo test1", &s_CurrentItem, s_Items + StartIndex, ARRAY_SIZE(s_Items) - StartIndex);
        UI::NewLine();

        char TempBuff[20];
        snprintf(TempBuff, sizeof(TempBuff), "Wheel %d", Input->MouseWheelScreen);
        UI::Text(TempBuff);

        UI::Checkbox("Show Image", &s_Checkbox0);
        if(s_Checkbox0)
        {
          UI::SameLine();
          UI::Checkbox("Put image in frame", &s_Checkbox1);
          UI::NewLine();
          if(s_Checkbox1)
          {
            UI::BeginChildWindow("Image frame", { 300, 170 });
          }

          UI::Image(GameState->IDTexture, "material preview", { 400, 220 });

          if(s_Checkbox1)
          {
            UI::EndChildWindow();
          }
        }
      }
    }
    UI::EndWindow();

    UI::BeginWindow("Window B", { 200, 300 }, { 550, 400 });
    {
      UI::Text("This is some text written with a special widget");
      EntityGUI(GameState);
    }

    UI::EndWindow();

    UI::EndFrame();
  }
}

char*
PathArrayToString(void* Data, int Index)
{
  path* Paths = (path*)Data;
  return Paths[Index].Name;
}

void
EntityGUI(game_state* GameState)
{
  static bool s_ShowEntityTools = true;
  if(UI::CollapsingHeader("Entity Tools", &s_ShowEntityTools))
  {
    static int32_t ActivePathIndex = 0;
    UI::Combo("Model", &ActivePathIndex, GameState->Resources.ModelPaths, GameState->Resources.ModelPathCount, PathArrayToString);
    {
      rid NewRID = { 0 };
      if(!GameState->Resources.GetModelPathRID(&NewRID, GameState->Resources.ModelPaths[ActivePathIndex].Name))
      {
        NewRID                    = GameState->Resources.RegisterModel(GameState->Resources.ModelPaths[ActivePathIndex].Name);
        GameState->CurrentModelID = NewRID;
      }
      else
      {
        GameState->CurrentModelID = NewRID;
      }
    }

    if(UI::Button("Create Entity"))
    {
      GameState->IsEntityCreationMode = !GameState->IsEntityCreationMode;
    }

    entity* SelectedEntity = {};
    if(GetSelectedEntity(GameState, &SelectedEntity))
    {
      if(UI::Button("Delete Entity"))
      {
        DeleteEntity(GameState, GameState->SelectedEntityIndex);
        GameState->SelectedEntityIndex = -1;
      }

      Anim::transform* Transform = &SelectedEntity->Transform;
      UI::DragFloat3("Translation", (float*)&Transform->Translation, -INFINITY, INFINITY, 10.0f);
      UI::DragFloat3("Rotation", (float*)&Transform->Rotation, -INFINITY, INFINITY, 720.0f);
      UI::DragFloat3("Scale", (float*)&Transform->Scale, -INFINITY, INFINITY, 10.0f);

      Render::model* SelectedModel = GameState->Resources.GetModel(SelectedEntity->ModelID);
      if(SelectedModel->Skeleton)
      {
        if(!SelectedEntity->AnimController && UI::Button("Add Anim. Controller"))
        {
          SelectedEntity->AnimController  = PushStruct(GameState->PersistentMemStack, Anim::animation_controller);
          *SelectedEntity->AnimController = {};

          SelectedEntity->AnimController->Skeleton           = SelectedModel->Skeleton;
          SelectedEntity->AnimController->OutputTransforms   = PushArray(GameState->PersistentMemStack, ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT * SelectedModel->Skeleton->BoneCount, Anim::transform);
          SelectedEntity->AnimController->BoneSpaceMatrices  = PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
          SelectedEntity->AnimController->ModelSpaceMatrices = PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
          SelectedEntity->AnimController->HierarchicalModelSpaceMatrices = PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
        }
        else if(SelectedEntity->AnimController && UI::Button("Delete Anim. Controller"))
        {
          SelectedEntity->AnimController = 0;
        }
        else if(SelectedEntity->AnimController)
        {
          if(UI::Button("Animate Selected Entity"))
          {
            GameState->SelectionMode = SELECT_Bone;
            AttachEntityToAnimEditor(GameState, &GameState->AnimEditor, GameState->SelectedEntityIndex);
            // g_ShowAnimationEditor = true;
          }
          if(UI::Button("Play Animation"))
          {
            if(GameState->CurrentAnimationID.Value > 0)
            {
              if(GameState->Resources.GetAnimation(GameState->CurrentAnimationID)->ChannelCount == SelectedModel->Skeleton->BoneCount)
              {
                if(SelectedEntity->AnimController->AnimStateCount == 0)
                {
                  SelectedEntity->AnimController->AnimationIDs[0] = GameState->CurrentAnimationID;
                  SelectedEntity->AnimController->AnimStateCount  = 1;
                }
                else
                {
                  SelectedEntity->AnimController->AnimationIDs[0] = GameState->CurrentAnimationID;
                }
                Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 0);
              }
              else if(SelectedEntity->AnimController->AnimStateCount != 0)
              {
                StopAnimation(SelectedEntity->AnimController, 0);
              }
            }
          }
          {
            static int32_t ActivePathIndex = 0;
            UI::Combo("Animation", &ActivePathIndex, GameState->Resources.AnimationPaths, GameState->Resources.AnimationPathCount, PathArrayToString);
            rid NewRID = { 0 };
            if(GameState->Resources.AnimationPathCount > 0 && !GameState->Resources.GetAnimationPathRID(&NewRID, GameState->Resources.AnimationPaths[ActivePathIndex].Name))
            {
              NewRID = GameState->Resources.RegisterAnimation(GameState->Resources.AnimationPaths[ActivePathIndex].Name);
            }
            GameState->CurrentAnimationID = NewRID;
          }
#if 0
            UI::Row(&Layout);
            char CurrentAnimationIDString[30];
            sprintf(CurrentAnimationIDString, "Current Anim ID: %d",
                    GameState->CurrentAnimationID.Value);
            UI::DrawTextBox(GameState, &Layout, CurrentAnimationIDString);
#endif
          if(UI::Button("Play as entity"))
          {
            Gameplay::ResetPlayer();
            GameState->PlayerEntityIndex = GameState->SelectedEntityIndex;
            StartAnimationAtGlobalTime(SelectedEntity->AnimController, 0, true);
            StartAnimationAtGlobalTime(SelectedEntity->AnimController, 1, true);
            StartAnimationAtGlobalTime(SelectedEntity->AnimController, 2, true);
          }
          if(GameState->PlayerEntityIndex == GameState->SelectedEntityIndex)
          {
            UI::SliderFloat("Acceleration", &g_Acceleration, 0, 40);
            UI::SliderFloat("Deceleration", &g_Decceleration, 0, 40);
            UI::SliderFloat("Max Speed", &g_MaxSpeed, 0, 50);
            UI::SliderFloat("Playback Rate", &g_MovePlaybackRate, 0.1f, 10);

            { // Walk animation
              static int32_t ActivePathIndex = 0;
#if 0
                if(SelectedEntity->AnimController->AnimationIDs[0].Value > 0)
                {
                  ActivePathIndex = GameState->Resources.GetAnimationPathIndex(
                    SelectedEntity->AnimController->AnimationIDs[0]);
                }
#endif
              UI::Combo("Walk", &ActivePathIndex, GameState->Resources.AnimationPaths, GameState->Resources.AnimationPathCount, PathArrayToString);
              rid NewRID = { 0 };
              if(GameState->Resources.AnimationPathCount > 0 && !GameState->Resources.GetAnimationPathRID(&NewRID, GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                NewRID = GameState->Resources.RegisterAnimation(GameState->Resources.AnimationPaths[ActivePathIndex].Name);
              }
              if(GameState->Resources.AnimationPathCount && SelectedEntity->AnimController->AnimationIDs[0].Value != NewRID.Value)
              {
                Anim::SetAnimation(SelectedEntity->AnimController, NewRID, 0);
                printf("Setting walk\n");
                Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 0, true);
              }
            }
            { // Run animation
              static int32_t ActivePathIndex = 0;
#if 0
                if(SelectedEntity->AnimController->AnimationIDs[1].Value > 0)
                {
                  ActivePathIndex = GameState->Resources.GetAnimationPathIndex(
                    SelectedEntity->AnimController->AnimationIDs[1]);
                }
#endif
              UI::Combo("Run", &ActivePathIndex, GameState->Resources.AnimationPaths, GameState->Resources.AnimationPathCount, PathArrayToString);
              rid NewRID = { 0 };
              if(GameState->Resources.AnimationPathCount > 0 && !GameState->Resources.GetAnimationPathRID(&NewRID, GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                NewRID = GameState->Resources.RegisterAnimation(GameState->Resources.AnimationPaths[ActivePathIndex].Name);
              }
              if(SelectedEntity->AnimController->AnimationIDs[1].Value != NewRID.Value)
              {
                Anim::SetAnimation(SelectedEntity->AnimController, NewRID, 1);
                printf("Setting run\n");
                Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 1, true);
              }
            }
            { // Idle animation
              static int32_t ActivePathIndex = 0;
#if 0
                if(SelectedEntity->AnimController->AnimationIDs[2].Value > 0)
                {
                  ActivePathIndex = GameState->Resources.GetAnimationPathIndex(
                    SelectedEntity->AnimController->AnimationIDs[2]);
                }
#endif
              UI::Combo("Idle", &ActivePathIndex, GameState->Resources.AnimationPaths, GameState->Resources.AnimationPathCount, PathArrayToString);
              rid NewRID = { 0 };
              if(GameState->Resources.AnimationPathCount > 0 && !GameState->Resources.GetAnimationPathRID(&NewRID, GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                NewRID = GameState->Resources.RegisterAnimation(GameState->Resources.AnimationPaths[ActivePathIndex].Name);
              }
              if(SelectedEntity->AnimController->AnimationIDs[2].Value != NewRID.Value)
              {
                Anim::SetAnimation(SelectedEntity->AnimController, NewRID, 2);
                printf("Setting idle\n");
                Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 2, true);
              }
            }
          }
        }
      }
    }
  }
}
