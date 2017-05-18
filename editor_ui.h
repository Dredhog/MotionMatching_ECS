#include "game.h"
#include "ui.h"
#include "debug_drawing.h"
#include "render_data.h"
#include "skeleton.h"

#include <limits>

char*
VoidPtrToCharPtr(void* NamePtr)
{
  return *(char**)NamePtr;
}

char*
VoidPtrToTextureName(void* TextureName)
{
  return ((texture_name*)TextureName)->Name;
}

char*
ElementToBoneName(void* Bone)
{
  return ((Anim::bone*)Bone)->Name;
}

namespace UI
{
  void SliderTransform(game_state* GameState, im_layout* Layout, const char* Text = 0);
  void
  SliderVec3(game_state* GameState, im_layout* Layout, const game_input* Input, const char* Text,
             vec3* VecPtr, float Min = -INFINITY, float Max = INFINITY,
             float ValueScreenDelta = 10.0f)
  {
    UI::Row(GameState, Layout, 3, Text);
    UI::SliderFloat(GameState, Layout, Input, "x", &VecPtr->X, Min, Max, ValueScreenDelta);
    UI::SliderFloat(GameState, Layout, Input, "y", &VecPtr->Y, Min, Max, ValueScreenDelta);
    UI::SliderFloat(GameState, Layout, Input, "z", &VecPtr->Z, Min, Max, ValueScreenDelta);
  }

  void
  SliderVec3Color(game_state* GameState, im_layout* Layout, const game_input* Input,
                  const char* Text, vec3* VecPtr, float Min = 0.0f, float Max = 1.0f,
                  float ValueScreenDelta = 3.0f)
  {
    UI::Row(GameState, Layout, 3, Text);
    UI::SliderFloat(GameState, Layout, Input, "r", &VecPtr->R, Min, Max, ValueScreenDelta);
    UI::SliderFloat(GameState, Layout, Input, "g", &VecPtr->G, Min, Max, ValueScreenDelta);
    UI::SliderFloat(GameState, Layout, Input, "b", &VecPtr->B, Min, Max, ValueScreenDelta);
  }

  void
  SliderVec4Color(game_state* GameState, im_layout* Layout, const game_input* Input,
                  const char* Text, vec4* VecPtr, float Min = 0.0f, float Max = 1.0f,
                  float ValueScreenDelta = 3.0f)
  {
    UI::Row(GameState, Layout, 4, Text);
    UI::SliderFloat(GameState, Layout, Input, "r", &VecPtr->R, Min, Max, ValueScreenDelta);
    UI::SliderFloat(GameState, Layout, Input, "g", &VecPtr->G, Min, Max, ValueScreenDelta);
    UI::SliderFloat(GameState, Layout, Input, "b", &VecPtr->B, Min, Max, ValueScreenDelta);
    UI::SliderFloat(GameState, Layout, Input, "a", &VecPtr->A, Min, Max, ValueScreenDelta);
  }
}

