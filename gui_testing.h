#include "ui.h"
#include "scene.h"
#include "shader_def.h"
#include "profile.h"
#include <cstdlib>
#include "blend_stack.h"

void MaterialGUI(game_state* GameState, bool& ShowMaterialEditor);
void EntityGUI(game_state* GameState, bool& ShowEntityTools);
void AnimationGUI(game_state* GameState, bool& ShowAnimationEditor, bool& ShowEntityTools);
void MiscGUI(game_state* GameState, bool& ShowLightSettings, bool& ShowDisplaySet,
             bool& ShowCameraSettings, bool& ShowSceneSettings, bool& ShowPostProcessingSettings,
             bool& ShowECDData);

char*
PathArrayToString(void* Data, int Index)
{
  path* Paths = (path*)Data;
  return Paths[Index].Name;
}

char*
BoneArrayToString(void* Data, int Index)
{
  Anim::bone* Bones = (Anim::bone*)Data;
  return Bones[Index].Name;
}

int
AllocationInfoComparison(const void* A, const void* B)
{
  Memory::allocation_info* AllocInfoA = (Memory::allocation_info*)A;
  Memory::allocation_info* AllocInfoB = (Memory::allocation_info*)B;
  return AllocInfoA->Base > AllocInfoB->Base;
}

