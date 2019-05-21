#pragma once
#include "string.h"
#include "skeleton.h"
#include "file_io.h"

const char* PathArrayToString(const void* Data, int Index);
const char* BoneArrayToString(const void* Data, int Index);

#define TEMPLATE_NAME_MAX_LENGTH 100

// Note(Lukas) the Params have to have the names for this to export correctly
inline void
ExportAndSetMMController(Memory::stack_allocator* Alloc, Resource::resource_manager* Resources,
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

  size_t AlignmentSize = (uint8_t*)MMControllerAsset - (uint8_t*)MMControllerAssetStart.Address;
  assert(0 <= AlignmentSize && AlignmentSize < 64);
  size_t MMControllerAssetSize =
    Alloc->GetByteCountAboveMarker(MMControllerAssetStart) - AlignmentSize;

  MMControllerAsset->Params.AnimPaths.HardClear();
  // Fetch the animation names
  for(int i = 0; i < MMControllerAsset->Params.AnimRIDs.Count; i++)
  {
    int PathIndex = Resources->GetAnimationPathIndex(MMControllerAsset->Params.AnimRIDs[i]);
    MMControllerAsset->Params.AnimPaths.Push(Resources->AnimationPaths[PathIndex]);
  }

  Resources->UpdateOrCreateMMController(MMControllerAsset, MMControllerAssetSize, FileName);

  Asset::PackMMController(MMControllerAsset);
  Platform::WriteEntireFile(FileName, MMControllerAssetSize, MMControllerAsset);

  Alloc->FreeToMarker(MMControllerAssetStart);
}