void
IMGUIControlPanel(game_state* GameState, const game_input* Input)
{
  // Humble beginnings of the editor GUI system
  const float StartX      = 0.75f;
  const float StartY      = 1;
  const float LayoutWidth = 0.15f;
  const float RowHeight   = 0.035f;
  const float SliderWidth = 0.05f;

  static bool g_ShowDisplaySet      = false;
  static bool g_ShowEntityTools     = false;
  static bool g_ShowAnimationEditor = false;
  static bool g_ShowMaterialEditor  = false;
  static bool g_ShowCameraSettings  = false;
  static bool g_ShowLightSettings   = false;
  static bool g_ShowGUISettings     = false;

  UI::im_layout Layout = UI::NewLayout({ StartX, StartY }, LayoutWidth, RowHeight, SliderWidth);

  UI::Row(GameState, &Layout, 1, "Select");
  UI::ComboBox((int32_t*)&GameState->SelectionMode, g_SelectionEnumStrings, SELECT_EnumCount,
               GameState, &Layout, Input, sizeof(const char*), VoidPtrToCharPtr);
  if(GameState->SelectionMode == SELECT_Mesh || GameState->SelectionMode == SELECT_Entity)
  {
    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Material Editor", &g_ShowMaterialEditor))
    {
      char MaterialNameBuffer[10];
      sprintf(MaterialNameBuffer, "Material %d", GameState->CurrentMaterialIndex);
      UI::Row(GameState, &Layout, 2, MaterialNameBuffer);
      if(UI::ReleaseButton(GameState, &Layout, Input, "Previous", "Material"))
      {
        if(0 < GameState->CurrentMaterialIndex)
        {
          --GameState->CurrentMaterialIndex;
        }
      }
      if(UI::ReleaseButton(GameState, &Layout, Input, "Next", "Material"))
      {
        if(GameState->CurrentMaterialIndex < GameState->R.MaterialCount - 1)
        {
          ++GameState->CurrentMaterialIndex;
        }
      }
      UI::DrawSquareTexture(GameState, &Layout, GameState->IDTexture);

      assert(GameState->R.MaterialCount > 0);
      material* CurrentMaterial = &GameState->R.Materials[GameState->CurrentMaterialIndex];

      UI::Row(GameState, &Layout, 2, "Shader Type");
      if(UI::ReleaseButton(GameState, &Layout, Input, "Previous"))
      {
        if(CurrentMaterial->Common.ShaderType > 0)
        {
          uint32_t ShaderType                 = CurrentMaterial->Common.ShaderType - 1;
          *CurrentMaterial                    = {};
          CurrentMaterial->Common.ShaderType  = ShaderType;
          CurrentMaterial->Common.UseBlending = true;
        }
      }
      if(UI::ReleaseButton(GameState, &Layout, Input, "  Next  "))
      {
        if(CurrentMaterial->Common.ShaderType < SHADER_EnumCount - 1)
        {
          uint32_t ShaderType                 = CurrentMaterial->Common.ShaderType + 1;
          *CurrentMaterial                    = {};
          CurrentMaterial->Common.ShaderType  = ShaderType;
          CurrentMaterial->Common.UseBlending = true;
        }
      }

      material* Material = &GameState->R.Materials[GameState->CurrentMaterialIndex];
      UI::Row(GameState, &Layout, 1, "Blending");
      UI::_BoolButton(&Layout, Input, "Toggle", &CurrentMaterial->Common.UseBlending);
      switch(Material->Common.ShaderType)
      {
        case SHADER_Phong:
        {
          bool DiffuseFlagValue = (Material->Phong.Flags & PHONG_UseDiffuseMap);

          UI::Row(GameState, &Layout, 1, "Use Diffuse");
          UI::_BoolButton(&Layout, Input, "Toggle", &DiffuseFlagValue);
          if(DiffuseFlagValue)
          {
            Material->Phong.Flags |= PHONG_UseDiffuseMap;

            UI::Row(GameState, &Layout, 1, "Diffuse Map");
            {
              int32_t ActiveDiffuseMapIndex = CurrentMaterial->Phong.DiffuseMapIndex;
              UI::ComboBox(&ActiveDiffuseMapIndex, GameState->R.TextureNames,
                           GameState->R.TextureCount, GameState, &Layout, Input,
                           sizeof(texture_name), VoidPtrToTextureName);
              CurrentMaterial->Phong.DiffuseMapIndex = ActiveDiffuseMapIndex;
            }
          }
          else
          {
            Material->Phong.Flags &= ~PHONG_UseDiffuseMap;

            UI::Row(GameState, &Layout, 4, "Diffuse Color");
            UI::SliderFloat(GameState, &Layout, Input, "R", &CurrentMaterial->Phong.DiffuseColor.R,
                            0, 1.0f, 5.0f);
            UI::SliderFloat(GameState, &Layout, Input, "G", &CurrentMaterial->Phong.DiffuseColor.G,
                            0, 1.0f, 5.0f);
            UI::SliderFloat(GameState, &Layout, Input, "B", &CurrentMaterial->Phong.DiffuseColor.B,
                            0, 1.0f, 5.0f);
            UI::SliderFloat(GameState, &Layout, Input, "A", &CurrentMaterial->Phong.DiffuseColor.A,
                            0, 1.0f, 5.0f);
          }

          bool SpecularFlagValue = Material->Phong.Flags & PHONG_UseSpecularMap;
          UI::Row(GameState, &Layout, 1, "Use Specular");
          UI::_BoolButton(&Layout, Input, "Toggle", &SpecularFlagValue);

          if(SpecularFlagValue)
          {
            Material->Phong.Flags |= PHONG_UseSpecularMap;

            UI::Row(GameState, &Layout, 1, "Specular");
            {
              int32_t ActiveSpecularMapIndex = CurrentMaterial->Phong.SpecularMapIndex;
              UI::ComboBox(&ActiveSpecularMapIndex, GameState->R.TextureNames,
                           GameState->R.TextureCount, GameState, &Layout, Input,
                           sizeof(texture_name), VoidPtrToTextureName);
              CurrentMaterial->Phong.SpecularMapIndex = ActiveSpecularMapIndex;
            }
          }
          else
          {
            Material->Phong.Flags &= ~PHONG_UseSpecularMap;
          }

          bool NormalFlagValue = Material->Phong.Flags & PHONG_UseNormalMap;

          UI::Row(GameState, &Layout, 1, "Use Normal");
          UI::_BoolButton(&Layout, Input, "Toggle", &NormalFlagValue);
          if(NormalFlagValue)
          {
            Material->Phong.Flags |= PHONG_UseNormalMap;

            UI::Row(GameState, &Layout, 1, "Normal");
            {
              int32_t ActiveNormalMapIndex = CurrentMaterial->Phong.NormalMapIndex;
              UI::ComboBox(&ActiveNormalMapIndex, GameState->R.TextureNames,
                           GameState->R.TextureCount, GameState, &Layout, Input,
                           sizeof(texture_name), VoidPtrToTextureName);
              CurrentMaterial->Phong.NormalMapIndex = ActiveNormalMapIndex;
            }
          }
          else
          {
            Material->Phong.Flags &= ~PHONG_UseNormalMap;
          }

          bool SkeletalFlagValue = (Material->Phong.Flags & PHONG_UseSkeleton);

          UI::Row(GameState, &Layout, 1, "Is Skeletal");
          UI::_BoolButton(&Layout, Input, "Toggle", &SkeletalFlagValue);
          if(SkeletalFlagValue)
          {
            Material->Phong.Flags |= PHONG_UseSkeleton;
            Material->Common.IsSkeletal = true;
          }
          else
          {
            Material->Phong.Flags &= ~PHONG_UseSkeleton;
            Material->Common.IsSkeletal = false;
          }

          UI::Row(GameState, &Layout, 1, "Shininess");
          UI::SliderFloat(GameState, &Layout, Input, "Shi", &Material->Phong.Shininess, 1.0f,
                          512.0f, 1024.0f);
        }
        break;
        case SHADER_Color:
        {
          UI::SliderVec4Color(GameState, &Layout, Input, "Color", &Material->Color.Color);
        }
        break;
      }
      UI::Row(&Layout);
      if(UI::ReleaseButton(GameState, &Layout, Input, "Create New"))
      {
        material NewMaterial           = {};
        NewMaterial.Common.UseBlending = true;
        NewMaterial.Phong.DiffuseColor = { 0.5f, 0.5f, 0.5f, 1.0f };
        NewMaterial.Phong.Shininess    = 60;
        AddMaterial(&GameState->R, NewMaterial);
        GameState->CurrentMaterialIndex = GameState->R.MaterialCount - 1;
      }
      UI::Row(&Layout);
      if(UI::ReleaseButton(GameState, &Layout, Input, "Reset Material"))
      {
        uint32_t ShaderType                = CurrentMaterial->Common.ShaderType;
        *CurrentMaterial                   = {};
        CurrentMaterial->Common.ShaderType = ShaderType;
      }
      UI::Row(&Layout);
      if(UI::ReleaseButton(GameState, &Layout, Input, "Create From Current"))
      {
        AddMaterial(&GameState->R, *CurrentMaterial);
        GameState->CurrentMaterialIndex = GameState->R.MaterialCount - 1;
      }
      entity* SelectedEntity = {};
      if(GetSelectedEntity(GameState, &SelectedEntity))
      {
        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, "Apply To Selected"))
        {
          if(0 <= GameState->CurrentMaterialIndex &&
             GameState->CurrentMaterialIndex < GameState->R.MaterialCount)
          {
            if(GameState->SelectionMode == SELECT_Mesh)
            {
              SelectedEntity->MaterialIndices[GameState->SelectedMeshIndex] =
                GameState->CurrentMaterialIndex;
            }
            else if(GameState->SelectionMode == SELECT_Entity)
            {
              Render::model* Model = GameState->Resources.GetModel(SelectedEntity->ModelID);
              for(int m = 0; m < Model->MeshCount; m++)
              {
                SelectedEntity->MaterialIndices[m] = GameState->CurrentMaterialIndex;
              }
            }
          }
        }
      }
    }
  }
  if(GameState->SelectionMode == SELECT_Entity)
  {
    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Entity Tools", &g_ShowEntityTools))
    {
      char ModelNameBuffer[10];
      sprintf(ModelNameBuffer, "Model %d", GameState->CurrentModelIndex);
      UI::Row(GameState, &Layout, 2, ModelNameBuffer);
      if(UI::ReleaseButton(GameState, &Layout, Input, "Previous", "model"))
      {
        if(0 < GameState->CurrentModelIndex)
        {
          --GameState->CurrentModelIndex;
        }
      }
      if(UI::ReleaseButton(GameState, &Layout, Input, "Next", "model"))
      {
        if(GameState->CurrentModelIndex < GameState->R.ModelCount - 1)
        {
          ++GameState->CurrentModelIndex;
        }
      }
      UI::Row(&Layout);
      if(UI::ReleaseButton(GameState, &Layout, Input, "Create Entity"))
      {
        GameState->IsEntityCreationMode = !GameState->IsEntityCreationMode;
      }

      entity* SelectedEntity = {};
      if(GetSelectedEntity(GameState, &SelectedEntity))
      {
        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, "Delete Entity"))
        {
          DeleteEntity(GameState, GameState->SelectedEntityIndex);
          GameState->SelectedEntityIndex = -1;
        }
        if(SelectedEntity->AnimController)
        {
          UI::Row(&Layout);
          if(UI::ReleaseButton(GameState, &Layout, Input, "Animate Selected Entity"))
          {
            GameState->SelectionMode = SELECT_Bone;
            AttachEntityToAnimEditor(GameState, &GameState->AnimEditor,
                                     GameState->SelectedEntityIndex);
            g_ShowAnimationEditor = true;
          }
          if(GameState->TestAnimation)
          {
            UI::Row(&Layout);
            if(UI::ReleaseButton(GameState, &Layout, Input, "Add Animation"))
            {
              Anim::AddAnimation(SelectedEntity->AnimController, GameState->TestAnimation);
              Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 0);
            }
          }
        }

        Anim::transform* Transform = &SelectedEntity->Transform;
        UI::SliderVec3(GameState, &Layout, Input, "Translation", &Transform->Translation, -INFINITY,
                       INFINITY, 10.0f);
        UI::SliderVec3(GameState, &Layout, Input, "Rotation", &Transform->Rotation, -INFINITY,
                       INFINITY, 720.0f);
        UI::SliderVec3(GameState, &Layout, Input, "Scale", &Transform->Scale, -INFINITY, INFINITY,
                       10.0f);

        Render::model* SelectedModel = GameState->Resources.GetModel(SelectedEntity->ModelID);
        if(SelectedModel->Skeleton)
        {
          UI::Row(&Layout);
          if(!SelectedEntity->AnimController &&
             UI::ReleaseButton(GameState, &Layout, Input, "Add Anim. Controller"))
          {
            SelectedEntity->AnimController =
              PushStruct(GameState->PersistentMemStack, Anim::animation_controller);
            *SelectedEntity->AnimController = {};

            SelectedEntity->AnimController->Skeleton = SelectedModel->Skeleton;
            SelectedEntity->AnimController->OutputTransforms =
              PushArray(GameState->PersistentMemStack,
                        ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT *
                          SelectedModel->Skeleton->BoneCount,
                        Anim::transform);
            SelectedEntity->AnimController->BoneSpaceMatrices =
              PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount,
                        mat4);
            SelectedEntity->AnimController->ModelSpaceMatrices =
              PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount,
                        mat4);
            SelectedEntity->AnimController->HierarchicalModelSpaceMatrices =
              PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount,
                        mat4);
          }
          else if(SelectedEntity->AnimController &&
                  (UI::ReleaseButton(GameState, &Layout, Input, "Delete Anim. Controller")))
          {
            SelectedEntity->AnimController = 0;
          }
        }
      }
    }
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Animation Editor", &g_ShowAnimationEditor))
  {
    if(GameState->AnimEditor.Skeleton)
    {
      UI::Row(&Layout);
      if(UI::ReleaseButton(GameState, &Layout, Input, "Stop Editing"))
      {
        DettachEntityFromAnimEditor(GameState, &GameState->AnimEditor);
        GameState->SelectionMode = SELECT_Entity;
        g_ShowEntityTools        = true;
        g_ShowAnimationEditor    = false;
      }
    }

    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Import Animation"))
    {
      Anim::animation_group* AnimGroup;
      Asset::ImportAnimationGroup(GameState->PersistentMemStack, &AnimGroup,
                                  "data/animation_export_test");
      GameState->TestAnimation = AnimGroup->Animations[0];
    }
    if(GameState->TestAnimation && GameState->AnimEditor.Skeleton &&
       GameState->AnimEditor.Skeleton->BoneCount == GameState->TestAnimation->ChannelCount)
    {
      UI::Row(&Layout);
      if(UI::ReleaseButton(GameState, &Layout, Input, "Edit Loaded Animation"))
      {
        EditAnimation::EditAnimation(&GameState->AnimEditor, GameState->TestAnimation);
      }
    }
    if(GameState->SelectionMode == SELECT_Bone)
    {
      if(GameState->SelectionMode == SELECT_Bone && GameState->AnimEditor.Skeleton)
      {
        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, "Export Animation"))
        {
          Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor,
                                      "data/animation_export_test");
        }
        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, "Delete keyframe"))
        {
          EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
        }
        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, "Insert keyframe"))
        {
          EditAnimation::InsertBlendedKeyframeAtTime(&GameState->AnimEditor,
                                                     GameState->AnimEditor.PlayHeadTime);
        }

        UI::Row(GameState, &Layout, 1, "Playhead");
        UI::SliderFloat(GameState, &Layout, Input, "Playhead Time",
                        &GameState->AnimEditor.PlayHeadTime, -100, 100, 2.0f);
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, 0);
        if(GameState->AnimEditor.KeyframeCount > 0)
        {
          UI::Row(GameState, &Layout, 1, "Bone");
          {
            int32_t ActiveBoneIndex = GameState->AnimEditor.CurrentBone;
            UI::ComboBox(&ActiveBoneIndex, GameState->AnimEditor.Skeleton->Bones,
                         GameState->AnimEditor.Skeleton->BoneCount, GameState, &Layout, Input,
                         sizeof(Anim::bone), ElementToBoneName);
            EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, ActiveBoneIndex);
          }

          UI::Row(&Layout);
          UI::DrawTextBox(GameState, &Layout, "Transform", g_DescriptionColor);

          Anim::transform* Transform =
            &GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
               .Transforms[GameState->AnimEditor.CurrentBone];
          mat4 Mat4Transform = TransformToGizmoMat4(Transform);
          UI::SliderVec3(GameState, &Layout, Input, "Translation", &Transform->Translation,
                         -INFINITY, INFINITY, 10.0f);
          UI::SliderVec3(GameState, &Layout, Input, "Rotation", &Transform->Rotation, -INFINITY,
                         INFINITY, 720.0f);
          UI::SliderVec3(GameState, &Layout, Input, "Scale", &Transform->Scale, -INFINITY, INFINITY,
                         10.0f);
        }
      }
    }
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Light Settings", &g_ShowLightSettings))
  {
    UI::SliderVec3Color(GameState, &Layout, Input, "Diffuse",
                        (vec3*)&GameState->R.LightDiffuseColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "Specular",
                        (vec3*)&GameState->R.LightSpecularColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "Ambient",
                        (vec3*)&GameState->R.LightAmbientColor);
    UI::SliderVec3(GameState, &Layout, Input, "Position", &GameState->R.LightPosition);
    UI::Row(GameState, &Layout, 1, "Show Gizmo");
    UI::BoolButton(GameState, &Layout, Input, "Show", &GameState->R.ShowLightPosition);
  }
  if(GameState->R.ShowLightPosition)
  {
    mat4 Mat4LightPosition = Math::Mat4Translate(GameState->R.LightPosition);
    Debug::PushGizmo(&GameState->Camera, &Mat4LightPosition);
  }

  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Render Switches", &g_ShowDisplaySet))
  {
    UI::Row(GameState, &Layout, 1, "Timeline");
    UI::_BoolButton(&Layout, Input, "Toggle", &GameState->DrawTimeline);
    UI::Row(GameState, &Layout, 1, "Gizmos");
    UI::_BoolButton(&Layout, Input, "Toggle", &GameState->DrawGizmos);
    UI::Row(GameState, &Layout, 1, "Debug Spheres");
    UI::_BoolButton(&Layout, Input, "Toggle", &GameState->DrawDebugSpheres);
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Camera     ", &g_ShowCameraSettings))
  {
    UI::Row(GameState, &Layout, 1, "FOV    ");
    UI::SliderFloat(GameState, &Layout, Input, "FieldOfView", &GameState->Camera.FieldOfView, 0,
                    200.0f, 50.0f);
    UI::Row(GameState, &Layout, 1, "Near  ");
    UI::SliderFloat(GameState, &Layout, Input, "Near Clip Plane", &GameState->Camera.NearClipPlane,
                    0.001f, 1000, 50.0f);
    UI::Row(GameState, &Layout, 1, "Far   ");
    UI::SliderFloat(GameState, &Layout, Input, "Far Clip Plane", &GameState->Camera.FarClipPlane,
                    GameState->Camera.NearClipPlane, 1000, 50.0f);
    UI::Row(GameState, &Layout, 1, "Speed");
    UI::SliderFloat(GameState, &Layout, Input, "Far Clip Plane", &GameState->Camera.Speed, 0, 200,
                    50.0f);
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "GUI Colors     ", &g_ShowGUISettings))
  {
    UI::SliderVec3Color(GameState, &Layout, Input, "Border", (vec3*)&g_BorderColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "Base", (vec3*)&g_NormalColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "Highlight", (vec3*)&g_HighlightColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "Pressed", (vec3*)&g_PressedColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "Bool Base", (vec3*)&g_BoolNormalColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "BoolPressed", (vec3*)&g_BoolPressedColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "Bool Highlight", (vec3*)&g_BoolHighlightColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "Description bg", (vec3*)&g_DescriptionColor);
    UI::SliderVec3Color(GameState, &Layout, Input, "Font", (vec3*)&g_FontColor);
  }
}