namespace UI
{
  void
  TestGui(game_state* GameState, const game_input* Input)
  {
    BEGIN_TIMED_BLOCK(GUI);
    UI::BeginFrame(GameState, Input);

    static bool s_ShowDemoWindow           = false;
    static bool s_ShowPhysicsWindow        = false;
    static bool s_ShowProfilerWindow       = false;
    static bool s_ShowMotionMatchingWindow = false;

    UI::BeginWindow("Editor Window", { 1200, 50 }, { 700, 600 });
    {
      {
        //char TempBuffer[32];
        //sprintf(TempBuffer, "dt: %.4f", Input->dt);
        //UI::Text(TempBuffer);
      }
      UI::Combo("Selection mode", (int32_t*)&GameState->SelectionMode, g_SelectionEnumStrings,
                SELECT_EnumCount, UI::StringArrayToString);

      static bool s_ShowMaterialEditor         = false;
      static bool s_ShowEntityTools            = false;
      static bool s_ShowAnimationEditor        = false;
      static bool s_ShowLightSettings          = false;
      static bool s_ShowDisplaySet             = false;
      static bool s_ShowCameraSettings         = false;
      static bool s_ShowSceneSettings          = false;
      static bool s_ShowPostProcessingSettings = false;
      static bool s_ShowECSData                = false;

      UI::Checkbox("Use Hot Reloading", &GameState->UseHotReloading);
      UI::Checkbox("Update Path List", &GameState->UpdatePathList);
      UI::Checkbox("Profiler Window", &s_ShowProfilerWindow);
      UI::Checkbox("Physics Window", &s_ShowPhysicsWindow);
      UI::Checkbox("GUI Params Window", &s_ShowDemoWindow);
      UI::Checkbox("Motion Matching", &s_ShowMotionMatchingWindow);
      EntityGUI(GameState, s_ShowEntityTools);
      MaterialGUI(GameState, s_ShowMaterialEditor);
      AnimationGUI(GameState, s_ShowAnimationEditor, s_ShowEntityTools);
      MiscGUI(GameState, s_ShowLightSettings, s_ShowDisplaySet, s_ShowCameraSettings,
              s_ShowSceneSettings, s_ShowPostProcessingSettings, s_ShowECSData);
    }
    UI::EndWindow();

    if(s_ShowProfilerWindow)
    {
      UI::BeginWindow("Profiler Window", { 150, 500 }, { 1500, 500 });
#ifdef USE_DEBUG_PROFILING
      {
        int PreviousFrameIndex =
          (g_CurrentProfilerFrameIndex + PROFILE_MAX_FRAME_COUNT - 1) % PROFILE_MAX_FRAME_COUNT;
        static bool s_AllowCurrentFrameChoice      = false;
        static bool s_ShowTimelineRegion           = false;
        static bool s_ShowFrameSummaries           = false;
        static bool s_ShowGPUFrameSummaries        = false;
        static bool s_ShowEntityEditor             = true;
        static bool s_ShowChunkMemoryVisualization = true;

        static int s_CurrentModifiableFrameIndex = 0;

        bool Changed = s_AllowCurrentFrameChoice;
        UI::Checkbox("Inspect Specific Frame", &s_AllowCurrentFrameChoice);
        Changed = Changed != s_AllowCurrentFrameChoice;
        if(s_AllowCurrentFrameChoice)
        {
          UI::SameLine();
          if(UI::Button("Previous Frame"))
          {
            s_CurrentModifiableFrameIndex =
              (s_CurrentModifiableFrameIndex + PROFILE_MAX_FRAME_COUNT - 1) %
              PROFILE_MAX_FRAME_COUNT;
          }
          UI::SameLine();
          if(UI::Button("Next Frame"))
          {
            s_CurrentModifiableFrameIndex =
              (s_CurrentModifiableFrameIndex + PROFILE_MAX_FRAME_COUNT + 1) %
              PROFILE_MAX_FRAME_COUNT;
          }
          UI::NewLine();
        }

        s_CurrentModifiableFrameIndex =
          (s_AllowCurrentFrameChoice) ? s_CurrentModifiableFrameIndex : PreviousFrameIndex;

        if(Changed)
        {
          if(s_AllowCurrentFrameChoice)
          {
            s_CurrentModifiableFrameIndex = PreviousFrameIndex;
            PAUSE_PROFILE();
          }
          else
          {
            RESUME_PROFILE();
          }
        }

        if(s_AllowCurrentFrameChoice)
        {
          UI::SliderInt("Current Frame", &s_CurrentModifiableFrameIndex, 0,
                        PROFILE_MAX_FRAME_COUNT - 1);
        }
        else
        {
          // int temp = g_CurrentProfilerFrameIndex;
          // UI::SliderInt("Current Frame", &PreviousFrameIndex, 0, PROFILE_MAX_FRAME_COUNT-1);
          // g_CurrentProfilerFrameIndex = temp;
        }

#if 0
				{
					const vec3 StartPosition = {0, 1, 0};
					const float MaxProfileWidth = 0.5f;
					const float StackBlockHeight = 0.05f;
					const frame_endpoints FrameCycleCounter = GLOBAL_FRAME_ENDPOINT_TABLE[s_CurrentModifiableFrameIndex];
					const float BaselineCycleCount = 5e7;//(float)(FrameCycleCounter.FrameEnd - FrameCycleCounter.FrameStart);
					for(int i = 0; i < GLOBAL_TIMER_FRAME_EVENT_COUNT_TABLE[s_CurrentModifiableFrameIndex]; i++)
					{
						timer_event CurrentEvent = GLOBAL_FRAME_TIMER_EVENT_TABLE[s_CurrentModifiableFrameIndex][i];
						float ElementWidth = (MaxProfileWidth/BaselineCycleCount)*(float)(CurrentEvent.EndCycleCount - CurrentEvent.StartCycleCount);
						float ElementLeft = (MaxProfileWidth/BaselineCycleCount)*(CurrentEvent.StartCycleCount-FrameCycleCounter.FrameStart);
						float ElementTop = StackBlockHeight * (float)CurrentEvent.EventDepth;

						//Note(Lukas): Big Gocha the alpha value means the StencilValue and should be >= window depth
						int StencilValue = 1;
						vec3 EventColor = DEBUG_ENTRY_COLORS[CurrentEvent.NameTableIndex];
						Debug::PushTopLeftQuad(StartPosition + vec3{ElementLeft, -ElementTop,0}, ElementWidth, StackBlockHeight, vec4{EventColor.R, EventColor.G, EventColor.B, (float)StencilValue});
					}
				}
#else
        static int s_BlockIndexForSummary = 0;

        if(UI::CollapsingHeader("Frame Timeline", &s_ShowTimelineRegion))
        {
          static float s_TimelineZoom = 1;
          UI::SliderFloat("Timeline Zoom", &s_TimelineZoom, 0, 10);

          float AvailableWidth = UI::GetAvailableWidth();
          float ChildPadding   = 30.0f;
          UI::Dummy(ChildPadding);
          UI::SameLine();

          UI::PushStyleColor(UI::COLOR_WindowBackground, vec4{ 0.4f, 0.4f, 0.5f, 0.3f });
          UI::BeginChildWindow("Profile Timeline Window",
                               { AvailableWidth - ChildPadding * 2, 200 });
          {
            UI::PushStyleVar(UI::VAR_BoxPaddingX, 1);
            UI::PushStyleVar(UI::VAR_BoxPaddingY, 1);
            UI::PushStyleVar(UI::VAR_SpacingX, 0);
            UI::PushStyleVar(UI::VAR_SpacingY, 1);
            {
              const float MaxProfileWidth = (0.5f * s_TimelineZoom) * UI::GetWindowWidth();
              const frame_endpoints FrameCycleCounter =
                GLOBAL_FRAME_ENDPOINT_TABLE[s_CurrentModifiableFrameIndex];
              const float BaselineCycleCount =
                5e6; //(float)(FrameCycleCounter.FrameEnd - FrameCycleCounter.FrameStart);
              for(int j = 0; j < 5; j++)
              {
                float CurrentHorizontalPosition = 0.0f;
                for(int i = 0;
                    i < GLOBAL_TIMER_FRAME_EVENT_COUNT_TABLE[s_CurrentModifiableFrameIndex]; i++)
                {
                  timer_event CurrentEvent =
                    GLOBAL_FRAME_TIMER_EVENT_TABLE[s_CurrentModifiableFrameIndex][i];
                  if(CurrentEvent.EventDepth == j)
                  {
                    float EventWidth =
                      (MaxProfileWidth / BaselineCycleCount) *
                      (float)(CurrentEvent.EndCycleCount - CurrentEvent.StartCycleCount);
                    float EventLeft = (MaxProfileWidth / BaselineCycleCount) *
                                      (CurrentEvent.StartCycleCount - FrameCycleCounter.FrameStart);

                    const float* EventColor = &TIMER_UI_COLOR_TABLE[CurrentEvent.NameTableIndex][0];
                    UI::PushStyleColor(UI::COLOR_ButtonNormal,
                                       vec4{ EventColor[0], EventColor[1], EventColor[2], 1 });
                    {
                      float DummyWidth = EventLeft - CurrentHorizontalPosition;
                      UI::Dummy(EventLeft - CurrentHorizontalPosition);
                      UI::SameLine();
                      if(UI::Button(TIMER_NAME_TABLE[CurrentEvent.NameTableIndex], EventWidth))
                      {
                        s_BlockIndexForSummary = CurrentEvent.NameTableIndex;
                      }
                      UI::SameLine();
                      CurrentHorizontalPosition += DummyWidth + EventWidth;
                    }
                    UI::PopStyleColor();
                  }
                }
                UI::NewLine();
              }
            }
            UI::PopStyleVar();
            UI::PopStyleVar();
            UI::PopStyleVar();
            UI::PopStyleVar();
          }
          UI::EndChildWindow();
          UI::PopStyleColor();
          UI::NewLine();
          {
            {
              UI::Text(TIMER_NAME_TABLE[s_BlockIndexForSummary]);
              UI::SameLine();
              {
                char CountBuffer[40];
                sprintf(CountBuffer, ": %lu",
                        GLOBAL_TIMER_FRAME_SUMMARY_TABLE[s_CurrentModifiableFrameIndex]
                                                        [s_BlockIndexForSummary]
                                                          .CycleCount);
                UI::Text(CountBuffer);
              }
              UI::NewLine();
            }
          }
        }
#endif

        if(UI::CollapsingHeader("Frame Event Summaries", &s_ShowFrameSummaries))
        {
          for(int i = 0; i < ArrayCount(TIMER_NAME_TABLE); i++)
          {
            UI::Text(TIMER_NAME_TABLE[i]);
            UI::SameLine();
            {
              char CountBuffer[40];
              sprintf(CountBuffer, ": %lu",
                      GLOBAL_TIMER_FRAME_SUMMARY_TABLE[s_CurrentModifiableFrameIndex][i]
                        .CycleCount);
              UI::Text(CountBuffer);
            }
            UI::NewLine();
          }
        }

        if(UI::CollapsingHeader("GPU Frame Event Summaries", &s_ShowGPUFrameSummaries))
        {
          for(int i = 0; i < ARRAY_COUNT(GPU_TIMER_NAME_TABLE); i++)
          {
            UI::Text(GPU_TIMER_NAME_TABLE[i]);
            UI::SameLine();
            {
              char CountBuffer[40];
              sprintf(CountBuffer, ": %fms",
                      GPU_TIMER_EVENT_TABLE[s_CurrentModifiableFrameIndex][i].ElapsedTime /
                        (float)1e6);
              UI::Text(CountBuffer);
            }
            UI::NewLine();
          }
        }
        {
          Memory::marker EntityEditorMemStart = GameState->TemporaryMemStack->GetMarker();
          const int      TempBufferCapacity   = 64;
          char* TempBuffer = PushArray(GameState->TemporaryMemStack, TempBufferCapacity, char);

          if(UI::CollapsingHeader("ECS Entity Editor", &s_ShowEntityEditor))
          {
            static int32_t SelectedEntityID = -1;

            UI::SliderInt("Selected Entity ID", &SelectedEntityID, 0,
                          MaxInt32(1, GameState->ECSWorld->Entities.Count - 1));

            if(UI::Button("Create New Entity"))
            {

              SelectedEntityID = (int32_t)CreateEntity(GameState->ECSWorld);
            }

            if(DoesEntityExist(GameState->ECSWorld, (entity_id)SelectedEntityID))
            {
              UI::SameLine();
              if(UI::Button("Destroy Entity"))
              {
                DestroyEntity(GameState->ECSWorld, (entity_id)SelectedEntityID);
                SelectedEntityID = -1;
              }
              UI::NewLine();
            }

            if(DoesEntityExist(GameState->ECSWorld, (entity_id)SelectedEntityID))
            {
              static int32_t NewComponentID = -1;

              if(NewComponentID != -1)
              {
                if(!HasComponent(GameState->ECSWorld, (entity_id)SelectedEntityID,
                                 (component_id)NewComponentID))
                {
                  if(UI::Button("Add Component   "))
                  {

                    AddComponent(GameState->ECSWorld, (entity_id)SelectedEntityID,
                                 (component_id)NewComponentID);
                  }
                }
                else if(UI::Button("Remove Component"))
                {
                  RemoveComponent(GameState->ECSWorld, (entity_id)SelectedEntityID,
                                  (component_id)NewComponentID);
                }
              }

              UI::Combo("Component Type", (int32_t*)&NewComponentID,
                        &GameState->ECSRuntime->ComponentNames.Elements,
                        GameState->ECSRuntime->ComponentNames.Count, UI::StringArrayToString, 6,
                        200);

              snprintf(TempBuffer, TempBufferCapacity, "Chunk Index   : %d",
                       GameState->ECSWorld->Entities[(entity_id)SelectedEntityID].ChunkIndex);
              UI::Text(TempBuffer);
              snprintf(TempBuffer, TempBufferCapacity, "Index In Chunk: %d",
                       GameState->ECSWorld->Entities[(entity_id)SelectedEntityID].IndexInChunk);
              UI::Text(TempBuffer);
            }
          }
          if(UI::CollapsingHeader("Chunk Memory", &s_ShowChunkMemoryVisualization))
          {
            Memory::heap_allocator&  ChunkHeap      = GameState->ECSRuntime->ChunkHeap;
            chunk*                   HeapBase       = (chunk*)ChunkHeap.GetBase();
            Memory::allocation_info* RawAllocInfos  = ChunkHeap.GetAllocationInfos();
            int32_t                  AllocInfoCount = ChunkHeap.GetAllocationCount();

            Memory::allocation_info* SortedAllocInfos =
              PushArray(GameState->TemporaryMemStack, AllocInfoCount, Memory::allocation_info);
            memcpy(SortedAllocInfos, RawAllocInfos,
                   AllocInfoCount * sizeof(Memory::allocation_info));
            qsort(SortedAllocInfos, (size_t)AllocInfoCount, sizeof(Memory::allocation_info),
                  &AllocationInfoComparison);

            const float    ChunkWidthInPixels = 150;
            static int32_t SelectedChunkIndex = -1;

            int CurrentBoxIndex = 0;
            for(int i = 0; i < AllocInfoCount; i++)
            {
              chunk*  Chunk      = (chunk*)SortedAllocInfos[i].Base;
              int32_t ChunkIndex = (int32_t)(Chunk - HeapBase);

              for(int j = CurrentBoxIndex; j < ChunkIndex; j++)
              {
                UI::Dummy(ChunkWidthInPixels);
                UI::SameLine();
              }

              snprintf(TempBuffer, TempBufferCapacity, "Chunk #%d", ChunkIndex);
              {
                const float* EventColor = &TIMER_UI_COLOR_TABLE[Chunk->Header.ArchetypeIndex][0];
                UI::PushStyleColor(UI::COLOR_ButtonNormal,
                                   vec4{ EventColor[0], EventColor[1], EventColor[2], 1 });
                if(UI::Button(TempBuffer, ChunkWidthInPixels))
                {
                  SelectedChunkIndex = ChunkIndex;
                }
                UI::SameLine();
                UI::PopStyleColor();
              }

              CurrentBoxIndex++;
            }
            UI::NewLine();

            bool FoundSelected = false;
            for(int i = 0; i < AllocInfoCount; i++)
            {
              chunk*  Chunk      = (chunk*)SortedAllocInfos[i].Base;
              int32_t ChunkIndex = (int32_t)(Chunk - HeapBase);
              if(ChunkIndex == SelectedChunkIndex)
              {
                FoundSelected = true;
              }
            }

            if(FoundSelected)
            {
              chunk*       Chunk  = &HeapBase[SelectedChunkIndex];
              chunk_header Header = Chunk->Header;

              // Output Chunk details
              {
                snprintf(TempBuffer, TempBufferCapacity, "Archetype Index : %d",
                         Header.ArchetypeIndex);
                UI::Text(TempBuffer);

                snprintf(TempBuffer, TempBufferCapacity, "Entity Capacity : %d",
                         Header.EntityCapacity);
                UI::Text(TempBuffer);

                snprintf(TempBuffer, TempBufferCapacity, "Entity Count    : %d",
                         Header.EntityCount);
                UI::Text(TempBuffer);

                int32_t NextChunkIndex =
                  (Header.NextChunk != 0) ? (int32_t)(Header.NextChunk - HeapBase) : -1;
                snprintf(TempBuffer, TempBufferCapacity, "Next Chunk Index: %d", NextChunkIndex);
                UI::Text(TempBuffer);
              }

              // Visualize individual chunk
              {
                const float TotalChunkWidth = UI::GetWindowWidth();
                const float PixelsPerByte   = TotalChunkWidth / (float)sizeof(chunk);

                archetype* Archetype = &GameState->ECSRuntime->Archetypes[Header.ArchetypeIndex];

                UI::NewLine();

                float CurrentPos  = 0;
                float HeaderWidth = PixelsPerByte * (float)sizeof(chunk_header);
                UI::Button("Header", HeaderWidth);
                UI::SameLine();
                CurrentPos += HeaderWidth;
                for(int i = 0; i < Archetype->ComponentTypes.Count; i++)
                {
                  component_id_and_offset ComponentOffset = Archetype->ComponentTypes[i];
                  component_struct_info   ComponentInfo =
                    GameState->ECSRuntime->ComponentStructInfos[ComponentOffset.ID];

                  float ComponentOffsetInPixels = PixelsPerByte * (float)ComponentOffset.Offset;
                  float AlignmentWidth          = ComponentOffsetInPixels - CurrentPos;

                  Dummy(AlignmentWidth);
                  UI::SameLine();

                  float ComponentWidth =
                    PixelsPerByte * (float)(Header.EntityCount * ComponentInfo.Size);
                  UI::Button(GameState->ECSRuntime->ComponentNames[ComponentOffset.ID],
                             ComponentWidth);
                  UI::SameLine();
                }
                UI::NewLine();
              }

              // Output archetype details
              {
                UI::NewLine();

                archetype* Archetype = &GameState->ECSRuntime->Archetypes[Header.ArchetypeIndex];

                for(int i = 0; i < Archetype->ComponentTypes.Count; i++)
                {
                  component_id_and_offset ComponentOffset = Archetype->ComponentTypes[i];
                  component_struct_info   ComopnentInfo =
                    GameState->ECSRuntime->ComponentStructInfos[ComponentOffset.ID];

                  UI::Text(GameState->ECSRuntime->ComponentNames[ComponentOffset.ID]);

                  snprintf(TempBuffer, TempBufferCapacity,
                           "ID: %d, Offset: %d; Size: %d; Alignment: %d", ComponentOffset.ID,
                           ComponentOffset.Offset, ComopnentInfo.Size, ComopnentInfo.Alignment);
                  UI::Text(TempBuffer);
                  UI::NewLine();
                }
              }
            }
            else
            {
              SelectedChunkIndex = -1;
            }
          }
          GameState->TemporaryMemStack->FreeToMarker(EntityEditorMemStart);
        }
      }
#endif // USE_DEBUG_PROFILING
      UI::EndWindow();
    }

    if(s_ShowPhysicsWindow)
    {
      UI::BeginWindow("Physics Window", { 150, 50 }, { 500, 380 });
      {
        physics_params&   Params   = GameState->Physics.Params;
        physics_switches& Switches = GameState->Physics.Switches;
        UI::Checkbox("Simulating Dynamics", &Switches.SimulateDynamics);
        UI::SliderInt("Iteration Count", &Params.PGSIterationCount, 0, 250);
        UI::SliderFloat("Beta", &Params.Beta, 0.0f, 1.0f / (FRAME_TIME_MS / 1000.0f));
        Switches.PerformDynamicsStep = UI::Button("Step Dynamics");
        UI::Checkbox("Gravity", &Switches.UseGravity);
        UI::Checkbox("Friction", &Switches.SimulateFriction);
        UI::SliderFloat("Mu", &Params.Mu, 0.0f, 1.0f);

        UI::Checkbox("Draw Omega    (green)", &Switches.VisualizeOmega);
        UI::Checkbox("Draw V        (yellow)", &Switches.VisualizeV);
        UI::Checkbox("Draw Fc       (red)", &Switches.VisualizeFc);
        UI::Checkbox("Draw Friction (green)", &Switches.VisualizeFriction);
        UI::Checkbox("Draw Fc Comopnents     (Magenta)", &Switches.VisualizeFcComponents);
        UI::Checkbox("Draw Contact Points    (while)", &Switches.VisualizeContactPoints);
        UI::Checkbox("Draw Contact Manifolds (blue/red)", &Switches.VisualizeContactManifold);
        UI::DragFloat3("Net Force Start", &Params.ExternalForceStart.X, -INFINITY, INFINITY, 5);
        UI::DragFloat3("Net Force Vector", &Params.ExternalForce.X, -INFINITY, INFINITY, 5);
        UI::Checkbox("Apply Force", &Switches.ApplyExternalForce);
        UI::Checkbox("Apply Torque", &Switches.ApplyExternalTorque);
      }
      UI::EndWindow();
    }

    if(s_ShowDemoWindow)
    {
      UI::BeginWindow("window A", { 670, 50 }, { 500, 380 });
      {
        static int         s_CurrentItem = -1;
        static const char* s_Items[]     = { "Cat", "Rat", "Hat", "Pat", "meet", "with", "dad" };

        static bool s_ShowDemo = true;
        if(UI::CollapsingHeader("Demo", &s_ShowDemo))
        {
          static bool s_Checkbox0 = false;
          static bool s_Checkbox1 = false;

          {
            gui_style& Style     = *UI::GetStyle();
            int32_t    Thickness = (int32_t)Style.Vars[UI::VAR_BorderThickness];
            UI::SliderInt("Border Thickness ", &Thickness, 0, 10);
            Style.Vars[UI::VAR_BorderThickness] = Thickness;

            UI::Text("Hold ctrl when dragging to snap to whole values");
            UI::DragFloat4("Window background", &Style.Colors[UI::COLOR_WindowBackground].X, 0, 1,
                           5);
            UI::DragFloat4("Header Normal", &Style.Colors[UI::COLOR_HeaderNormal].X, 0, 1, 5);
            UI::DragFloat4("Header Hovered", &Style.Colors[UI::COLOR_HeaderHovered].X, 0, 1, 5);
            UI::DragFloat4("Header Pressed", &Style.Colors[UI::COLOR_HeaderPressed].X, 0, 1, 5);
            UI::SliderFloat("Horizontal Padding", &Style.Vars[UI::VAR_BoxPaddingX], 0, 10);
            UI::SliderFloat("Vertical Padding", &Style.Vars[UI::VAR_BoxPaddingY], 0, 10);
            UI::SliderFloat("Horizontal Spacing", &Style.Vars[UI::VAR_SpacingX], 0, 10);
            UI::SliderFloat("Vertical Spacing", &Style.Vars[UI::VAR_SpacingY], 0, 10);
            UI::SliderFloat("Internal Spacing", &Style.Vars[UI::VAR_InternalSpacing], 0, 10);
          }
          UI::Combo("Combo test", &s_CurrentItem, s_Items, ARRAY_SIZE(s_Items), 5, 150);
          int StartIndex = 3;
          UI::Combo("Combo test1", &s_CurrentItem, s_Items + StartIndex,
                    ARRAY_SIZE(s_Items) - StartIndex);
          UI::NewLine();

          char TempBuff[30];
          snprintf(TempBuff, sizeof(TempBuff), "Wheel %d", Input->MouseWheelScreen);
          UI::Text(TempBuff);

          snprintf(TempBuff, sizeof(TempBuff), "Mouse Screen: { %d, %d }", Input->MouseScreenX,
                   Input->MouseScreenY);
          UI::Text(TempBuff);
          snprintf(TempBuff, sizeof(TempBuff), "Mouse Normal: { %.1f, %.1f }", Input->NormMouseX,
                   Input->NormMouseY);
          UI::Text(TempBuff);
          snprintf(TempBuff, sizeof(TempBuff), "delta time: %f ms", Input->dt);
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

            UI::Image("material preview", GameState->IDTexture, { 400, 220 });

            if(s_Checkbox1)
            {
              UI::EndChildWindow();
            }
          }
        }
      }
      UI::EndWindow();
    }