void
MMControllerEditorGUI(mm_profile_editor* MMEditor, Memory::stack_allocator* TempStack,
                      Resource::resource_manager* Resources)
{
  static int  TargetPathIndex                  = -1;
  static bool TargetIsTemplate                 = true;
  static bool s_AnimationDropdown              = false;
  static bool s_MirrorInfoDropdown             = false;
  static bool s_GeneralParametersDropdown      = false;
  static bool s_TargetSkeletonDropdown         = false;
  static bool s_SkeletalHieararchyDropdown     = false;
  static bool s_SkeletonMirrorInfoDropdown     = false;
  static bool s_MatchingPointSelectionDropdown = false;

  const float LeftOfComboButtonWidth              = 150;
  const float ComboRightEdgeDistanceFromRightSize = 200;
  // THE LOAD/EXTRACT TEMPLATE BUTTON
  const char* ButtonText = (TargetPathIndex == -1) ? "New Template" : "Load";
  if(UI::Button(ButtonText, LeftOfComboButtonWidth))
  {
    // UPDATING MMEditor.ActiveProfile
    MMEditor->ActiveProfile = MMEditor->SelectedProfile;
  }

  // UPDATING MMEditor.SelectedProfile
  {
    UI::SameLine();
    int NewTargetPathIndex = TargetPathIndex;
    UI::PushWidth(-ComboRightEdgeDistanceFromRightSize);
    UI::Combo("Target File", &NewTargetPathIndex, Resources->MMParamPaths,
              Resources->MMParamPathCount, PathArrayToString);
    UI::PopWidth();
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

  // Set skeleton if not already set. Otherwise give a red "switch target" button
  if(MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount <= 0)
  {
    bool           ClickedSetSkeleton   = UI::Button("Set skeleton", LeftOfComboButtonWidth);
    static int32_t ActiveModelPathIndex = -1;
    {
      UI::SameLine();
      UI::PushWidth(-ComboRightEdgeDistanceFromRightSize);
      UI::Combo("Target Skeleton", &ActiveModelPathIndex, Resources->ModelPaths,
                Resources->ModelPathCount, PathArrayToString);
      UI::PopWidth();
    }
    if(ClickedSetSkeleton && ActiveModelPathIndex != -1)
    {
      rid TargetModelRID =
        Resources->ObtainModelPathRID(Resources->ModelPaths[ActiveModelPathIndex].Name);
      Render::model* TargetModel = Resources->GetModel(TargetModelRID);
      if(TargetModel->Skeleton)
      {
        ResetMMParamsToDefault(&MMEditor->ActiveProfile);

        memcpy(&MMEditor->ActiveProfile.FixedParams.Skeleton, TargetModel->Skeleton,
               sizeof(Anim::skeleton));
        GenerateSkeletonMirroringInfo(&MMEditor->ActiveProfile.DynamicParams.MirrorInfo,
                                      &MMEditor->ActiveProfile.FixedParams.Skeleton);
      }
    }
  }
  else
  {
    if(UI::CollapsingHeader("Skeleton", &s_TargetSkeletonDropdown))
    {
      if(UI::TreeNode("Skeletal Hierarchy", &s_SkeletalHieararchyDropdown))
      {
        int ParentIndex = -1;
        for(int i = 0; i < MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount; i++)
        {
          while(ParentIndex != MMEditor->ActiveProfile.FixedParams.Skeleton.Bones[i].ParentIndex)
          {
            ParentIndex =
              MMEditor->ActiveProfile.FixedParams.Skeleton.Bones[ParentIndex].ParentIndex;
            UI::Unindent();
          }
          // bool Open = false;
          // UI::TreeNode(MMEditor->ActiveProfile.FixedParams.Skeleton.Bones[i].Name, &Open);
					UI::Indent();
          UI::Text(MMEditor->ActiveProfile.FixedParams.Skeleton.Bones[i].Name);
          ParentIndex = i;
        }
        while(ParentIndex != -1)
        {
          ParentIndex = MMEditor->ActiveProfile.FixedParams.Skeleton.Bones[ParentIndex].ParentIndex;
          UI::Unindent();
        }
        UI::TreePop();
      }
      if(UI::TreeNode("Mirror Info", &s_SkeletonMirrorInfoDropdown))
      {
        int RemovedPairIndex = -1;
        UI::PushWidth(260);
        for(int i = 0; i < MMEditor->ActiveProfile.DynamicParams.MirrorInfo.PairCount; i++)
        {
          int& IndA = MMEditor->ActiveProfile.DynamicParams.MirrorInfo.BoneMirrorIndices[i].a;
          int& IndB = MMEditor->ActiveProfile.DynamicParams.MirrorInfo.BoneMirrorIndices[i].b;

          UI::PushID(&IndA);
          if(UI::Button("Remove"))
          {
            RemovedPairIndex = i;
          }
          UI::SameLine();
          UI::Combo("Left", &IndA, MMEditor->ActiveProfile.FixedParams.Skeleton.Bones,
                    MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount, BoneArrayToString, 5);
          UI::SameLine();
          UI::Combo("Right", &IndB, MMEditor->ActiveProfile.FixedParams.Skeleton.Bones,
                    MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount, BoneArrayToString, 5);
          UI::PopID();
        }
        UI::PopWidth();
        if(RemovedPairIndex != -1)
        {
          stack_handle<Anim::int32_pair> TempRemovalStackHandle;
          TempRemovalStackHandle
            .Init(MMEditor->ActiveProfile.DynamicParams.MirrorInfo.BoneMirrorIndices,
                  MMEditor->ActiveProfile.DynamicParams.MirrorInfo.PairCount,
                  MMEditor->ActiveProfile.DynamicParams.MirrorInfo.PairCount);
          TempRemovalStackHandle.Remove(RemovedPairIndex);
          MMEditor->ActiveProfile.DynamicParams.MirrorInfo.PairCount = TempRemovalStackHandle.Count;
        }
        if(UI::Button("Regenerate Pairs", LeftOfComboButtonWidth))
        {
          GenerateSkeletonMirroringInfo(&MMEditor->ActiveProfile.DynamicParams.MirrorInfo,
                                        &MMEditor->ActiveProfile.FixedParams.Skeleton);
        }
        UI::TreePop();
      }
    }

    if(UI::CollapsingHeader("Bones To Match", &s_MatchingPointSelectionDropdown))
    {
      bool ClickedAddBone = UI::Button("Add Bone", LeftOfComboButtonWidth);
      UI::SameLine();
      static int32_t ActiveBoneIndex = 0;
      UI::PushWidth(-ComboRightEdgeDistanceFromRightSize);
      UI::Combo("Bone", &ActiveBoneIndex, MMEditor->ActiveProfile.FixedParams.Skeleton.Bones,
                MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount, BoneArrayToString);
      UI::PopWidth();

      if(ClickedAddBone && !MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices.Full())
      {
        MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices.Push(ActiveBoneIndex);
      }

      for(int i = 0; i < MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices.Count; i++)
      {
        UI::PushID(&MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices.Elements[i]);
        bool RemoveCurrent = UI::Button("Remove");
        UI::SameLine();
        {
          UI::Text(MMEditor->ActiveProfile.FixedParams.Skeleton
                     .Bones[MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices[i]]
                     .Name);
        }
        if(RemoveCurrent)
        {
          MMEditor->ActiveProfile.FixedParams.ComparisonBoneIndices.Remove(i);
        }
        UI::PopID();
      }
    }

    if(UI::CollapsingHeader("Matching Parameters", &s_GeneralParametersDropdown))
    {
			UI::Text("Cost Computation Parameters");
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
			static bool s_ShowTrajectoryPointWeights;
      if(UI::TreeNode("Trajectory Point Weights", &s_ShowTrajectoryPointWeights))
			{
				for(int i = 0; i < MM_POINT_COUNT; i++)
        {
          char TempBuff[32];
          snprintf(TempBuff, ArrayCount(TempBuff), "Point #%d Weight", i + 1);
          UI::Text(TempBuff);
          UI::SliderFloat(TempBuff, &MMEditor->ActiveProfile.DynamicParams.TrajectoryWeights[i], 0, 1);
        }
        UI::TreePop();
			}
      UI::Text("Metadata Generation Parameters");
      UI::SliderFloat("Metadata Sampling Frequency",
                      &MMEditor->ActiveProfile.FixedParams.MetadataSamplingFrequency, 15, 240);
      UI::SliderFloat("Trajectory Time Horizon",
                      &MMEditor->ActiveProfile.DynamicParams.TrajectoryTimeHorizon, 0.0f, 5.0f);
      UI::Text("Playback Parameters");
      UI::SliderFloat("BlendInTime", &MMEditor->ActiveProfile.DynamicParams.BlendInTime, 0, 1);
      UI::SliderFloat("Min Time Offset Threshold",
                      &MMEditor->ActiveProfile.DynamicParams.MinTimeOffsetThreshold, 0, 1);
      UI::Checkbox("Match MirroredAnimations",
                   &MMEditor->ActiveProfile.DynamicParams.MatchMirroredAnimations);
    }
    if(UI::CollapsingHeader("Animations", &s_AnimationDropdown))
    {
      {
        bool ClickedAddAnimation = UI::Button("Add Animation", LeftOfComboButtonWidth);

        UI::SameLine();
        static int32_t ActivePathIndex = -1;
        UI::PushWidth(-ComboRightEdgeDistanceFromRightSize);
        UI::Combo("Animation", &ActivePathIndex, Resources->AnimationPaths,
                  Resources->AnimationPathCount, PathArrayToString);
        UI::PopWidth();
        if(ActivePathIndex >= 0 && ClickedAddAnimation)
        {
          rid NewRID =
            Resources->ObtainAnimationPathRID(Resources->AnimationPaths[ActivePathIndex].Name);
          if(!MMEditor->ActiveProfile.AnimRIDs.Full() &&
             Resources->GetAnimation(NewRID)->ChannelCount ==
               MMEditor->ActiveProfile.FixedParams.Skeleton.BoneCount)
          {
            MMEditor->ActiveProfile.AnimRIDs.Push(NewRID);
          }
        }
      }
      {
        for(int i = 0; i < MMEditor->ActiveProfile.AnimRIDs.Count; i++)
        {
          UI::PushID(&MMEditor->ActiveProfile.AnimRIDs.Elements[i]);
          bool RemoveCurrent = UI::Button("Remove");
          UI::SameLine();
          {
            char* Path;
            if(Resources->Animations.Get(MMEditor->ActiveProfile.AnimRIDs[i], NULL, &Path))
            {
              UI::Text(Path);
            }
          }
          if(RemoveCurrent)
          {
            MMEditor->ActiveProfile.AnimRIDs.Remove(i);
            i--;
          }
          else // NOTE(Lukas) this branch is only expected to be taken if the path array changes
               // behind our back e.g. when reloading the scene
          {
            UI::NewLine();
          }
          UI::PopID();
        }
      }
    }

    if(UI::Button("Save Template", LeftOfComboButtonWidth))
    {
      MMEditor->ActiveProfile.AnimPaths.HardClear();
      // Set the paths from the animation RIDs
      for(int i = 0; i < MMEditor->ActiveProfile.AnimRIDs.Count; i++)
      {
        int PathIndex = Resources->GetAnimationPathIndex(MMEditor->ActiveProfile.AnimRIDs[i]);
        MMEditor->ActiveProfile.AnimPaths.Push(Resources->AnimationPaths[PathIndex]);
      }
      Asset::ExportMMParams(&MMEditor->ActiveProfile, "data/controllers/test.template");
      MMEditor->SelectedProfile = MMEditor->ActiveProfile;
    }
    if(0 < MMEditor->ActiveProfile.AnimRIDs.Count)
    {
      UI::SameLine();
      if(UI::Button("Build Controller", LeftOfComboButtonWidth))
      {
        ExportAndSetMMController(TempStack, Resources, &MMEditor->ActiveProfile,
                                 "data/controllers/test.controller");
      }
    }
    else
    {
      // UI::Dummy("Build MM data");
    }
  }
}
