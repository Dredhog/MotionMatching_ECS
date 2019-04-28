#pragma once
#include "string.h"
#include "skeleton.h"
#include "file_io.h"

const char* PathArrayToString(const void* Data, int Index);
const char* BoneArrayToString(const void* Data, int Index);

#define TEMPLATE_NAME_MAX_LENGTH 100

// Note(Lukas) the Params have to have the names for this to export correctly
inline void
BuildAndExportMMController(Memory::stack_allocator* Alloc, Resource::resource_manager* Resources,
                           const mm_params* Params, const char* FileName)
{
  // Fetch the animation pointers
  fixed_stack<Anim::animation*, MM_ANIM_CAPACITY> Animations = {};
  for(int i = 0; i < Params->AnimRIDs.Count; i++)
  {
    Animations.Push(Resources->GetAnimation(Params->AnimRIDs[i]));
  }

  Memory::marker MMControllerAssetStart = Alloc->GetMarker();

  mm_controller_data* MMControllerAsset =
    PrecomputeRuntimeMMData(Alloc, Animations.GetArrayHandle(), *Params);
  // assert(MMControllerAssetStart.Address == (uint8_t*)MMControllerAsset);

  size_t MMControllerAssetSize = Alloc->GetByteCountAboveMarker(MMControllerAssetStart);

  MMControllerAsset->Params.AnimPaths.HardClear();
  // Fetch the animation names
  for(int i = 0; i < MMControllerAsset->Params.AnimRIDs.Count; i++)
  {
    int PathIndex = Resources->GetAnimationPathIndex(MMControllerAsset->Params.AnimRIDs[i]);
    MMControllerAsset->Params.AnimPaths.Push(Resources->AnimationPaths[PathIndex]);
  }

  Asset::PackMMController(MMControllerAsset);
  Platform::WriteEntireFile(FileName, MMControllerAssetSize, MMControllerAsset);
}

