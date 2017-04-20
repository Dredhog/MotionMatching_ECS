#include "game.h"
#include "ui.h"
#include "debug_drawing.h"
#include "render_data.h"
#include "skeleton.h"

#include <limits>

char*
VoidPtrToCharPtr(void* NamePtr)
{
  return (char*)(*(char**)NamePtr);
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

void
DrawAndInteractWithEditorUI(game_state* GameState, const game_input* Input)
{
  // Humble beginnings of the editor GUI system
  const float   TEXT_HEIGHT    = 0.03f;
  const float   StartX         = 0.75f;
  const float   StartY         = 0.97f;
  const float   YPadding       = 0.02f;
  const float   LayoutWidth    = 0.15f;
  const float   RowHeight      = 0.035f;
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
  static bool  g_ShowGUISettings        = false;
  static float g_ScrollK                = 0.0f;

  UI::im_layout Layout = UI::NewLayout({ StartX, StartY }, LayoutWidth, RowHeight, SliderWidth);

  if(UI::_ExpandableButton(&Layout, Input, "Material Editor", &g_ShowMaterialEditor))
  {
    UI::Row(GameState, &Layout, 2, "Material");
    if(UI::ReleaseButton(GameState, &Layout, Input, "Previous", "Material"))
    {
      if(0 < GameState->CurrentMaterial)
      {
        --GameState->CurrentMaterial;
      }
    }
    if(UI::ReleaseButton(GameState, &Layout, Input, "Next", "Material"))
    {
      if(GameState->CurrentMaterial < GameState->R.MaterialCount - 1)
      {
        ++GameState->CurrentMaterial;
      }
    }
    UI::DrawSquareTexture(GameState, &Layout, GameState->IDTexture);

    assert(GameState->R.MaterialCount > 0);
    material* CurrentMaterial = &GameState->R.Materials[GameState->CurrentMaterial];

    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Apply To Selected"))
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

    material* Material = &GameState->R.Materials[GameState->CurrentMaterial];
    UI::Row(GameState, &Layout, 1, "Blending");
    UI::_BoolButton(&Layout, Input, "Toggle", &CurrentMaterial->Common.UseBlending);
    switch(Material->Common.ShaderType)
    {
      case SHADER_Phong:
      {
        UI::Row(GameState, &Layout, 2, "Albedo");
        if(UI::ReleaseButton(GameState, &Layout, Input, "Prev"))
        {
          if(CurrentMaterial->Phong.DiffuseMapIndex > 0)
          {
            --CurrentMaterial->Phong.DiffuseMapIndex;
          }
        }
        if(UI::ReleaseButton(GameState, &Layout, Input, "Next  "))
        {
          if(CurrentMaterial->Phong.DiffuseMapIndex < GameState->R.TextureCount)
          {
            ++CurrentMaterial->Phong.DiffuseMapIndex;
          }
        }
        UI::Row(GameState, &Layout, 1, "Ambient");
        UI::SliderFloat(GameState, &Layout, Input, "Amb", &Material->Phong.AmbientStrength, 0, 1.0f,
                        5.0f);
        UI::Row(GameState, &Layout, 1, "Specular");
        UI::SliderFloat(GameState, &Layout, Input, "Dif", &Material->Phong.SpecularStrength, 0,
                        1.0f, 5.0f);
      }
      break;
      // TODO(Rytis): Fix bug involving specular map and different shader slider value change
      case SHADER_LightMapPhong:
      {
        UI::Row(GameState, &Layout, 1, "Diffuse");
        {
          int32_t ActiveDiffuseMapIndex = CurrentMaterial->LightMapPhong.DiffuseMapIndex;
          UI::ComboBox(&ActiveDiffuseMapIndex, GameState->R.TextureNames, GameState->R.TextureCount,
                       GameState, &Layout, Input, 0.2f, &g_ScrollK, sizeof(texture_name),
                       VoidPtrToTextureName);
          CurrentMaterial->LightMapPhong.DiffuseMapIndex = ActiveDiffuseMapIndex;
        }

        UI::Row(GameState, &Layout, 1, "Specular");
        {
          int32_t ActiveSpecularMapIndex = CurrentMaterial->LightMapPhong.SpecularMapIndex;
          UI::ComboBox(&ActiveSpecularMapIndex, GameState->R.TextureNames,
                       GameState->R.TextureCount, GameState, &Layout, Input, 0.4f, &g_ScrollK,
                       sizeof(texture_name), VoidPtrToTextureName);
          CurrentMaterial->LightMapPhong.SpecularMapIndex = ActiveSpecularMapIndex;
        }

        UI::Row(GameState, &Layout, 1, "Shininess");
        UI::SliderFloat(GameState, &Layout, Input, "Shi", &Material->LightMapPhong.Shininess, 0,
                        1.0f, 5.0f);
      }
      break;
      case SHADER_Color:
      {
        UI::Row(GameState, &Layout, 4, "Color");
        UI::SliderFloat(GameState, &Layout, Input, "R", &Material->Color.Color.R, 0, 1.0f, 5.0f);
        UI::SliderFloat(GameState, &Layout, Input, "G", &Material->Color.Color.G, 0, 1.0f, 5.0f);
        UI::SliderFloat(GameState, &Layout, Input, "B", &Material->Color.Color.B, 0, 1.0f, 5.0f);
        UI::SliderFloat(GameState, &Layout, Input, "A", &Material->Color.Color.A, 0, 1.0f, 5.0f);
      }
      break;
    }
    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Reset Material"))
    {
      uint32_t ShaderType                = CurrentMaterial->Common.ShaderType;
      *CurrentMaterial                   = {};
      CurrentMaterial->Common.ShaderType = ShaderType;
    }
    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Create New"))
    {
      AddMaterial(&GameState->R, {});
      GameState->CurrentMaterial = GameState->R.MaterialCount - 1;
    }
    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Crete From Current"))
    {
      AddMaterial(&GameState->R, *CurrentMaterial);
      GameState->CurrentMaterial = GameState->R.MaterialCount - 1;
    }
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Render Settings", &g_ShowDisplaySet))
  {
    UI::Row(GameState, &Layout, 6, "Toggleables");
    UI::_BoolButton(&Layout, Input, "Toggle Timeline", &GameState->DrawTimeline);
    UI::_BoolButton(&Layout, Input, "Toggle Cubemap", &GameState->DrawCubemap);
    UI::_BoolButton(&Layout, Input, "Toggle Wireframe", &GameState->DrawWireframe);
    UI::_BoolButton(&Layout, Input, "Toggle Gizmos", &GameState->DrawGizmos);
    UI::_BoolButton(&Layout, Input, "Toggle BWeights", &GameState->DrawBoneWeights);
    UI::_BoolButton(&Layout, Input, "Toggle Spinning", &GameState->IsModelSpinning);
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
  if(UI::_ExpandableButton(&Layout, Input, "Entity Creation", &g_ShowEntityDrowpown))
  {
    UI::Row(GameState, &Layout, 2, "model");
    if(UI::ReleaseButton(GameState, &Layout, Input, "Previous", "model"))
    {
      if(0 < GameState->CurrentModel)
      {
        --GameState->CurrentModel;
      }
    }
    if(UI::ReleaseButton(GameState, &Layout, Input, "Next", "model"))
    {
      if(GameState->CurrentModel < GameState->R.ModelCount - 1)
      {
        ++GameState->CurrentModel;
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
    }
  }
  entity* SelectedEntity = {};
  if(GetSelectedEntity(GameState, &SelectedEntity))
  {
#if 1
    Anim::transform* Transform     = &SelectedEntity->Transform;
    mat4             Mat4Transform = TransformToGizmoMat4(Transform);
    DEBUGPushGizmo(&GameState->Camera, &Mat4Transform);
#else
    Anim::transform* Transform =
      &GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
         .Transforms[GameState->AnimEditor.CurrentBone];
#endif
    UI::Row(&Layout);
    if(UI::_ExpandableButton(&Layout, Input, "Transform", &g_ShowTranslationButtons))
    {

      UI::Row(GameState, &Layout, 3, "Translation");
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Translation.X, -INFINITY,
                      INFINITY, 20.0f);
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Translation.Y, -INFINITY,
                      INFINITY, 20.0f);
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Translation.Z, -INFINITY,
                      INFINITY, 20.0f);
      UI::Row(GameState, &Layout, 3, "Rotation");
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Rotation.X, -360, 360.0f,
                      720.0f);
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Rotation.Y, -360, 360.0f,
                      720.0f);
      UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Rotation.Z, -360, 360.0f,
                      720.0f);
      UI::Row(GameState, &Layout, 3, "Scale  ");
      UI::SliderFloat(GameState, &Layout, Input, "Scale X", &Transform->Scale.X, 0, 100,
                      10.0f * ClampFloat(0.01f, AbsFloat(Transform->Scale.X), 10));
      UI::SliderFloat(GameState, &Layout, Input, "Scale Y", &Transform->Scale.Y, 0, 100,
                      10.0f * ClampFloat(0.01f, AbsFloat(Transform->Scale.X), 10));
      UI::SliderFloat(GameState, &Layout, Input, "Scale Z", &Transform->Scale.Z, 0, 100,
                      10.0f * ClampFloat(0.01f, AbsFloat(Transform->Scale.X), 10));
    }
  }
  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "Animation Editor", &g_ShowAnimSetings))
  {
    UI::Row(GameState, &Layout, 1, "Bone");
    {
      int32_t ActiveBoneIndex = GameState->AnimEditor.CurrentBone;
      UI::ComboBox(&ActiveBoneIndex, GameState->AnimEditor.Skeleton->Bones,
                   GameState->AnimEditor.Skeleton->BoneCount, GameState, &Layout, Input, 0.2f,
                   &g_ScrollK, sizeof(Anim::bone), ElementToBoneName);
      EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, ActiveBoneIndex);
    }

    UI::Row(GameState, &Layout, 1, "Playhead Time");
    UI::SliderFloat(GameState, &Layout, Input, "Playhead Time", &GameState->AnimEditor.PlayHeadTime,
                    -100, 100, 2.0f);
    EditAnimation::AdvancePlayHead(&GameState->AnimEditor, 0);

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
    UI::Row(&Layout);
    if(UI::ReleaseButton(GameState, &Layout, Input, "Export Animation"))
    {
      Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor,
                                  "data/animation_export_test");
    }
  }

  UI::Row(&Layout);
  if(UI::_ExpandableButton(&Layout, Input, "GUI Parameters", &g_ShowGUISettings))
  {
    UI::Row(GameState, &Layout, 3, "Border");
    UI::SliderFloat(GameState, &Layout, Input, "R", &g_BorderColor.R, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "G", &g_BorderColor.G, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "B", &g_BorderColor.B, 0, 1.0f, 5.0f);
    //UI::SliderFloat(GameState, &Layout, Input, "A", &g_BorderColor.A, 0, 1.0f, 5.0f);
    UI::Row(GameState, &Layout, 3, "Base");
    UI::SliderFloat(GameState, &Layout, Input, "R", &g_NormalColor.R, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "G", &g_NormalColor.G, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "B", &g_NormalColor.B, 0, 1.0f, 5.0f);
    //UI::SliderFloat(GameState, &Layout, Input, "A", &g_NormalColor.A, 0, 1.0f, 5.0f);
    UI::Row(GameState, &Layout, 3, "Highlight");
    UI::SliderFloat(GameState, &Layout, Input, "R", &g_HighlightColor.R, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "G", &g_HighlightColor.G, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "B", &g_HighlightColor.B, 0, 1.0f, 5.0f);
    //UI::SliderFloat(GameState, &Layout, Input, "A", &g_HighlightColor.A, 0, 1.0f, 5.0f);
    UI::Row(GameState, &Layout, 3, "Pressed");
    UI::SliderFloat(GameState, &Layout, Input, "R", &g_PressedColor.R, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "G", &g_PressedColor.G, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "B", &g_PressedColor.B, 0, 1.0f, 5.0f);
    //UI::SliderFloat(GameState, &Layout, Input, "A", &g_PressedColor.A, 0, 1.0f, 5.0f);
    UI::Row(GameState, &Layout, 3, "BoolPressed");
    UI::SliderFloat(GameState, &Layout, Input, "R", &g_BoolPressedColor.R, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "G", &g_BoolPressedColor.G, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "B", &g_BoolPressedColor.B, 0, 1.0f, 5.0f);
    //UI::SliderFloat(GameState, &Layout, Input, "A", &g_BoolPressedColor.A, 0, 1.0f, 5.0f);
    UI::Row(GameState, &Layout, 3, "Bool Base");
    UI::SliderFloat(GameState, &Layout, Input, "R", &g_BoolNormalColor.R, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "G", &g_BoolNormalColor.G, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "B", &g_BoolNormalColor.B, 0, 1.0f, 5.0f);
    //UI::SliderFloat(GameState, &Layout, Input, "A", &g_BoolNormalColor.A, 0, 1.0f, 5.0f);
    UI::Row(GameState, &Layout, 3, "Bool Highlight");
    UI::SliderFloat(GameState, &Layout, Input, "R", &g_BoolHighlightColor.R, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "G", &g_BoolHighlightColor.G, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "B", &g_BoolHighlightColor.B, 0, 1.0f, 5.0f);
    //UI::SliderFloat(GameState, &Layout, Input, "A", &g_BoolHighlightColor.A, 0, 1.0f, 5.0f);
    UI::Row(GameState, &Layout, 3, "Description bg");
    UI::SliderFloat(GameState, &Layout, Input, "R", &g_DescriptionColor.R, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "G", &g_DescriptionColor.G, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "B", &g_DescriptionColor.B, 0, 1.0f, 5.0f);
    //UI::SliderFloat(GameState, &Layout, Input, "A", &g_DescriptionColor.A, 0, 1.0f, 5.0f);
    UI::Row(GameState, &Layout, 3, "Font");
    UI::SliderFloat(GameState, &Layout, Input, "R", &g_FontColor.R, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "G", &g_FontColor.G, 0, 1.0f, 5.0f);
    UI::SliderFloat(GameState, &Layout, Input, "B", &g_FontColor.B, 0, 1.0f, 5.0f);
    //UI::SliderFloat(GameState, &Layout, Input, "A", &g_FontColor.A, 0, 1.0f, 5.0f);
  }

#if 0
    UI::Row(&Layout, 1, PlayHead);
    UI::SliderFloat(GameState, &Layout, Input, "Rotation", &Transform->Translation.X, -INFINITY,
                    INFINITY, 20.0f);
#elif 0

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
                            KeyframeWidth, 0.05f, { 0.5f, 0.3f, 0.3f, 1 });
    }
    float PlayheadPercentage = (Editor->PlayHeadTime - FirstTime) / TimeDiff;
    DEBUGDrawCenteredQuad(GameState,
                          vec3{ TimelineStartX + PlayheadPercentage * TimelineWidth,
                                TimelineStartY + TimelineHeight * 0.5f },
                          KeyframeWidth, 0.05f, { 1, 0, 0, 1 });
    float CurrentPercentage = (Editor->SampleTimes[Editor->CurrentKeyframe] - FirstTime) / TimeDiff;
    DEBUGDrawCenteredQuad(GameState,
                          vec3{ TimelineStartX + CurrentPercentage * TimelineWidth,
                                TimelineStartY + TimelineHeight * 0.5f },
                          0.003f, 0.05f, { 0.5f, 0.5f, 0.5f, 1 });
  }
}