    if(s_ShowMotionMatchingWindow)
    {
      UI::BeginWindow("Motion Matching", { 20, 20 }, { 500, 500 });
      {
        UI::SliderFloat("Trajectory Duration (sec)", &GameState->MMDebug.TrajectoryDuration, 0, 10);
        UI::SliderInt("Trajectory Sample Count", &GameState->MMDebug.TrajectorySampleCount, 2, 40);
        UI::SliderFloat("Player Speed (m/s)", &GameState->PlayerSpeed, 0, 10);
        UI::SliderFloat("Position Coefficient", &GameState->MMParams.DynamicParams.PosCoefficient,
                        0, 2);
        UI::SliderFloat("Velocity Coefficient", &GameState->MMParams.DynamicParams.VelCoefficient,
                        0, 2);
        UI::SliderFloat("Trajectory Coefficient",
                        &GameState->MMParams.DynamicParams.TrajCoefficient, 0, 2);
        UI::SliderFloat("BlendInTime", &GameState->MMParams.DynamicParams.BelndInTime, 0, 2);
        UI::SliderFloat("Min Time Offset Threshold",
                        &GameState->MMParams.DynamicParams.MinTimeOffsetThreshold, 0, 2);

        UI::Checkbox("Match MirroredAnimations",
                     &GameState->MMParams.DynamicParams.MatchMirroredAnimations);
				UI::Text("Debug Display");
        UI::Checkbox("Show Root Trajectory", &GameState->MMDebug.ShowRootTrajectories);
        UI::Checkbox("Show Hip Trajectory", &GameState->MMDebug.ShowHipTrajectories);
        UI::Checkbox("Preview In Root Space", &GameState->MMDebug.PreviewInRootSpace);
        UI::Text("Current Goal");
        UI::Checkbox("Show Current Goal", &GameState->MMDebug.CurrentGoal.ShowTrajectory);
        UI::Checkbox("Show Current Positions", &GameState->MMDebug.CurrentGoal.ShowBonePositions);
        UI::Checkbox("Show Current Velocities", &GameState->MMDebug.CurrentGoal.ShowBoneVelocities);
        UI::Text("Matched Goal");
        UI::Checkbox("Show Matched Goal", &GameState->MMDebug.MatchedGoal.ShowTrajectory);
        UI::Checkbox("Show Matched Positions", &GameState->MMDebug.MatchedGoal.ShowBonePositions);
        UI::Checkbox("Show Matched Velocities", &GameState->MMDebug.MatchedGoal.ShowBoneVelocities);

        {
          static int32_t ActivePathIndex = 0;
          UI::Combo("Animation", &ActivePathIndex, GameState->Resources.AnimationPaths,
                    GameState->Resources.AnimationPathCount, PathArrayToString);
          rid NewRID = { 0 };
          if(GameState->Resources.AnimationPathCount > 0 &&
             !GameState->Resources
                .GetAnimationPathRID(&NewRID,
                                     GameState->Resources.AnimationPaths[ActivePathIndex].Name))
          {
            NewRID = GameState->Resources.RegisterAnimation(
              GameState->Resources.AnimationPaths[ActivePathIndex].Name);
          }

          if(UI::Button("Add Animation") && !GameState->MMParams.AnimRIDs.Full())
          {
            GameState->MMParams.AnimRIDs.Push(NewRID);
          }
        }
      }
      {
        for(int i = 0; i < GameState->MMParams.AnimRIDs.Count; i++)
        {
          bool DeleteCurrent = UI::Button("Delete", 0, i);
          UI::SameLine();
          {
            char* Path;
            GameState->Resources.Animations.Get(GameState->MMParams.AnimRIDs[i], NULL, &Path);
            UI::Text(Path);
          }
          UI::NewLine();
          if(DeleteCurrent)
          {
            GameState->MMParams.AnimRIDs.Remove(i);
            i--;
          }
        }
      }
      entity* SelectedEntity = {};
      if(GetSelectedEntity(GameState, &SelectedEntity) && SelectedEntity->AnimController)
      {
        static int32_t ActiveBoneIndex = 0;
        UI::Combo("Bone", &ActiveBoneIndex, SelectedEntity->AnimController->Skeleton->Bones,
                  SelectedEntity->AnimController->Skeleton->BoneCount, BoneArrayToString);

        if(UI::Button("Add Bone") && !GameState->MMParams.FixedParams.ComparisonBoneIndices.Full())
        {
          GameState->MMParams.FixedParams.ComparisonBoneIndices.Push(ActiveBoneIndex);
        }

        for(int i = 0; i < GameState->MMParams.FixedParams.ComparisonBoneIndices.Count; i++)
        {
          bool DeleteCurrent = UI::Button("Delete", 0, 111 + i);
          UI::SameLine();
          {
            UI::Text(SelectedEntity->AnimController->Skeleton
                       ->Bones[GameState->MMParams.FixedParams.ComparisonBoneIndices[i]]
                       .Name);
          }
          UI::NewLine();
          if(DeleteCurrent)
          {
            GameState->MMParams.FixedParams.ComparisonBoneIndices.Remove(i);
          }
        }

        if(0 < GameState->MMParams.AnimRIDs.Count)
        {
          UI::SliderFloat("Trajectory Time Horizon",
                          &GameState->MMParams.DynamicParams.TrajectoryTimeHorizon, 0.0f, 5.0f);
          if(GameState->PlayerEntityIndex == GameState->SelectedEntityIndex)
          {
            if(UI::Button("Build MM data"))
            {
              for(int i = 0; i < GameState->MMParams.AnimRIDs.Count; i++)
              {
                GameState->Resources.Animations.AddReference(GameState->MMParams.AnimRIDs[i]);
              }
              if(GameState->MMData.FrameInfos.IsValid())
              {
                Gameplay::ResetPlayer(SelectedEntity);
                for(int i = 0; i < GameState->MMData.Params.AnimRIDs.Count; i++)
                {
                  GameState->Resources.Animations.RemoveReference(
                    GameState->MMData.Params.AnimRIDs[i]);
                }
              }
              GameState->MMData =
                PrecomputeRuntimeMMData(GameState->TemporaryMemStack, &GameState->Resources,
                                        GameState->MMParams,
                                        SelectedEntity->AnimController->Skeleton);
            }
						
          }

					char TempBuffer[32];
          sprintf(TempBuffer, "g_BlendInfos.m_Count: %d", g_BlendInfos.m_Count);
          UI::Text(TempBuffer);
        }
      }
#if 0
			{
				static vec3 R = {};
        static vec3 T = {};

        UI::DragFloat3("Test Rotation (euler)", (float*)&R, -INFINITY, INFINITY, 5);
        quat Q = Math::EulerToQuat(R);
        UI::DragFloat4("Test Rotation  (quat)", (float*)&Q, -INFINITY, INFINITY, 5);
        UI::DragFloat3("Test Translation", (float*)&T, -INFINITY, INFINITY, 5);

        mat4 OriginalMatrix = Math::MulMat4(Math::Mat4Translate(T), Math::Mat4Rotate(Q));
				UI::Text("(Original Matrix)^T");
        UI::DragFloat4("X_h", (float*)&OriginalMatrix.X_h, -INFINITY, INFINITY, 5);
        UI::DragFloat4("Y_h", (float*)&OriginalMatrix.Y_h, -INFINITY, INFINITY, 5);
        UI::DragFloat4("Z_h", (float*)&OriginalMatrix.Z_h, -INFINITY, INFINITY, 5);
        UI::DragFloat4("T_h", (float*)&OriginalMatrix.T_h, -INFINITY, INFINITY, 5);

        quat ExtractedQ = Math::Mat4ToQuat(OriginalMatrix);
        vec3 ExtractedT = OriginalMatrix.T;
        vec3 ExtractedR = Math::QuatToEuler(ExtractedQ);

        UI::DragFloat3("Extracted Rotation (euler)", (float*)&ExtractedR, -INFINITY, INFINITY, 5);
        UI::DragFloat4("Extracted Rotation  (quat)", (float*)&ExtractedQ, -INFINITY, INFINITY, 5);
        UI::DragFloat3("Extracted Translation", (float*)&ExtractedT, -INFINITY, INFINITY, 5);
			}
#endif
      UI::EndWindow();
    }

