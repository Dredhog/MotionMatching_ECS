#include "game.h"
#include "ui.h"
#include "debug_drawing.h"
#include "render_data.h"
#include "skeleton.h"
#include "time.h"
#include "scene.h"

#include <limits>

char*
VoidPtrToCharPtr(void* NamePtr)
{
  return *(char**)NamePtr;
}

char*
ModelPathToCharPtr(void* CharArray)
{
  static const size_t PrefixLength = strlen("data/built/");
  return ((char*)CharArray) + PrefixLength;
}

char*
TexturePathToCharPtr(void* CharArray)
{
  static const size_t PrefixLength = strlen("data/textures/");
  return ((char*)CharArray) + PrefixLength;
}

char*
AnimationPathToCharPtr(void* CharArray)
{
  static const size_t PrefixLength = strlen("data/animations/");
  return ((char*)CharArray) + PrefixLength;
}

char*
MaterialPathToCharPtr(void* CharArray)
{
  static const size_t PrefixLength = strlen("data/materials/");
  return ((char*)CharArray) + PrefixLength;
}

char*
BonePtrToCharPtr(void* Bone)
{
  return ((Anim::bone*)Bone)->Name;
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
  static bool g_ShowSceneSettings   = false;

  UI::im_layout Layout = UI::NewLayout({ StartX, StartY }, LayoutWidth, RowHeight, SliderWidth);

  UI::Row(GameState, &Layout, 1, "Select");
  UI::ComboBox((int32_t*)&GameState->SelectionMode, g_SelectionEnumStrings, SELECT_EnumCount,
               GameState, &Layout, Input, sizeof(const char*), VoidPtrToCharPtr);
  if(GameState->SelectionMode == SELECT_Mesh || GameState->SelectionMode == SELECT_Entity)
  {
    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Material Editor", &g_ShowMaterialEditor))
    {
      {
        int32_t ActivePathIndex = 0;
        if(GameState->CurrentMaterialID.Value > 0)
        {
          ActivePathIndex = GameState->Resources.GetMaterialPathIndex(GameState->CurrentMaterialID);
        }
        UI::Row(&Layout);
        UI::ComboBox(&ActivePathIndex, GameState->Resources.MaterialPaths,
                     GameState->Resources.MaterialPathCount, GameState, &Layout, Input,
                     sizeof(path), MaterialPathToCharPtr);
        if(GameState->Resources.MaterialPathCount > 0)
        {
          rid NewRID = { 0 };
          if(GameState->Resources
               .GetMaterialPathRID(&NewRID,
                                   GameState->Resources.MaterialPaths[ActivePathIndex].Name))
          {
            GameState->CurrentMaterialID = NewRID;
          }
          else
          {
            GameState->CurrentMaterialID = GameState->Resources.RegisterMaterial(
              GameState->Resources.MaterialPaths[ActivePathIndex].Name);
          }
        }
      }
      if(GameState->CurrentMaterialID.Value > 0)
      {
        // Draw material preview to texture
        UI::DrawSquareTexture(GameState, &Layout, GameState->IDTexture);

        material* CurrentMaterial = GameState->Resources.GetMaterial(GameState->CurrentMaterialID);
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
        if(UI::ReleaseButton(GameState, &Layout, Input, "Next"))
        {
          if(CurrentMaterial->Common.ShaderType < SHADER_EnumCount - 1)
          {
            uint32_t ShaderType                 = CurrentMaterial->Common.ShaderType + 1;
            *CurrentMaterial                    = {};
            CurrentMaterial->Common.ShaderType  = ShaderType;
            CurrentMaterial->Common.UseBlending = true;
          }
        }
        UI::Row(GameState, &Layout, 1, "Blending");
        UI::_BoolButton(&Layout, Input, "Toggle", &CurrentMaterial->Common.UseBlending);

        switch(CurrentMaterial->Common.ShaderType)
        {
          case SHADER_Phong:
          {
            SliderVec3Color(GameState, &Layout, Input, "Ambient Color",
                            &CurrentMaterial->Phong.AmbientColor, 0.0f, 1.0f, 5.0f);

            bool UseDIffuse = (CurrentMaterial->Phong.Flags & PHONG_UseDiffuseMap);

            UI::Row(GameState, &Layout, 1, "Use Diffuse");
            UI::_BoolButton(&Layout, Input, "Toggle", &UseDIffuse);

            if(UseDIffuse)
            {
              UI::Row(GameState, &Layout, 1, "Diffuse Map");
              {

                int32_t ActivePathIndex = 0;
                if(CurrentMaterial->Phong.DiffuseMapID.Value > 0)
                {
                  ActivePathIndex =
                    GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.DiffuseMapID);
                }
                if(GameState->Resources.TexturePathCount > 0)
                {
                  CurrentMaterial->Phong.Flags |= PHONG_UseDiffuseMap;

                  UI::ComboBox(&ActivePathIndex, GameState->Resources.TexturePaths,
                               GameState->Resources.TexturePathCount, GameState, &Layout, Input,
                               sizeof(path), TexturePathToCharPtr);
                  rid NewRID;
                  if(GameState->Resources
                       .GetTexturePathRID(&NewRID,
                                          GameState->Resources.TexturePaths[ActivePathIndex].Name))
                  {
                    CurrentMaterial->Phong.DiffuseMapID = NewRID;
                  }
                  else
                  {
                    CurrentMaterial->Phong.DiffuseMapID = GameState->Resources.RegisterTexture(
                      GameState->Resources.TexturePaths[ActivePathIndex].Name);
                  }
                  assert(CurrentMaterial->Phong.DiffuseMapID.Value > 0);
                }
              }
            }
            else
            {
              CurrentMaterial->Phong.Flags &= ~PHONG_UseDiffuseMap;
              SliderVec4Color(GameState, &Layout, Input, "Diffuse Color",
                              &CurrentMaterial->Phong.DiffuseColor, 0.0f, 1.0f, 5.0f);
            }

            bool UseSpecular = CurrentMaterial->Phong.Flags & PHONG_UseSpecularMap;
            UI::Row(GameState, &Layout, 1, "Use Specular");
            UI::_BoolButton(&Layout, Input, "Toggle", &UseSpecular);

            if(UseSpecular)
            {
              UI::Row(GameState, &Layout, 1, "Specular");
              {
                int32_t ActivePathIndex = 0;
                if(CurrentMaterial->Phong.SpecularMapID.Value > 0)
                {
                  ActivePathIndex =
                    GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.SpecularMapID);
                }
                UI::ComboBox(&ActivePathIndex, GameState->Resources.TexturePaths,
                             GameState->Resources.TexturePathCount, GameState, &Layout, Input,
                             sizeof(path), TexturePathToCharPtr);
                if(GameState->Resources.TexturePathCount > 0)
                {
                  CurrentMaterial->Phong.Flags |= PHONG_UseSpecularMap;

                  rid NewRID;
                  if(GameState->Resources
                       .GetTexturePathRID(&NewRID,
                                          GameState->Resources.TexturePaths[ActivePathIndex].Name))
                  {
                    CurrentMaterial->Phong.SpecularMapID = NewRID;
                  }
                  else
                  {
                    CurrentMaterial->Phong.SpecularMapID = GameState->Resources.RegisterTexture(
                      GameState->Resources.TexturePaths[ActivePathIndex].Name);
                  }
                  assert(CurrentMaterial->Phong.SpecularMapID.Value > 0);
                }
              }
            }
            else
            {
              CurrentMaterial->Phong.Flags &= ~PHONG_UseSpecularMap;
              SliderVec3Color(GameState, &Layout, Input, "Specular Color",
                              &CurrentMaterial->Phong.SpecularColor, 0.0f, 1.0f, 5.0f);
            }

            bool NormalFlagValue = CurrentMaterial->Phong.Flags & PHONG_UseNormalMap;

            UI::Row(GameState, &Layout, 1, "Use Normal");
            UI::_BoolButton(&Layout, Input, "Toggle", &NormalFlagValue);
            if(NormalFlagValue)
            {
              UI::Row(GameState, &Layout, 1, "Normal");
              {
                int32_t ActivePathIndex = 0;
                if(CurrentMaterial->Phong.NormalMapID.Value > 0)
                {
                  ActivePathIndex =
                    GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.NormalMapID);
                }
                UI::ComboBox(&ActivePathIndex, GameState->Resources.TexturePaths,
                             GameState->Resources.TexturePathCount, GameState, &Layout, Input,
                             sizeof(path), TexturePathToCharPtr);
                if(GameState->Resources.TexturePathCount > 0)
                {
                  CurrentMaterial->Phong.Flags |= PHONG_UseNormalMap;

                  rid NewRID;
                  if(GameState->Resources
                       .GetTexturePathRID(&NewRID,
                                          GameState->Resources.TexturePaths[ActivePathIndex].Name))
                  {
                    CurrentMaterial->Phong.NormalMapID = NewRID;
                  }
                  else
                  {
                    CurrentMaterial->Phong.NormalMapID = GameState->Resources.RegisterTexture(
                      GameState->Resources.TexturePaths[ActivePathIndex].Name);
                  }
                  assert(CurrentMaterial->Phong.NormalMapID.Value > 0);
                }
              }
            }
            else
            {
              CurrentMaterial->Phong.Flags &= ~PHONG_UseNormalMap;
            }

            bool SkeletalFlagValue = (CurrentMaterial->Phong.Flags & PHONG_UseSkeleton);

            UI::Row(GameState, &Layout, 1, "Is Skeletal");
            UI::_BoolButton(&Layout, Input, "Toggle", &SkeletalFlagValue);
            if(SkeletalFlagValue)
            {
              CurrentMaterial->Phong.Flags |= PHONG_UseSkeleton;
              CurrentMaterial->Common.IsSkeletal = true;
            }
            else
            {
              CurrentMaterial->Phong.Flags &= ~PHONG_UseSkeleton;
              CurrentMaterial->Common.IsSkeletal = false;
            }

            UI::Row(GameState, &Layout, 1, "Shininess");
            UI::SliderFloat(GameState, &Layout, Input, "Shi", &CurrentMaterial->Phong.Shininess,
                            1.0f, 512.0f, 1024.0f);
          }
          break;
          case SHADER_Color:
          {
            UI::SliderVec4Color(GameState, &Layout, Input, "Color", &CurrentMaterial->Color.Color);
          }
          break;
        }
        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, "Clear Material Fields"))
        {
          uint32_t ShaderType                = CurrentMaterial->Common.ShaderType;
          *CurrentMaterial                   = {};
          CurrentMaterial->Common.ShaderType = ShaderType;
        }
        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, "Create New"))
        {
          GameState->CurrentMaterialID =
            GameState->Resources.CreateMaterial(NewPhongMaterial(), NULL);
        }
        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, "Duplicate Current"))
        {
          GameState->CurrentMaterialID =
            GameState->Resources.CreateMaterial(*CurrentMaterial, NULL);
        }
        entity* SelectedEntity = {};
        if(GetSelectedEntity(GameState, &SelectedEntity))
        {
          UI::Row(&Layout);
          if(UI::ReleaseButton(GameState, &Layout, Input, "Apply To Selected"))
          {
            if(GameState->CurrentMaterialID.Value > 0)
            {
              if(GameState->SelectionMode == SELECT_Mesh)
              {
                SelectedEntity->MaterialIDs[GameState->SelectedMeshIndex] =
                  GameState->CurrentMaterialID;
              }
              else if(GameState->SelectionMode == SELECT_Entity)
              {
                Render::model* Model = GameState->Resources.GetModel(SelectedEntity->ModelID);
                for(int m = 0; m < Model->MeshCount; m++)
                {
                  SelectedEntity->MaterialIDs[m] = GameState->CurrentMaterialID;
                }
              }
            }
          }
          if(GameState->SelectionMode == SELECT_Mesh)
          {
            UI::Row(&Layout);
            if(UI::ReleaseButton(GameState, &Layout, Input, "Edit Selected"))
            {
              GameState->CurrentMaterialID =
                SelectedEntity->MaterialIDs[GameState->SelectedMeshIndex];
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
      UI::Row(&Layout);
      static int32_t ActivePathIndex = 0;
      UI::ComboBox(&ActivePathIndex, GameState->Resources.ModelPaths,
                   GameState->Resources.ModelPathCount, GameState, &Layout, Input, sizeof(path),
                   ModelPathToCharPtr);
      {
        rid NewRID = { 0 };
        if(!GameState->Resources
              .GetModelPathRID(&NewRID, GameState->Resources.ModelPaths[ActivePathIndex].Name))
        {
          NewRID = GameState->Resources.RegisterModel(
            GameState->Resources.ModelPaths[ActivePathIndex].Name);
          GameState->CurrentModelID = NewRID;
        }
        else
        {
          GameState->CurrentModelID = NewRID;
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
                        ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT * SelectedModel->Skeleton->BoneCount,
                        Anim::transform);
            SelectedEntity->AnimController->BoneSpaceMatrices =
              PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
            SelectedEntity->AnimController->ModelSpaceMatrices =
              PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
            SelectedEntity->AnimController->HierarchicalModelSpaceMatrices =
              PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
          }
          else if(SelectedEntity->AnimController &&
                  UI::ReleaseButton(GameState, &Layout, Input, "Delete Anim. Controller"))
          {
            SelectedEntity->AnimController = 0;
          }
          else if(SelectedEntity->AnimController)
          {
            UI::Row(&Layout);
            if(UI::ReleaseButton(GameState, &Layout, Input, "Animate Selected Entity"))
            {
              GameState->SelectionMode = SELECT_Bone;
              AttachEntityToAnimEditor(GameState, &GameState->AnimEditor,
                                       GameState->SelectedEntityIndex);
              g_ShowAnimationEditor = true;
            }
            {
              static int32_t ActivePathIndex = 0;
              UI::Row(&Layout);
              UI::ComboBox(&ActivePathIndex, GameState->Resources.AnimationPaths,
                           GameState->Resources.AnimationPathCount, GameState, &Layout, Input,
                           sizeof(path), AnimationPathToCharPtr);
              rid NewRID = { 0 };
              if(!GameState->Resources
                    .GetAnimationPathRID(&NewRID,
                                         GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                GameState->CurrentAnimationID = GameState->Resources.RegisterAnimation(
                  GameState->Resources.AnimationPaths[ActivePathIndex].Name);
              }
              GameState->CurrentAnimationID = NewRID;
            }
            if(GameState->CurrentAnimationID.Value > 0)
            {
              UI::Row(&Layout);
              if(UI::ReleaseButton(GameState, &Layout, Input, "Add Animation"))
              {
                if(GameState->Resources.GetAnimation(GameState->CurrentAnimationID)->ChannelCount ==
                   SelectedModel->Skeleton->BoneCount)
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
                  Anim::StopAnimation(SelectedEntity->AnimController, 0);
                }
              }
            }
          }
        }
      }
    }
  }
  if(GameState->SelectionMode == SELECT_Bone && GameState->AnimEditor.Skeleton)
  {
    entity* AttachedEntity;
    if(GetEntityAtIndex(GameState, &AttachedEntity, GameState->AnimEditor.EntityIndex))
    {
      Render::model* AttachedModel = GameState->Resources.GetModel(AttachedEntity->ModelID);
      assert(AttachedModel->Skeleton == GameState->AnimEditor.Skeleton);
    }
    else
    {
      assert(0 && "no entity found in GameState->AnimEditor");
    }

    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Animation Editor", &g_ShowAnimationEditor))
    {
      UI::Row(&Layout);
      if(UI::ReleaseButton(GameState, &Layout, Input, "Stop Editing"))
      {
        DettachEntityFromAnimEditor(GameState, &GameState->AnimEditor);
        GameState->SelectionMode = SELECT_Entity;
        g_ShowEntityTools        = true;
        g_ShowAnimationEditor    = false;
      }

      if(GameState->AnimEditor.Skeleton)
      {
        if(0 < AttachedEntity->AnimController->AnimStateCount)
        {
          Anim::animation* Animation = AttachedEntity->AnimController->Animations[0];
          UI::Row(&Layout);
          if(UI::ReleaseButton(GameState, &Layout, Input, "Edit Attached Animation"))
          {
            int32_t AnimationPathIndex = GameState->Resources.GetAnimationPathIndex(
              AttachedEntity->AnimController->AnimationIDs[0]);
            EditAnimation::EditAnimation(&GameState->AnimEditor, Animation,
                                         GameState->Resources.AnimationPaths[AnimationPathIndex]
                                           .Name);
          }
        }
        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, "Insert keyframe"))
        {
          EditAnimation::InsertBlendedKeyframeAtTime(&GameState->AnimEditor,
                                                     GameState->AnimEditor.PlayHeadTime);
        }
        if(GameState->AnimEditor.KeyframeCount > 0)
        {
          UI::Row(&Layout);
          if(UI::ReleaseButton(GameState, &Layout, Input, "Delete keyframe"))
          {
            EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
          }
          UI::Row(&Layout);
          if(UI::ReleaseButton(GameState, &Layout, Input, "Export Animation"))
          {
            UI::Row(&Layout);
            time_t     current_time;
            struct tm* time_info;
            char       AnimGroupName[30];
            time(&current_time);
            time_info = localtime(&current_time);
            strftime(AnimGroupName, sizeof(AnimGroupName), "data/animations/%H_%M_%S.anim",
                     time_info);
            Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor,
                                        AnimGroupName);
          }
          if(GameState->AnimEditor.AnimationPath[0] != '\0')
          {
            UI::Row(&Layout);
            if(UI::ReleaseButton(GameState, &Layout, Input, "Override Animation"))
            {
              Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor,
                                          GameState->AnimEditor.AnimationPath);
            }
          }
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
                         sizeof(Anim::bone), BonePtrToCharPtr);
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
  if(UI::_ExpandableButton(&Layout, Input, "Camera", &g_ShowCameraSettings))
  {
    UI::Row(GameState, &Layout, 1, "FOV");
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
    UI::Row(GameState, &Layout, 1, "Cubemap");
    UI::_BoolButton(&Layout, Input, "Toggle", &GameState->DrawCubemap);

    if(GameState->DrawCubemap)
    {
    }
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "GUI Colors", &g_ShowGUISettings))
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
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Scene", &g_ShowSceneSettings))
  {
    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Export Scene"))
    {
      ExportScene(GameState, "data/scenes/fist_scene_export.scene");
    }
    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Import Scene"))
    {
      ImportScene(GameState, "data/scenes/fist_scene_export.scene");
    }
  }

  static bool g_ShowHeapActions = true;
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Heap Test", &g_ShowHeapActions))
  {
    static float g_ActionSize = Kibibytes(1);

    UI::Row(&Layout);
    UI::SliderFloat(GameState, &Layout, Input, "Action Size", &g_ActionSize,
                    (float)sizeof(Memory::free_block), Kibibytes(1), 512.0f);

    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Create"))
    {
      uint8_t* MemForHeap = GameState->PersistentMemStack->Alloc((int32_t)g_ActionSize);
      GameState->HeapAllocator.Create(MemForHeap, (int32_t)g_ActionSize);
    }
    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Alloc"))
    {
      GameState->HeapAllocator.Alloc((int32_t)g_ActionSize);
    }

    if(GameState->HeapAllocator.m_Base != NULL)
    {
      static float g_MemStartX    = 0.1f;
      static float g_MemStartY    = 0.8f;
      static float g_MemBarZ      = 0.1f;
      static float g_MemBarWidth  = 0.6f;
      static float g_MemBarHeight = 0.03f;

      Debug::PushTopLeftQuad({ g_MemStartX, g_MemStartY, g_MemBarZ }, g_MemBarWidth, g_MemBarHeight,
                             { 1, 1, 1, 1 });

      char AllocInfoString[64];
      char AllocBlockSizeString[32];
      for(int i = 0; i < GameState->HeapAllocator.m_AllocCount; i++)
      {
        Memory::allocation_info* AllocInfo =
          &GameState->HeapAllocator.m_AllocInfos[-GameState->HeapAllocator.m_AllocCount + i];

        size_t  BlockOffset      = AllocInfo->Base - GameState->HeapAllocator.m_Base;
        int32_t BlockSize        = AllocInfo->Size;
        float   OffsetPercentage = (float)BlockOffset / (float)GameState->HeapAllocator.m_Capacity;
        float   SizePercentage   = (float)BlockSize / (float)GameState->HeapAllocator.m_Capacity;

        sprintf(AllocBlockSizeString, "%d", BlockSize);
        UI::DrawTextBox(GameState,
                        { g_MemStartX + OffsetPercentage * g_MemBarWidth, g_MemStartY, g_MemBarZ },
                        SizePercentage * g_MemBarWidth, g_MemBarHeight, AllocBlockSizeString,
                        { 0.7f, 0.1f, 0.2f, 1 }, { 0.1f, 0.1f, 0.1f, 1 });

        size_t InfoOffset          = (uint8_t*)AllocInfo - GameState->HeapAllocator.m_Base;
        float InfoOffsetPercentage = (float)InfoOffset / (float)GameState->HeapAllocator.m_Capacity;
        float InfoSizePercentage =
          (float)sizeof(Memory::allocation_info) / (float)GameState->HeapAllocator.m_Capacity;

        DrawBox(GameState,
                { g_MemStartX + InfoOffsetPercentage * g_MemBarWidth, g_MemStartY, g_MemBarZ },
                InfoSizePercentage * g_MemBarWidth, g_MemBarHeight, { 1, 0, 0, 1 },
                { 0.1f, 0.1f, 0.1f, 1 });

        sprintf(AllocInfoString, "delete Base: %ld, Size: %d, Align: %u, InfoS: %ld", BlockOffset,
                BlockSize, AllocInfo->AlignmentOffset, InfoOffset);

        UI::Row(&Layout);
        if(UI::ReleaseButton(GameState, &Layout, Input, AllocInfoString, (void*)(uintptr_t)i))
        {
          GameState->HeapAllocator.Dealloc(AllocInfo->Base);
        }
      }
      Memory::free_block* CurrentBlock = GameState->HeapAllocator.m_FirstFreeBlock;
      while(CurrentBlock)
      {
        size_t  BlockOffset      = (uint8_t*)CurrentBlock - GameState->HeapAllocator.m_Base;
        int32_t BlockSize        = CurrentBlock->Size;
        float   OffsetPercentage = (float)BlockOffset / (float)GameState->HeapAllocator.m_Capacity;
        float   SizePercentage   = (float)BlockSize / (float)GameState->HeapAllocator.m_Capacity;
        sprintf(AllocBlockSizeString, "%d", BlockSize);
        UI::DrawTextBox(GameState,
                        { g_MemStartX + OffsetPercentage * g_MemBarWidth,
                          g_MemStartY - g_MemBarHeight, g_MemBarZ },
                        SizePercentage * g_MemBarWidth, g_MemBarHeight, AllocBlockSizeString,
                        { 0.3f, 0.6f, 0.4f, 1 }, { 0.1f, 0.1f, 0.1f, 1 });

        CurrentBlock = CurrentBlock->Next;
      }
      if(GameState->HeapAllocator.m_Base < GameState->HeapAllocator.m_Barrier)
      {
        size_t BlockSize = GameState->HeapAllocator.m_Barrier - GameState->HeapAllocator.m_Base;
        float   SizePercentage = (float)BlockSize / (float)GameState->HeapAllocator.m_Capacity;
        sprintf(AllocBlockSizeString, "%ld", BlockSize);
        UI::DrawTextBox(GameState, { g_MemStartX, g_MemStartY - 2 * g_MemBarHeight, g_MemBarZ },
                        SizePercentage * g_MemBarWidth, g_MemBarHeight, AllocBlockSizeString,
                        { 0.25f, 0.35f, 0.8f, 1 }, { 0.1f, 0.1f, 0.1f, 1 });
      }
    }
  }
#if 0
  char FrameRateString[20];
  sprintf(FrameRateString, "%.2f ms", (double)Input->dt * 1000.0);
  UI::Row(&Layout);
  UI::ReleaseButton(GameState, &Layout, Input, FrameRateString);
#endif
}