void
MMControllerEditorGUI(mm_profile_editor* MMEditor, Memory::stack_allocator* TempStack,
                      Resource::resource_manager* Resources)
{
  static int  TargetPathIndex                                    = -1;
  static bool TargetIsTemplate                                   = true;
  static bool s_AnimationDropdown                                = false;
  static bool s_MirrorInfoDropdown                               = false;
  static bool s_GeneralParametersDropdown                        = false;
  static bool s_TargetSkeletonDropdown                           = false;
  static bool s_SkeletalHieararchyDropdown                       = false;
  static bool s_SkeletonMirrorInfoDropdown                       = false;
  static bool s_MatchingPointSelectionDropdown                   = false;

  // UPDATING MMEditor.SelectedProfile
  {
    int NewTargetPathIndex = TargetPathIndex;
    UI::Combo("Target File", &NewTargetPathIndex, Resources->MMParamPaths,
              Resources->MMParamPathCount, PathArrayToString);
    if(NewTargetPathIndex != TargetPathIndex)
    {
      if(NewTargetPathIndex != -1)
      {
        // TODO Determine the target extension ".controller/.template" and assign result to
        // TargetIsTemplate
        char* ControllerString =
          strstr(Resources->MMParamPaths[NewTargetPathIndex].Name, ".controller");
        char* TemplateString =
          strstr(Resources->MMParamPaths[NewTargetPathIndex].Name, ".template");

        assert(!(ControllerString && TemplateString) &&
               (ControllerString || TemplateString)); // Path assumed to not have two extensions

        TargetIsTemplate = (TemplateString);
        if(TargetIsTemplate)
        {
          Asset::ImportMMParams(TempStack, &MMEditor->SelectedProfile,
                                Resources->MMParamPaths[NewTargetPathIndex].Name);

          // Set the animation RIDs from the paths
          MMEditor->SelectedProfile.AnimRIDs.HardClear();
          for(int i = 0; i < MMEditor->SelectedProfile.AnimPaths.Count; i++)
          {
            MMEditor->SelectedProfile.AnimRIDs.Push(
              Resources->ObtainAnimationPathRID(MMEditor->SelectedProfile.AnimPaths[i].Name));
          }
        }
        else
        {
          rid MMControllerRID =
            Resources->ObtainMMControllerPathRID(Resources->MMParamPaths[NewTargetPathIndex].Name);
          mm_controller_data* MMController = Resources->GetMMController(MMControllerRID);
          MMEditor->SelectedProfile        = MMController->Params;
        }
      }
      else
      {
        ResetMMParamsToDefault(&MMEditor->SelectedProfile);
      }
    }
    TargetPathIndex = NewTargetPathIndex;
  }

  // THE LOAD/EXTRACT TEMPLATE BUTTON
  const char* ButtonText = (TargetIsTemplate) ? "Load Template   " : "Extract Template";
  if(UI::Button(ButtonText))
  {
    // UPDATING MMEditor.ActiveProfile
    MMEditor->ActiveProfile = MMEditor->SelectedProfile;
  }

  // Set skeleton if not already set. Otherwise give a red "switch target" button
  if(MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount <= 0)
  {
    static int32_t ActiveModelPathIndex = -1;
    UI::Combo("Target Skeleton", &ActiveModelPathIndex, Resources->ModelPaths,
              Resources->ModelPathCount, PathArrayToString);
    if(ActiveModelPathIndex != -1)
    {
      rid TargetModelRID =
        Resources->ObtainModelPathRID(Resources->ModelPaths[ActiveModelPathIndex].Name);
      Render::model* TargetModel = Resources->GetModel(TargetModelRID);
      if(TargetModel->Skeleton)
      {
        memcpy(&MMEditor->ActiveProfile.FixedParams.Skeleton, TargetModel->Skeleton,
               sizeof(Anim::skeleton));
        GenerateSkeletonMirroringInfo(&MMEditor->ActiveProfile.DynamicParams.MirrorInfo,
                                      &MMEditor->ActiveProfile.FixedParams.Skeleton);
      }
    }
  }
  else
  {
    if(UI::CollapsingHeader("Target Skeleton", &s_TargetSkeletonDropdown))
    {
      if(UI::CollapsingHeader("Skeletal Hierarchy", &s_SkeletalHieararchyDropdown))
      {
        for(int i = 0; i < MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount; i++)
        {
          UI::Text(MMEditor->ActiveProfile.FixedParams.Skeleton.Bones[i].Name);
        }
      }
      if(UI::CollapsingHeader("Mirror Info", &s_SkeletonMirrorInfoDropdown))
      {
        char TempBuff[3 * BONE_NAME_LENGTH];
        for(int i = 0; i < MMEditor->ActiveProfile.DynamicParams.MirrorInfo.PairCount; i++)
        {
          int IndA = MMEditor->ActiveProfile.DynamicParams.MirrorInfo.BoneMirrorIndices[i].a;
          int IndB = MMEditor->ActiveProfile.DynamicParams.MirrorInfo.BoneMirrorIndices[i].b;
          snprintf(TempBuff, sizeof(TempBuff), "{%s %s}",
                   MMEditor->ActiveProfile.FixedParams.Skeleton.Bones[IndA].Name,
                   MMEditor->ActiveProfile.FixedParams.Skeleton.Bones[IndB].Name);
          UI::Text(TempBuff);
        }
      }
    }
    if(UI::CollapsingHeader("General parameters", &s_GeneralParametersDropdown))
    {
      UI::SliderFloat("Bone Position Influence",
                      &MMEditor->ActiveProfile.DynamicParams.BonePCoefficient, 0, 5);
      UI::SliderFloat("Bone Velocity Influence",
                      &MMEditor->ActiveProfile.DynamicParams.BoneVCoefficient, 0, 5);
      UI::SliderFloat("Trajectory Position Influence",
                      &MMEditor->ActiveProfile.DynamicParams.TrajPCoefficient, 0, 1);
      UI::SliderFloat("Trajectory Velocity Influence",
                      &MMEditor->ActiveProfile.DynamicParams.TrajVCoefficient, 0, 1);
      UI::SliderFloat("Trajectory Angle Influence",
                      &MMEditor->ActiveProfile.DynamicParams.TrajAngleCoefficient, 0, 1);
      UI::SliderFloat("BlendInTime", &MMEditor->ActiveProfile.DynamicParams.BelndInTime, 0, 1);
      UI::SliderFloat("Min Time Offset Threshold",
                      &MMEditor->ActiveProfile.DynamicParams.MinTimeOffsetThreshold, 0, 1);
      UI::Checkbox("Match MirroredAnimations",
                   &MMEditor->ActiveProfile.DynamicParams.MatchMirroredAnimations);

      UI::SliderFloat("Metadata Sampling Frequency",
                      &MMEditor->ActiveProfile.FixedParams.MetadataSamplingFrequency, 15, 240);
      UI::SliderFloat("Trajectory Time Horizon",
                      &MMEditor->ActiveProfile.DynamicParams.TrajectoryTimeHorizon, 0.0f, 5.0f);
    }
    if(UI::CollapsingHeader("Animations", &s_AnimationDropdown))
    {
      {
        static int32_t ActivePathIndex = 0;
        UI::Combo("Animation", &ActivePathIndex, Resources->AnimationPaths,
                  Resources->AnimationPathCount, PathArrayToString);
        rid NewRID = { 0 };
        if(Resources->AnimationPathCount > 0 &&
           !Resources->GetAnimationPathRID(&NewRID,
                                           Resources->AnimationPaths[ActivePathIndex].Name))
        {
          NewRID = Resources->RegisterAnimation(Resources->AnimationPaths[ActivePathIndex].Name);
        }

        if(UI::Button("Add Animation") && !MMEditor->ActiveProfile.AnimRIDs.Full() &&
           Resources->GetAnimation(NewRID)->ChannelCount ==
             MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount)
        {
          MMEditor->ActiveProfile.AnimRIDs.Push(NewRID);
        }
      }
      {
        for(int i = 0; i < MMEditor->ActiveProfile.AnimRIDs.Count; i++)
        {
          bool DeleteCurrent = UI::Button("Delete", 0, i);
          UI::SameLine();
          {
            char* Path;
            Resources->Animations.Get(MMEditor->ActiveProfile.AnimRIDs[i], NULL, &Path);
            UI::Text(Path);
          }
          UI::NewLine();
          if(DeleteCurrent)
          {
            MMEditor->ActiveProfile.AnimRIDs.Remove(i);
            i--;
          }
        }
      }
    }

    if(UI::CollapsingHeader("Points For Matching", &s_MatchingPointSelectionDropdown))
    {
      static int32_t ActiveBoneIndex = 0;
      UI::Combo("Bone", &ActiveBoneIndex, MMEditor->ActiveProfile.FixedParams.Skeleton.Bones,
                MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount, BoneArrayToString);

      if(UI::Button("Add Bone") &&
         !MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices.Full())
      {
        MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices.Push(ActiveBoneIndex);
      }

      for(int i = 0; i < MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices.Count; i++)
      {
        bool DeleteCurrent = UI::Button("Delete", 0, 111 + i);
        UI::SameLine();
        {
          UI::Text(MMEditor->ActiveProfile.FixedParams.Skeleton
                     .Bones[MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices[i]]
                     .Name);
        }
        UI::NewLine();
        if(DeleteCurrent)
        {
          MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices.Remove(i);
        }
      }
    }

    if(UI::Button("Save Template"))
    {
      MMEditor->ActiveProfile.AnimPaths.HardClear();
      // Set the paths from the animation RIDs
      for(int i = 0; i < MMEditor->ActiveProfile.AnimRIDs.Count; i++)
      {
        int PathIndex = Resources->GetAnimationPathIndex(MMEditor->ActiveProfile.AnimRIDs[i]);
        MMEditor->ActiveProfile.AnimPaths.Push(Resources->AnimationPaths[PathIndex]);
      }
      Asset::ExportMMParams(&MMEditor->ActiveProfile, "data/matching_params/test.template");
      MMEditor->SelectedProfile = MMEditor->ActiveProfile;
    }
    if(0 < MMEditor->ActiveProfile.AnimRIDs.Count)
    {
      UI::SameLine();
      if(UI::Button("Build Controller"))
      {
        BuildAndExportMMController(TempStack, Resources, &MMEditor->ActiveProfile,
                                   "data/matching_params/test.controller");
      }
      UI::NewLine();
    }
    else
    {
      // UI::Dummy("Build MM data");
    }
  }
}