    UI::EndFrame();
    END_TIMED_BLOCK(GUI);
  }
}

void
MaterialGUI(game_state* GameState, bool& ShowMaterialEditor)
{
  if(GameState->SelectionMode == SELECT_Mesh || GameState->SelectionMode == SELECT_Entity)
  {
    if(UI::CollapsingHeader("Material Editor", &ShowMaterialEditor))
    {
      {
        int32_t ActivePathIndex = 0;
        if(GameState->CurrentMaterialID.Value > 0)
        {
          ActivePathIndex = GameState->Resources.GetMaterialPathIndex(GameState->CurrentMaterialID);
        }
        UI::Combo("Material", &ActivePathIndex, GameState->Resources.MaterialPaths,
                  GameState->Resources.MaterialPathCount, PathArrayToString);
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
      if(0 < GameState->CurrentMaterialID.Value)
      {
        // Draw material preview to texture
        UI::Image("Material preview", GameState->IDTexture, { 500, 300 });

        material* CurrentMaterial = GameState->Resources.GetMaterial(GameState->CurrentMaterialID);

        // Select shader type
        {
          int32_t NewShaderType = CurrentMaterial->Common.ShaderType;
          UI::Combo("Shader Type", (int32_t*)&NewShaderType, g_ShaderTypeEnumStrings,
                    SHADER_EnumCount, UI::StringArrayToString, 6, 200);
          if(CurrentMaterial->Common.ShaderType != NewShaderType)
          {
            *CurrentMaterial                   = {};
            CurrentMaterial->Common.ShaderType = NewShaderType;
          }
        }

        UI::Checkbox("Blending", &CurrentMaterial->Common.UseBlending);
        UI::SameLine();
        UI::Checkbox("Skeletel", &CurrentMaterial->Common.IsSkeletal);
        UI::NewLine();

        if(CurrentMaterial->Common.ShaderType == SHADER_Phong)
        {
          if(CurrentMaterial->Common.IsSkeletal)
          {
            CurrentMaterial->Phong.Flags |= PHONG_UseSkeleton;
          }
          else
          {
            CurrentMaterial->Phong.Flags &= ~PHONG_UseSkeleton;
          }
        }

        switch(CurrentMaterial->Common.ShaderType)
        {
          case SHADER_Phong:
          {
            bool UseDIffuse      = (CurrentMaterial->Phong.Flags & PHONG_UseDiffuseMap);
            bool UseSpecular     = CurrentMaterial->Phong.Flags & PHONG_UseSpecularMap;
            bool NormalFlagValue = CurrentMaterial->Phong.Flags & PHONG_UseNormalMap;
            UI::Checkbox("Diffuse Map", &UseDIffuse);
            UI::Checkbox("Specular Map", &UseSpecular);
            UI::Checkbox("Normal Map", &NormalFlagValue);

            UI::DragFloat3("Ambient Color", &CurrentMaterial->Phong.AmbientColor.X, 0.0f, 1.0f,
                           5.0f);

            if(UseDIffuse)
            {
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

                  UI::Combo("Diffuse Map", &ActivePathIndex, GameState->Resources.TexturePaths,
                            GameState->Resources.TexturePathCount, PathArrayToString);
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
              UI::DragFloat4("Diffuse Color", &CurrentMaterial->Phong.DiffuseColor.X, 0.0f, 1.0f,
                             5.0f);
            }

            if(UseSpecular)
            {
              {
                int32_t ActivePathIndex = 0;
                if(CurrentMaterial->Phong.SpecularMapID.Value > 0)
                {
                  ActivePathIndex =
                    GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.SpecularMapID);
                }
                UI::Combo("Specular Map", &ActivePathIndex, GameState->Resources.TexturePaths,
                          GameState->Resources.TexturePathCount, PathArrayToString);
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
              UI::DragFloat3("Specular Color", &CurrentMaterial->Phong.SpecularColor.X, 0.0f, 1.0f,
                             5.0f);
            }

            if(NormalFlagValue)
            {
              {
                int32_t ActivePathIndex = 0;
                if(CurrentMaterial->Phong.NormalMapID.Value > 0)
                {
                  ActivePathIndex =
                    GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.NormalMapID);
                }
                UI::Combo("Normal map", &ActivePathIndex, GameState->Resources.TexturePaths,
                          GameState->Resources.TexturePathCount, PathArrayToString);
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

            UI::SliderFloat("Shininess", &CurrentMaterial->Phong.Shininess, 1.0f, 512.0f);
          }
          break;
          case SHADER_Env:
          {
            bool Refraction = CurrentMaterial->Env.Flags & ENV_UseRefraction;
            bool NormalMap  = CurrentMaterial->Env.Flags & ENV_UseNormalMap;
            UI::Checkbox("Refraction", &Refraction);
            UI::Checkbox("Normal Map", &NormalMap);

            if(Refraction)
            {
              CurrentMaterial->Env.Flags |= ENV_UseRefraction;
            }
            else
            {
              CurrentMaterial->Env.Flags &= ~ENV_UseRefraction;
            }

            if(NormalMap)
            {
              {
                int32_t ActivePathIndex = 0;
                if(CurrentMaterial->Env.NormalMapID.Value > 0)
                {
                  ActivePathIndex =
                    GameState->Resources.GetTexturePathIndex(CurrentMaterial->Env.NormalMapID);
                }
                UI::Combo("Normal map", &ActivePathIndex, GameState->Resources.TexturePaths,
                          GameState->Resources.TexturePathCount, PathArrayToString);
                if(GameState->Resources.TexturePathCount > 0)
                {
                  CurrentMaterial->Env.Flags |= ENV_UseNormalMap;

                  rid NewRID;
                  if(GameState->Resources
                       .GetTexturePathRID(&NewRID,
                                          GameState->Resources.TexturePaths[ActivePathIndex].Name))
                  {
                    CurrentMaterial->Env.NormalMapID = NewRID;
                  }
                  else
                  {
                    CurrentMaterial->Env.NormalMapID = GameState->Resources.RegisterTexture(
                      GameState->Resources.TexturePaths[ActivePathIndex].Name);
                  }
                  assert(CurrentMaterial->Env.NormalMapID.Value > 0);
                }
              }
            }
            else
            {
              CurrentMaterial->Env.Flags &= ~ENV_UseNormalMap;
            }

            UI::SliderFloat("Refractive Index", &CurrentMaterial->Env.RefractiveIndex, 1.0f, 3.0f);
          }
          break;
          case SHADER_Toon:
          {
            UI::DragFloat3("Ambient Color", &CurrentMaterial->Toon.AmbientColor.X, 0.0f, 1.0f,
                           5.0f);
            UI::DragFloat4("Diffuse Color", &CurrentMaterial->Toon.DiffuseColor.X, 0.0f, 1.0f,
                           5.0f);
            UI::DragFloat3("Specular Color", &CurrentMaterial->Toon.SpecularColor.X, 0.0f, 1.0f,
                           5.0f);

            UI::SliderFloat("Shininess", &CurrentMaterial->Toon.Shininess, 1.0f, 512.0f);
            UI::SliderInt("LevelCount", &CurrentMaterial->Toon.LevelCount, 1, 10);
          }
          break;
          case SHADER_LightWave:
          {
            UI::DragFloat3("Ambient Color", &CurrentMaterial->LightWave.AmbientColor.X, 0.0f, 1.0f,
                           5.0f);
            UI::DragFloat4("Diffuse Color", &CurrentMaterial->LightWave.DiffuseColor.X, 0.0f, 1.0f,
                           5.0f);
            UI::DragFloat3("Specular Color", &CurrentMaterial->LightWave.SpecularColor.X, 0.0f,
                           1.0f, 5.0f);

            UI::SliderFloat("Shininess", &CurrentMaterial->LightWave.Shininess, 1.0f, 512.0f);
          }
          break;
          case SHADER_Color:
          {
            UI::DragFloat4("Color", &CurrentMaterial->Color.Color.X, 0, 1, 5);
          }
          break;
          default:
          {
            struct shader_def* ShaderDef = NULL;
            assert(GetShaderDef(&ShaderDef, CurrentMaterial->Common.ShaderType));
            {
              named_shader_param_def ShaderParamDef = {};
              ResetShaderDefIterator(ShaderDef);
              while(GetNextShaderParam(&ShaderParamDef, ShaderDef))
              {
                void* ParamLocation =
                  (void*)(((uint8_t*)CurrentMaterial) + ShaderParamDef.OffsetIntoMaterial);
                switch(ShaderParamDef.Type)
                {
                  case SHADER_PARAM_TYPE_Int:
                  {
                    int32_t* ParamPtr = (int32_t*)ParamLocation;
                    UI::SliderInt(ShaderParamDef.Name, ParamPtr, 0, 10);
                  }
                  break;
                  case SHADER_PARAM_TYPE_Bool:
                  {
                    bool* ParamPtr = (bool*)ParamLocation;
                    UI::Checkbox(ShaderParamDef.Name, ParamPtr);
                  }
                  break;
                  case SHADER_PARAM_TYPE_Float:
                  {
                    float* ParamPtr = (float*)ParamLocation;
                    UI::DragFloat(ShaderParamDef.Name, ParamPtr, -100, 100, 2.0f);
                  }
                  break;
                  case SHADER_PARAM_TYPE_Vec3:
                  {
                    vec3* ParamPtr = (vec3*)ParamLocation;
                    UI::DragFloat3(ShaderParamDef.Name, (float*)ParamPtr, 0, INFINITY, 10);
                  }
                  break;
                  case SHADER_PARAM_TYPE_Vec4:
                  {
                    vec4* ParamPtr = (vec4*)ParamLocation;
                    UI::DragFloat4(ShaderParamDef.Name, (float*)ParamPtr, 0, INFINITY, 10);
                  }
                  break;
                  case SHADER_PARAM_TYPE_Map:
                  {
                    int32_t ActivePathIndex        = 0;
                    rid*    CurrentParamTextureRID = (rid*)ParamLocation;
                    if(0 < CurrentParamTextureRID->Value)
                    {
                      ActivePathIndex =
                        GameState->Resources.GetTexturePathIndex(*CurrentParamTextureRID);
                    }
                    if(0 < GameState->Resources.TexturePathCount)
                    {
                      UI::Combo(ShaderParamDef.Name, &ActivePathIndex,
                                GameState->Resources.TexturePaths,
                                GameState->Resources.TexturePathCount, PathArrayToString);
                      rid NewRID;
                      if(GameState->Resources.GetTexturePathRID(&NewRID,
                                                                GameState->Resources
                                                                  .TexturePaths[ActivePathIndex]
                                                                  .Name))
                      {
                        *CurrentParamTextureRID = NewRID;
                      }
                      else
                      {
                        *CurrentParamTextureRID = GameState->Resources.RegisterTexture(
                          GameState->Resources.TexturePaths[ActivePathIndex].Name);
                      }
                      assert(0 < CurrentParamTextureRID->Value);
                    }
                  }
                  break;
                }
              }
            }
          }
        }
        if(GameState->Resources.MaterialPathCount > 0 && GameState->CurrentMaterialID.Value > 0)
        {
          int CurrentMaterialPathIndex =
            GameState->Resources.GetMaterialPathIndex(GameState->CurrentMaterialID);
          if(CurrentMaterialPathIndex != -1)
          {
            char* CurrentMaterialPath =
              GameState->Resources.MaterialPaths[CurrentMaterialPathIndex].Name;
            if(UI::Button("Save"))
            {
              ExportMaterial(&GameState->Resources, CurrentMaterial, CurrentMaterialPath);
            }
          }
        }
        UI::SameLine();
        if(UI::Button("Duplicate Current"))
        {
          GameState->CurrentMaterialID =
            GameState->Resources.CreateMaterial(*CurrentMaterial, NULL);
          printf("Created Material with rid: %d\n", GameState->CurrentMaterialID.Value);
        }
          // Bad Idea, because texture RIDs cannot be 0
#if 0
        UI::SameLine();
        if(UI::Button("Clear Material Fields"))
        {
          uint32_t ShaderType                = CurrentMaterial->Common.ShaderType;
          *CurrentMaterial                   = {};
          CurrentMaterial->Common.ShaderType = ShaderType;
        }
#endif
        UI::SameLine();
        entity* SelectedEntity = {};
        if(UI::Button("Create New"))
        {
          GameState->CurrentMaterialID =
            GameState->Resources.CreateMaterial(NewPhongMaterial(), NULL);
          printf("Created Material with rid: %d\n", GameState->CurrentMaterialID.Value);
        }
        UI::SameLine();
        if(GetSelectedEntity(GameState, &SelectedEntity))
        {
          if(UI::Button("Apply To Selected"))
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
          UI::SameLine();
          if(GameState->SelectionMode == SELECT_Mesh)
          {
            if(UI::Button("Edit Selected"))
            {
              GameState->CurrentMaterialID =
                SelectedEntity->MaterialIDs[GameState->SelectedMeshIndex];
            }
          }
        }
        UI::NewLine();
      }
    }
  }
}

void
EntityGUI(game_state* GameState, bool& s_ShowEntityTools)
{
  if(UI::CollapsingHeader("Entity Tools", &s_ShowEntityTools))
  {
    static int32_t ActivePathIndex = 0;
    UI::Combo("Model", &ActivePathIndex, GameState->Resources.ModelPaths,
              GameState->Resources.ModelPathCount, PathArrayToString);
    {
      rid NewRID = { 0 };
      if(!GameState->Resources
            .GetModelPathRID(&NewRID, GameState->Resources.ModelPaths[ActivePathIndex].Name))
      {
        NewRID =
          GameState->Resources.RegisterModel(GameState->Resources.ModelPaths[ActivePathIndex].Name);
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
      UI::SameLine();
      if(UI::Button("Delete Entity"))
      {
        DeleteEntity(GameState, GameState->SelectedEntityIndex);
        GameState->SelectedEntityIndex = -1;
      }
      UI::NewLine();

      Anim::transform* Transform = &SelectedEntity->Transform;
      UI::DragFloat3("Translation", (float*)&Transform->Translation, -INFINITY, INFINITY, 10);
      // UI::DragFloat3("Rotation", (float*)&Transform->Rotation, -INFINITY, INFINITY, 720.0f);
      UI::DragFloat3("Scale", (float*)&Transform->Scale, -INFINITY, INFINITY, 10.0f);

      // Rigid Body
      {
        rigid_body* RB = &SelectedEntity->RigidBody;
        // UI::DragFloat3("X", &RB->X.X, -INFINITY, INFINITY, 10);

        if(FloatsEqualByThreshold(Math::Length(RB->q), 0.0f, 0.00001f))
        {
          RB->q.S = 1;
          RB->q.V = {};
        }
        // UI::DragFloat4("q", &RB->q.S, -INFINITY, INFINITY, 10);
        // Math::Normalize(&RB->q);

        if(UI::Button("Clear v"))
        {
          RB->v = {};
        }
        UI::SameLine();
        UI::DragFloat3("v", &RB->v.X, -INFINITY, INFINITY, 10);
        UI::NewLine();
        if(UI::Button("Clear w"))
        {
          RB->w = {};
        }
        UI::SameLine();
        UI::DragFloat3("w", &RB->w.X, -INFINITY, INFINITY, 10);
        UI::NewLine();

        UI::Checkbox("Regard Gravity", &RB->RegardGravity);

        UI::DragFloat("Mass", &RB->Mass, 0, INFINITY, 10);
        if(0 < RB->Mass)
        {
          RB->MassInv = 1.0f / RB->Mass;
        }
        else
        {
          RB->MassInv = 0;
        }
        char TempBuffer[40];
        snprintf(TempBuffer, sizeof(TempBuffer), "Mass Inv.: %f", (double)RB->MassInv);
        UI::Text(TempBuffer);

        { // Inertia
          vec3 InertiaDiagonal = { RB->InertiaBody._11, RB->InertiaBody._22, RB->InertiaBody._33 };
          UI::DragFloat3("Body Space Inertia diagonal", &InertiaDiagonal.X, 0, INFINITY, 10);
          RB->InertiaBody     = {};
          RB->InertiaBody._11 = InertiaDiagonal.X;
          RB->InertiaBody._22 = InertiaDiagonal.Y;
          RB->InertiaBody._33 = InertiaDiagonal.Z;

          RB->InertiaBodyInv = {};
          if(InertiaDiagonal != vec3{})
          {
            RB->InertiaBodyInv._11 = 1.0f / InertiaDiagonal.X;
            RB->InertiaBodyInv._22 = 1.0f / InertiaDiagonal.Y;
            RB->InertiaBodyInv._33 = 1.0f / InertiaDiagonal.Z;
          }
        }
      }

      Render::model* SelectedModel = GameState->Resources.GetModel(SelectedEntity->ModelID);
      if(SelectedModel->Skeleton)
      {
        if(!SelectedEntity->AnimController && UI::Button("Add Anim. Controller"))
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
                GameState->SelectedEntityIndex != GameState->AnimEditor.EntityIndex &&
                UI::Button("Delete Anim. Controller"))

        {
          // TODO(Lukas): REMOVE MEMORY LEAK!!!!!! The AnimController and its arrays are still on the
          // persistent stack
          RemoveAnimationReferences(&GameState->Resources, SelectedEntity->AnimController);
          SelectedEntity->AnimController = 0;
        }
        else if(SelectedEntity->AnimController)
        {
          if(UI::Button("Animate Selected Entity"))
          {
            GameState->SelectionMode = SELECT_Bone;
            AttachEntityToAnimEditor(GameState, &GameState->AnimEditor,
                                     GameState->SelectedEntityIndex);
            // s_ShowAnimationEditor = true;
          }
          if(UI::Button("Play Animation"))
          {
            if(GameState->CurrentAnimationID.Value > 0)
            {
              if(GameState->Resources.GetAnimation(GameState->CurrentAnimationID)->ChannelCount ==
                 SelectedModel->Skeleton->BoneCount)
              {
                if(SelectedEntity->AnimController->AnimStateCount == 0)
                {
                  SelectedEntity->AnimController->AnimationIDs[0] = GameState->CurrentAnimationID;
                  SelectedEntity->AnimController->AnimStateCount  = 1;

                  GameState->Resources.Animations.AddReference(GameState->CurrentAnimationID);
                }
                else
                {
                  if(SelectedEntity->AnimController->BlendFunc != NULL)
                  {
                    SelectedEntity->AnimController->BlendFunc = NULL;
                    GameState->Resources.Animations.RemoveReference(
                      SelectedEntity->AnimController->AnimationIDs[0]);
                  }
                  SelectedEntity->AnimController->AnimationIDs[0] = GameState->CurrentAnimationID;
                  GameState->Resources.Animations.AddReference(GameState->CurrentAnimationID);
                }
                Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 0);
              }
              else if(SelectedEntity->AnimController->AnimStateCount != 0)
              {
                StopAnimation(SelectedEntity->AnimController, 0);
              }
            }
            GameState->PlayerEntityIndex = -1;
          }

          {
            static int32_t ActivePathIndex = 0;
            UI::Combo("Animation", &ActivePathIndex, GameState->Resources.AnimationPaths,
                      GameState->Resources.AnimationPathCount, PathArrayToString);
            rid NewRID = { 0 };
            if(GameState->Resources.AnimationPathCount > 0 &&
               !GameState->Resources
                  .GetAnimationPathRID(&NewRID,
                                       GameState->Resources.AnimationPaths[ActivePathIndex].Name))
            {
              NewRID = GameState->Resources.RegisterAnimation(
                GameState->Resources.AnimationPaths[ActivePathIndex].Name);
            }
            GameState->CurrentAnimationID = NewRID;
          }

          if(UI::Button("Play as entity"))
          {
            Gameplay::ResetPlayer(SelectedEntity);
            GameState->PlayerEntityIndex = GameState->SelectedEntityIndex;
          }
          if(GameState->PlayerEntityIndex == GameState->SelectedEntityIndex)
          {
            if(UI::Button("Stop playing as entity"))
            {
              Gameplay::ResetPlayer(SelectedEntity);
              GameState->PlayerEntityIndex = -1;
            }
          }
        }
      }
    }
  }
}

void
AnimationGUI(game_state* GameState, bool& s_ShowAnimationEditor, bool& s_ShowEntityTools)
{
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

    if(UI::CollapsingHeader("Animation Editor", &s_ShowAnimationEditor))
    {
      if(UI::Button("Stop Editing"))
      {
        DettachEntityFromAnimEditor(GameState, &GameState->AnimEditor);
        GameState->SelectionMode = SELECT_Entity;
        s_ShowEntityTools        = true;
        s_ShowAnimationEditor    = false;
      }

      if(GameState->AnimEditor.Skeleton)
      {
        if(0 < AttachedEntity->AnimController->AnimStateCount)
        {
          Anim::animation* Animation = AttachedEntity->AnimController->Animations[0];
          if(UI::Button("Edit Attached Animation"))
          {
            int32_t AnimationPathIndex = GameState->Resources.GetAnimationPathIndex(
              AttachedEntity->AnimController->AnimationIDs[0]);
            EditAnimation::EditAnimation(&GameState->AnimEditor, Animation,
                                         GameState->Resources.AnimationPaths[AnimationPathIndex]
                                           .Name);
          }
        }
        if(UI::Button("Insert keyframe"))
        {
          EditAnimation::InsertBlendedKeyframeAtTime(&GameState->AnimEditor,
                                                     GameState->AnimEditor.PlayHeadTime);
        }
        if(GameState->AnimEditor.KeyframeCount > 0)
        {
          if(UI::Button("Delete keyframe"))
          {
            EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
          }
          if(UI::Button("Export Animation"))
          {
            time_t     CurrentTime;
            struct tm* TimeInfo;
            char       AnimGroupName[30];
            time(&CurrentTime);
            TimeInfo = localtime(&CurrentTime);
            strftime(AnimGroupName, sizeof(AnimGroupName), "data/animations/%H_%M_%S.anim",
                     TimeInfo);
            Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor,
                                        AnimGroupName);
          }
          if(GameState->AnimEditor.AnimationPath[0] != '\0')
          {
            if(UI::Button("Override Animation"))
            {
              Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor,
                                          GameState->AnimEditor.AnimationPath);
            }
          }
        }

        UI::DragFloat("Playhead Time", &GameState->AnimEditor.PlayHeadTime, -100, 100, 2.0f);
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, 0);
        if(GameState->AnimEditor.KeyframeCount > 0)
        {
          {
            int32_t ActiveBoneIndex = GameState->AnimEditor.CurrentBone;
            UI::Combo("Bone", &ActiveBoneIndex, GameState->AnimEditor.Skeleton->Bones,
                      GameState->AnimEditor.Skeleton->BoneCount, BoneArrayToString);
            EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, ActiveBoneIndex);
          }

          Anim::transform* Transform =
            &GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
               .Transforms[GameState->AnimEditor.CurrentBone];
          mat4 Mat4Transform = TransformToGizmoMat4(Transform);
          UI::DragFloat3("Translation", &Transform->Translation.X, -INFINITY, INFINITY, 10.0f);
          // UI::DragFloat3("Rotation", &Transform->Rotation.X, -INFINITY, INFINITY, 720.0f);
          UI::DragFloat3("Scale", &Transform->Scale.X, -INFINITY, INFINITY, 10.0f);
        }
      }
    }
  }
}