void
VisualizeTimeline(game_state* GameState)
{
  const int KeyframeCount = GameState->AnimEditor.KeyframeCount;
  if(KeyframeCount > 0)
  {
    const EditAnimation::animation_editor* Editor = &GameState->AnimEditor;

    const float TimelineStartX     = 0.20f;
    const float TimelineStartY     = 0.1f;
    const float TimelineWidth      = 0.6f;
    const float TimelineHeight     = 0.05f;
    const float CurrentMarkerWidth = 0.002f;
    const float FirstTime          = MinFloat(Editor->PlayHeadTime, Editor->SampleTimes[0]);
    const float LastTime =
      MaxFloat(Editor->PlayHeadTime, Editor->SampleTimes[Editor->KeyframeCount - 1]);

    float KeyframeSpacing = KEYFRAME_MIN_TIME_DIFFERENCE_APART * 0.5f; // seconds
    float TimeDiff        = MaxFloat((LastTime - FirstTime), 1.0f);
    float KeyframeWidth   = KeyframeSpacing / TimeDiff;

    Debug::PushQuad({ TimelineStartX, TimelineStartY }, TimelineWidth, TimelineHeight,
                    { 1, 1, 1, 1 });
    for(int i = 0; i < KeyframeCount; i++)
    {
      float PosPercentage = (Editor->SampleTimes[i] - FirstTime) / TimeDiff;
      Debug::PushQuad(vec3{ TimelineStartX + PosPercentage * TimelineWidth - 0.5f * KeyframeWidth,
                            TimelineStartY, -0.1f },
                      KeyframeWidth, TimelineHeight, { 0.5f, 0.3f, 0.3f, 1 });
    }
    float PlayheadPercentage = (Editor->PlayHeadTime - FirstTime) / TimeDiff;
    Debug::PushQuad(vec3{ TimelineStartX + PlayheadPercentage * TimelineWidth -
                            0.5f * KeyframeWidth,
                          TimelineStartY, -0.2f },
                    KeyframeWidth, TimelineHeight, { 1, 0, 0, 1 });
    float CurrentPercentage = (Editor->SampleTimes[Editor->CurrentKeyframe] - FirstTime) / TimeDiff;
    Debug::PushQuad(vec3{ TimelineStartX + CurrentPercentage * TimelineWidth -
                            0.5f * CurrentMarkerWidth,
                          TimelineStartY, -0.3f },
                    CurrentMarkerWidth, TimelineHeight, { 0.5f, 0.5f, 0.5f, 1 });
  }
}