#pragma pack(2)
struct test_struct
{
  float    f;
  float    f2;
  double   d;
  uint16_t u16;
};

// TODO(Lukas) Add bit mask checkbox to the UI API
void
MiscGUI(game_state* GameState, bool& s_ShowLightSettings, bool& s_ShowDisplaySet,
        bool& s_ShowCameraSettings, bool& s_ShowSceneSettings, bool& s_ShowPostProcessingSettings,
        bool& s_ShowECSData)
{
  if(UI::CollapsingHeader("ECS data", &s_ShowECSData))
  {
    char TempBuffer[50];

    snprintf(TempBuffer, sizeof(TempBuffer), "sizeof(entity): %ld", sizeof(entity));
    UI::Text(TempBuffer);

    snprintf(TempBuffer, sizeof(TempBuffer), "sizeof(animation_controller): %ld",
             sizeof(Anim::animation_controller));
    UI::Text(TempBuffer);

    snprintf(TempBuffer, sizeof(TempBuffer), "sizeof(test_struct)): %ld", sizeof(test_struct));
    UI::Text(TempBuffer);
    snprintf(TempBuffer, sizeof(TempBuffer), "alignof(test_struct)): %ld", alignof(test_struct));
    UI::Text(TempBuffer);

    snprintf(TempBuffer, sizeof(TempBuffer), "sizeof(material): %ld", sizeof(material));
    UI::Text(TempBuffer);

    snprintf(TempBuffer, sizeof(TempBuffer), "sizeof(Render::mesh): %ld", sizeof(Render::mesh));
    UI::Text(TempBuffer);

    snprintf(TempBuffer, sizeof(TempBuffer), "sizeof(Anim::transform): %ld",
             sizeof(Anim::transform));
    UI::Text(TempBuffer);

    snprintf(TempBuffer, sizeof(TempBuffer), "sizeof(rigid_body): %ld", sizeof(rigid_body));
    UI::Text(TempBuffer);
  }
  if(UI::CollapsingHeader("Camera", &s_ShowCameraSettings))
  {
    UI::SliderFloat("FieldOfView", &GameState->Camera.FieldOfView, 0, 180);
    UI::SliderFloat("Near CLip Plane", &GameState->Camera.NearClipPlane, 0.01f, 500);
    UI::SliderFloat("Far  Clip Plane", &GameState->Camera.FarClipPlane,
                    GameState->Camera.NearClipPlane, 500);
    UI::SliderFloat("Speed", &GameState->Camera.Speed, 0, 100);
  }
  if(UI::CollapsingHeader("Light Settings", &s_ShowLightSettings))
  {
    UI::DragFloat3("Diffuse", &GameState->R.LightDiffuseColor.X, 0, 10, 5);
    UI::DragFloat3("Ambient", &GameState->R.LightAmbientColor.X, 0, 10, 5);
    UI::DragFloat3("Position", &GameState->R.LightPosition.X, -INFINITY, INFINITY, 5);
    UI::Checkbox("Show gizmo", &GameState->R.ShowLightPosition);

    UI::DragFloat3("Diffuse", &GameState->R.Sun.DiffuseColor.X, 0, 1, 5);
    UI::DragFloat3("Ambient", &GameState->R.Sun.AmbientColor.X, 0, 1, 5);

    UI::SliderFloat("Sun Z Angle", &GameState->R.Sun.RotationZ, 0.0f, 90.0f);
    UI::SliderFloat("Sun Y Angle", &GameState->R.Sun.RotationY, -180, 180);

		if(1 < SHADOWMAP_CASCADE_COUNT)
		{
      UI::SliderInt("Current Cascade Index", &GameState->R.Sun.CurrentCascadeIndex, 0,
                    SHADOWMAP_CASCADE_COUNT - 1);
    }
    UI::Checkbox("Display sun-perspective depth map", &GameState->R.DrawShadowMap);
    UI::Checkbox("Real-time shadows", &GameState->R.RealTimeDirectionalShadows);
    if(UI::Button("Recompute Directional Shadows"))
    {
      GameState->R.RecomputeDirectionalShadows = true;
    }

    if(UI::Button("Clear Directional Shadows"))
    {
      GameState->R.ClearDirectionalShadows = true;
    }
  }

  if(UI::CollapsingHeader("Post-processing", &s_ShowPostProcessingSettings))
  {
    bool HDRTonemap   = GameState->R.PPEffects & POST_HDRTonemap;
    bool Bloom        = GameState->R.PPEffects & POST_Bloom;
    bool FXAA         = GameState->R.PPEffects & POST_FXAA;
    bool Blur         = GameState->R.PPEffects & POST_Blur;
    bool DepthOfField = GameState->R.PPEffects & POST_DepthOfField;
    bool Grayscale    = GameState->R.PPEffects & POST_Grayscale;
    bool NightVision  = GameState->R.PPEffects & POST_NightVision;
    bool MotionBlur   = GameState->R.PPEffects & POST_MotionBlur;
    bool EdgeOutline  = GameState->R.PPEffects & POST_EdgeOutline;
    bool SimpleFog    = GameState->R.PPEffects & POST_SimpleFog;
    bool Noise        = GameState->R.PPEffects & POST_Noise;
    bool Test         = GameState->R.PPEffects & POST_Test;

    UI::Checkbox("HDRTonemap", &HDRTonemap);
    UI::Checkbox("Bloom", &Bloom);
    UI::Checkbox("FXAA", &FXAA);
    UI::Checkbox("Blur", &Blur);
    UI::Checkbox("DepthOfField", &DepthOfField);
    UI::Checkbox("MotionBlur", &MotionBlur);
    UI::Checkbox("Grayscale", &Grayscale);
    UI::Checkbox("NightVision", &NightVision);
    UI::Checkbox("EdgeOutline", &EdgeOutline);
    UI::Checkbox("DepthBuffer", &GameState->R.DrawDepthBuffer);
    UI::Checkbox("SSAO", &GameState->R.RenderSSAO);
    UI::Checkbox("SimpleFog", &SimpleFog);
    UI::Checkbox("VolumetricScattering", &GameState->R.RenderVolumetricScattering);
    UI::Checkbox("Noise", &Noise);
    UI::Checkbox("Test", &Test);

    if(HDRTonemap)
    {
      GameState->R.PPEffects |= POST_HDRTonemap;
      UI::SliderFloat("HDR Exposure", &GameState->R.ExposureHDR, 0.01f, 8.0f);
    }
    else
    {
      GameState->R.PPEffects &= ~POST_HDRTonemap;
    }

    if(Bloom)
    {
      GameState->R.PPEffects |= POST_Bloom;
      UI::SliderFloat("Bloom Threshold", &GameState->R.BloomLuminanceThreshold, 0.01f, 5.0f);
      UI::SliderInt("Bloom Blur Iterations", &GameState->R.BloomBlurIterationCount, 0, 5);
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Bloom;
    }

    if(FXAA)
    {
      GameState->R.PPEffects |= POST_FXAA;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_FXAA;
    }

    if(Blur)
    {
      GameState->R.PPEffects |= POST_Blur;
      UI::SliderFloat("StdDev", &GameState->R.PostBlurStdDev, 0.01f, 10.0f);
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Blur;
    }

    if(DepthOfField)
    {
      GameState->R.PPEffects |= POST_DepthOfField;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_DepthOfField;
    }

    if(MotionBlur)
    {
      GameState->R.PPEffects |= POST_MotionBlur;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_MotionBlur;
    }

    if(Grayscale)
    {
      GameState->R.PPEffects |= POST_Grayscale;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Grayscale;
    }

    if(NightVision)
    {
      GameState->R.PPEffects |= POST_NightVision;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_NightVision;
    }

    if(EdgeOutline)
    {
      GameState->R.PPEffects |= POST_EdgeOutline;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_EdgeOutline;
    }

    if(GameState->R.RenderSSAO)
    {
      UI::SliderFloat("SSAO Sample Radius", &GameState->R.SSAOSamplingRadius, 0.02f, 0.2f);
#if 0
    	UI::Image("Material preview", GameState->R.SSAOTexID, { 700, (int)(700.0 * (3.0f / 5.0f)) });
#endif
    }

    // UI::Image("ScenePreview", GameState->R.LightScatterTextures[0], { 500, 300 });

    if(SimpleFog)
    {
      GameState->R.PPEffects |= POST_SimpleFog;
      UI::SliderFloat("FogDensity", &GameState->R.FogDensity, 0.01f, 0.5f);
      UI::SliderFloat("FogGradient", &GameState->R.FogGradient, 1.0f, 10.0f);
      UI::SliderFloat("FogColor", &GameState->R.FogColor, 0.0f, 1.0f);
    }
    else
    {
      GameState->R.PPEffects &= ~POST_SimpleFog;
    }

    if(Noise)
    {
      GameState->R.PPEffects |= POST_Noise;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Noise;
    }

    if(Test)
    {
      GameState->R.PPEffects |= POST_Test;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Test;
    }
  }

  if(UI::CollapsingHeader("Render Switches", &s_ShowDisplaySet))
  {
    UI::Checkbox("Cubemap", &GameState->DrawCubemap);
    UI::Checkbox("Draw Gizmos", &GameState->DrawGizmos);
    UI::Checkbox("Draw Debug Lines", &GameState->DrawDebugLines);
    UI::Checkbox("Draw Debug Spheres", &GameState->DrawDebugSpheres);
    UI::Checkbox("Draw Actor Meshes", &GameState->DrawActorMeshes);
    UI::Checkbox("Draw Shadowmap Cascade Volumes", &GameState->DrawShadowCascadeVolumes);
    // UI::Checkbox("Timeline", &GameState->DrawTimeline);
  }

  if(UI::CollapsingHeader("Scene", &s_ShowSceneSettings))
  {
    static int32_t SelectedSceneIndex = 0;
    UI::Combo("Import Path", &SelectedSceneIndex, GameState->Resources.ScenePaths,
              GameState->Resources.ScenePathCount, PathArrayToString);
    if(0 < GameState->Resources.ScenePathCount)
    {
      if(UI::Button("Import"))
      {
        ImportScene(GameState, GameState->Resources.ScenePaths[SelectedSceneIndex].Name);
      }
    }
    UI::SameLine();
    if(UI::Button("Export As New"))
    {
      struct tm* TimeInfo;
      time_t     CurrentTime;
      char       PathName[60];
      time(&CurrentTime);
      TimeInfo = localtime(&CurrentTime);
      strftime(PathName, sizeof(PathName), "data/scenes/%H_%M_%S.scene", TimeInfo);
      ExportScene(GameState, PathName);
    }
    UI::SameLine();
    if(0 < GameState->Resources.ScenePathCount)
    {
      if(UI::Button("Export"))
      {
        ExportScene(GameState, GameState->Resources.ScenePaths[SelectedSceneIndex].Name);
      }
    }
    UI::NewLine();
  }

  {
    char TempBuffer[32];
    sprintf(TempBuffer, "ActiveID: %u", UI::GetActiveID());
    UI::Text(TempBuffer);
    sprintf(TempBuffer, "HotID: %u", UI::GetHotID());
    UI::Text(TempBuffer);
  }
}
