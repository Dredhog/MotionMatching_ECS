#include "ui.h"
#include "scene.h"
#include "shader_def.h"
#include "profile.h"
#include <cstdlib>
#include "blend_stack.h"
#include "mm_profile_editor_gui.h"
#include "mm_timeline_editor.h"
#include "testing_system.h"
#include "inttypes.h"

void MaterialGUI(game_state* GameState, bool& ShowMaterialEditor);
void EntityGUI(game_state* GameState, bool& ShowEntityTools);
void AnimationGUI(game_state* GameState, bool& ShowAnimationEditor, bool& ShowEntityTools);
void TrajectoryGUI(game_state* GameState, bool& ShowTrajectoryEditor);
void TestGUI(game_state* GameState);
void RenderingGUI(game_state* GameState, bool& ShowRenderingSettings, bool& ShowLightSettings,
                  bool& ShowDisplaySet, bool& ShowCameraSettings, bool& ShowSceneSettings,
                  bool& ShowPostProcessingSettings);
void SceneGUI(game_state* GameState, bool& ShowSceneGUI);

const char*
PathArrayToString(const void* Data, int Index)
{
  path* Paths = (path*)Data;
  return Paths[Index].Name;
}

const char*
BoneArrayToString(const void* Data, int Index)
{
  Anim::bone* Bones = (Anim::bone*)Data;
  return Bones[Index].Name;
}

char* g_SplineIndexNames[TRAJECTORY_CAPACITY] = { "0",  "1",  "2",  "3",  "4",  "5",  "6",
                                                  "7",  "8",  "9",  "10", "11", "12", "13",
                                                  "14", "15", "16", "17", "18", "19" };

const char*
SplineArrayToString(const void* Data, int Index)
{
  return g_SplineIndexNames[Index];
}

int
AllocationInfoComparison(const void* A, const void* B)
{
  Memory::allocation_info* AllocInfoA = (Memory::allocation_info*)A;
  Memory::allocation_info* AllocInfoB = (Memory::allocation_info*)B;
  return AllocInfoA->Base > AllocInfoB->Base;
}

void
TestGui(game_state* GameState, const game_input* Input)
{
  BEGIN_TIMED_BLOCK(GUI);

  static bool s_ShowDemoWindow                   = false;
  static bool s_ShowPhysicsWindow                = false;
  static bool s_ShowProfilerWindow               = false;
  static bool s_ShowMotionMatchingTimelineWindow = false;
  static bool s_ShowMotionMatchingWindow         = false;
  static bool s_ShowMMDebugSettingsWindow        = false;

  UI::BeginWindow("Editor Window", { 1200, 20 }, { 650, 550 });
  {
    {
      // char TempBuffer[32];
      // sprintf(TempBuffer, "dt: %.4f", Input->dt);
      // UI::Text(TempBuffer);
    }
    UI::Combo("Selection mode", (int32_t*)&GameState->SelectionMode, g_SelectionEnumStrings,
              SELECT_EnumCount, UI::StringArrayToString);

    static bool s_ShowMaterialEditor         = false;
    static bool s_ShowEntityTools            = true;
    static bool s_ShowAnimationEditor        = false;
    static bool s_ShowLightSettings          = false;
    static bool s_ShowDisplaySet             = false;
    static bool s_ShowCameraSettings         = false;
    static bool s_ShowSceneSettings          = false;
    static bool s_ShowPostProcessingSettings = false;
    static bool s_ShowTrajectoryTools        = false;
    static bool s_ShowRenderingSettings      = false;

    UI::Checkbox("Use Hot Reloading", &GameState->UseHotReloading);
    UI::SameLine(220);
    UI::Checkbox("Update Path List", &GameState->UpdatePathList);
#if 0
    UI::SameLine(400);
    UI::Checkbox("Profiler Window", &s_ShowProfilerWindow);
    UI::Checkbox("Physics Window", &s_ShowPhysicsWindow);
    UI::Checkbox("GUI Params Window", &s_ShowDemoWindow);
    UI::SameLine(220);
#endif
    UI::Checkbox("Motion Matching Debug", &s_ShowMMDebugSettingsWindow);
    UI::SameLine(220);
    UI::Checkbox("Motion Matching", &s_ShowMotionMatchingWindow);
    UI::SameLine(400);
    UI::Checkbox("Debug Timeline", &s_ShowMotionMatchingTimelineWindow);

    EntityGUI(GameState, s_ShowEntityTools);
    TestGUI(GameState);
    TrajectoryGUI(GameState, s_ShowTrajectoryTools);
    MaterialGUI(GameState, s_ShowMaterialEditor);
    AnimationGUI(GameState, s_ShowAnimationEditor, s_ShowEntityTools);
    RenderingGUI(GameState, s_ShowRenderingSettings, s_ShowLightSettings, s_ShowDisplaySet,
                 s_ShowCameraSettings, s_ShowSceneSettings, s_ShowPostProcessingSettings);
    SceneGUI(GameState, s_ShowSceneSettings);
#if 0
    {
      char TempBuffer[32];
      sprintf(TempBuffer, "Selected Entity Index: %d", GameState->SelectedEntityIndex);
      UI::Text(TempBuffer);
    }
#endif
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
      static bool s_ShowEntityEditor             = false;
      static bool s_ShowChunkMemoryVisualization = false;

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
            (s_CurrentModifiableFrameIndex + PROFILE_MAX_FRAME_COUNT - 1) % PROFILE_MAX_FRAME_COUNT;
        }
        UI::SameLine();
        if(UI::Button("Next Frame"))
        {
          s_CurrentModifiableFrameIndex =
            (s_CurrentModifiableFrameIndex + PROFILE_MAX_FRAME_COUNT + 1) % PROFILE_MAX_FRAME_COUNT;
        }
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

        UI::PushColor(UI::COLOR_WindowBackground, vec4{ 0.4f, 0.4f, 0.5f, 0.3f });
        UI::BeginChildWindow("Profile Timeline Window", { AvailableWidth - ChildPadding * 2, 200 });
        {
          UI::PushVar(UI::VAR_BoxPaddingX, 1);
          UI::PushVar(UI::VAR_BoxPaddingY, 1);
          UI::PushVar(UI::VAR_SpacingX, 0);
          UI::PushVar(UI::VAR_SpacingY, 1);
          {
            const float           MaxProfileWidth = (0.5f * s_TimelineZoom) * UI::GetWindowWidth();
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
                  UI::PushColor(UI::COLOR_ButtonNormal,
                                vec4{ EventColor[0], EventColor[1], EventColor[2], 1 });
                  {
                    float DummyWidth = EventLeft - CurrentHorizontalPosition;
                    UI::Dummy(EventLeft - CurrentHorizontalPosition);
                    UI::SameLine();
                    if(UI::Button(TIMER_NAME_TABLE[CurrentEvent.NameTableIndex], EventWidth))
                    {
                      s_BlockIndexForSummary = CurrentEvent.NameTableIndex;
                    }
                    if(i < GLOBAL_TIMER_FRAME_EVENT_COUNT_TABLE[s_CurrentModifiableFrameIndex] - 1)
                    {
                      UI::SameLine();
                    }
                    CurrentHorizontalPosition += DummyWidth + EventWidth;
                  }
                  UI::PopColor();
                }
              }
              UI::NewLine();
            }
          }
          UI::PopVar();
          UI::PopVar();
          UI::PopVar();
          UI::PopVar();
        }
        UI::EndChildWindow();
        UI::PopColor();
        {
          {
            UI::Text(TIMER_NAME_TABLE[s_BlockIndexForSummary]);
            UI::SameLine();
            {
              char CountBuffer[40];
              sprintf(CountBuffer, ": %" PRIu64,
                      GLOBAL_TIMER_FRAME_SUMMARY_TABLE[s_CurrentModifiableFrameIndex]
                                                      [s_BlockIndexForSummary]
                                                        .CycleCount);
              UI::Text(CountBuffer);
            }
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
            sprintf(CountBuffer, ": %" PRIu64,
                    GLOBAL_TIMER_FRAME_SUMMARY_TABLE[s_CurrentModifiableFrameIndex][i].CycleCount);
            UI::Text(CountBuffer);
          }
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
                      GameState->ECSRuntime->ComponentNames.Count, UI::StringArrayToString, 6);

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
          memcpy(SortedAllocInfos, RawAllocInfos, AllocInfoCount * sizeof(Memory::allocation_info));
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
              UI::PushColor(UI::COLOR_ButtonNormal,
                            vec4{ EventColor[0], EventColor[1], EventColor[2], 1 });
              if(UI::Button(TempBuffer, ChunkWidthInPixels))
              {
                SelectedChunkIndex = ChunkIndex;
              }
              UI::SameLine();
              UI::PopColor();
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

              snprintf(TempBuffer, TempBufferCapacity, "Entity Count    : %d", Header.EntityCount);
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

                UI::Dummy(AlignmentWidth);
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
      UI::Checkbox("Update Physics Subsystem", &GameState->UpdatePhysics);

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
          UI::gui_style& Style     = *UI::GetStyle();
          int32_t        Thickness = (int32_t)Style.Vars[UI::VAR_BorderThickness];
          UI::SliderInt("Border Thickness ", &Thickness, 0, 10);
          Style.Vars[UI::VAR_BorderThickness] = Thickness;

          UI::Text("Hold ctrl when dragging to snap to whole values");
          UI::DragFloat4("Window background", &Style.Colors[UI::COLOR_WindowBackground].X, 0, 1, 5);
          UI::DragFloat4("Header Normal", &Style.Colors[UI::COLOR_HeaderNormal].X, 0, 1, 5);
          UI::DragFloat4("Header Hovered", &Style.Colors[UI::COLOR_HeaderHovered].X, 0, 1, 5);
          UI::DragFloat4("Header Pressed", &Style.Colors[UI::COLOR_HeaderPressed].X, 0, 1, 5);
          UI::SliderFloat("Window Padding X", &Style.Vars[UI::VAR_WindowPaddingX], 0, 20);
          UI::SliderFloat("Window Padding Y", &Style.Vars[UI::VAR_WindowPaddingY], 0, 20);
          UI::SliderFloat("Horizontal Padding", &Style.Vars[UI::VAR_BoxPaddingX], 0, 10);
          UI::SliderFloat("Vertical Padding", &Style.Vars[UI::VAR_BoxPaddingY], 0, 10);
          UI::SliderFloat("Horizontal Spacing", &Style.Vars[UI::VAR_SpacingX], 0, 10);
          UI::SliderFloat("Vertical Spacing", &Style.Vars[UI::VAR_SpacingY], 0, 10);
          UI::SliderFloat("Internal Spacing", &Style.Vars[UI::VAR_InternalSpacing], 0, 10);
          UI::SliderFloat("Indent Spacing", &Style.Vars[UI::VAR_IndentSpacing], 0, 20);
        }
        UI::PushWidth(150);
        UI::Combo("Combo test", &s_CurrentItem, s_Items, ARRAY_SIZE(s_Items), 5);
        UI::PopWidth();
        int StartIndex = 3;
        UI::Combo("Combo test1", &s_CurrentItem, s_Items + StartIndex,
                  ARRAY_SIZE(s_Items) - StartIndex);

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

        sprintf(TempBuff, "ActiveID: %u", UI::GetActiveID());
        UI::Text(TempBuff);
        sprintf(TempBuff, "HotID: %u", UI::GetHotID());
        UI::Text(TempBuff);

        UI::Checkbox("Show Image", &s_Checkbox0);
        if(s_Checkbox0)
        {
          UI::SameLine();
          UI::Checkbox("Put image in frame", &s_Checkbox1);
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

  if(s_ShowMotionMatchingTimelineWindow)
  {
    int32_t         SelectedEntityIndex = GameState->SelectedEntityIndex;
    mm_entity_data* MMEntityData        = &GameState->MMEntityData;

    UI::BeginWindow("MM Animation Visualizer", { 250, 820 }, { 1500, 250 });

    int32_t MMEntityIndex;
    if((MMEntityIndex = GetEntityMMDataIndex(SelectedEntityIndex, MMEntityData)) != -1)
    {
      mm_aos_entity_data MMEntity = GetAOSMMDataAtIndex(MMEntityIndex, MMEntityData);
      if(*MMEntity.MMController)
      {
        MMTimelineWindow(&GameState->MMTimelineState, *MMEntity.BlendStack,
                         *MMEntity.AnimPlayerTime, *MMEntity.AnimGoal, *MMEntity.MMController,
                         GameState->Entities[SelectedEntityIndex].Transform, Input,
                         &GameState->Font);
      }
    }
    UI::EndWindow();
  }
  if(s_ShowMotionMatchingWindow)
  {
    UI::BeginWindow("Motion Matching", { 60, 20 }, { 750, 700 });
    MMControllerEditorGUI(&GameState->MMEditor, GameState->TemporaryMemStack,
                          &GameState->Resources);
    UI::EndWindow();
  }
  if(s_ShowMMDebugSettingsWindow)
  {
    UI::BeginWindow("Matching Debug Settings", { 840, 20 }, { 320, 450 });

    mm_debug_settings& MMDebug = GameState->MMDebug;
    UI::Checkbox("Apply Root Motion", &MMDebug.ApplyRootMotion);
    UI::Text("Debug Display");
    UI::SliderFloat("Trajectory Duration (sec)", &MMDebug.TrajectoryDuration, 0, 10);
    UI::SliderInt("Trajectory Sample Count", &MMDebug.TrajectorySampleCount, 2, 40);
    UI::Checkbox("Show Root Trajectory", &MMDebug.ShowRootTrajectories);
    UI::Checkbox("Show Hip Trajectory", &MMDebug.ShowHipTrajectories);
    UI::Checkbox("Show Smooth Goals", &MMDebug.ShowSmoothGoals);
    UI::Text("Current Goal");
    UI::Checkbox("Show Current Goal", &MMDebug.CurrentGoal.ShowTrajectory);
    UI::Checkbox("Show Current Goal Directions", &MMDebug.CurrentGoal.ShowTrajectoryAngles);
    UI::Checkbox("Show Current Positions", &MMDebug.CurrentGoal.ShowBonePositions);
    UI::Checkbox("Show Current Velocities", &MMDebug.CurrentGoal.ShowBoneVelocities);
    UI::Text("Matched Goal");
    UI::Checkbox("Show Matched Goal", &MMDebug.MatchedGoal.ShowTrajectory);
    UI::Checkbox("Show Matched Goal Directions", &MMDebug.MatchedGoal.ShowTrajectoryAngles);
    UI::Checkbox("Show Matched Positions", &MMDebug.MatchedGoal.ShowBonePositions);
    UI::Checkbox("Show Matched Velocities", &MMDebug.MatchedGoal.ShowBoneVelocities);

		//TODO(Lukas) This is retarded ;(
    {
      bool OverlayGoals = MMDebug.CurrentGoal.Overlay;
      UI::Checkbox("Overlay Goals", &OverlayGoals);
      MMDebug.CurrentGoal.Overlay = OverlayGoals ;
      MMDebug.MatchedGoal.Overlay = OverlayGoals;
    }
    UI::Checkbox("Overlay Longerm Trajectories", &GameState->OverlaySplines);
#if 0
    {
      static vec3 A          = { -1, 0, 0 };
      static vec3 B          = {};
      static vec3 C          = { 1, 0, 0 };
      static vec3 D          = { 2, 0, 0 };
      static int  PointCount = 10;
      UI::Text("Catmull Rom Test");
      UI::SliderInt("Spline Viz Point Count", &PointCount, 1, 30);
      UI::DragFloat3("A", &A.X, -3, 3, 6);
      UI::DragFloat3("B", &B.X, -3, 3, 6);
      UI::DragFloat3("C", &C.X, -3, 3, 6);
      UI::DragFloat3("D", &D.X, -3, 3, 6);

      Debug::PushWireframeSphere(A, 0.1f, { 0, 1, 0, 0.1f });
      Debug::PushLine(A, B, { 1, 0, 0, 1 });
      Debug::PushWireframeSphere(B, 0.1f, { 0, 1, 1, 0.3f });
      Debug::PushWireframeSphere(C, 0.1f, { 0, 0, 0, 0.7f });
      Debug::PushLine(C, D, { 1, 0, 0, 1 });
      Debug::PushWireframeSphere(D, 0.1f, { 1, 1, 0, 1 });

      for(int i = 0; i < PointCount && PointCount > 1; i++)
      {
        float t = float(i) / float(PointCount - 1);

        vec3 CatmullRomPoint = GetCatmullRomPoint(A, B, C, D, t);
        Debug::PushWireframeSphere(CatmullRomPoint, 0.05f, { 1, 0.2f, 0.2f, 1 });
      }
    }
#endif
    UI::EndWindow();
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

  END_TIMED_BLOCK(GUI);
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
          UI::PushWidth(200);
          UI::Combo("Shader Type", (int32_t*)&NewShaderType, g_ShaderTypeEnumStrings,
                    SHADER_EnumCount, UI::StringArrayToString, 6);
          UI::PopWidth();
          if(CurrentMaterial->Common.ShaderType != NewShaderType)
          {
            *CurrentMaterial                   = {};
            CurrentMaterial->Common.ShaderType = NewShaderType;
          }
        }

        UI::Checkbox("Blending", &CurrentMaterial->Common.UseBlending);
        UI::SameLine();
        UI::Checkbox("Skeletel", &CurrentMaterial->Common.IsSkeletal);

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
        if(GetSelectedEntity(GameState, &SelectedEntity))
        {
          UI::SameLine();
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
          if(GameState->SelectionMode == SELECT_Mesh)
          {
            UI::SameLine();
            if(UI::Button("Edit Selected"))
            {
              GameState->CurrentMaterialID =
                SelectedEntity->MaterialIDs[GameState->SelectedMeshIndex];
            }
          }
        }
      }
    }
  }
}

void
EntityGUI(game_state* GameState, bool& s_ShowEntityTools)
{
  if(UI::Button("Create Entity"))
  {
    GameState->IsEntityCreationMode = !GameState->IsEntityCreationMode;
  }

  entity* SelectedEntity = {};
  GetSelectedEntity(GameState, &SelectedEntity);

  if(SelectedEntity)
  {
    UI::SameLine();
    if(UI::Button("Delete Entity"))
    {
      DeleteEntity(GameState, &GameState->Resources, GameState->SelectedEntityIndex);
      GameState->SelectedEntityIndex = -1;
    }
  }

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
  }

  if(SelectedEntity && UI::CollapsingHeader("Entity Parameters", &s_ShowEntityTools))
  {

    if(SelectedEntity)
    {
      static bool s_ShowTransformComponent = false;
      if(UI::TreeNode("Transform Component", &s_ShowTransformComponent))
      {
        transform* Transform = &SelectedEntity->Transform;
        UI::DragFloat3("Translation", (float*)&Transform->T, -INFINITY, INFINITY, 10);
        // UI::DragFloat3("Rotation", (float*)&Transform->Rotation, -INFINITY, INFINITY, 720.0f);
        UI::DragFloat3("Scale", (float*)&Transform->S, -INFINITY, INFINITY, 10.0f);
        UI::TreePop();
      }

      static bool s_ShowPhysicsComponent = false;
      if(UI::TreeNode("Physics Component", &s_ShowPhysicsComponent))
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
        if(UI::Button("Clear w"))
        {
          RB->w = {};
        }
        UI::SameLine();
        UI::DragFloat3("w", &RB->w.X, -INFINITY, INFINITY, 10);

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
        UI::TreePop();
      }

      Render::model* SelectedModel = GameState->Resources.GetModel(SelectedEntity->ModelID);
      if(SelectedModel->Skeleton)
      {
        if(!SelectedEntity->AnimPlayer && UI::Button("Add Animation Player"))
        {
          SelectedEntity->AnimPlayer =
            PushStruct(GameState->PersistentMemStack, Anim::animation_player);
          *SelectedEntity->AnimPlayer = {};

          SelectedEntity->AnimPlayer->Skeleton = SelectedModel->Skeleton;
          SelectedEntity->AnimPlayer->OutputTransforms =
            PushArray(GameState->PersistentMemStack,
                      ANIM_PLAYER_OUTPUT_BLOCK_COUNT * SelectedModel->Skeleton->BoneCount,
                      transform);
          SelectedEntity->AnimPlayer->BoneSpaceMatrices =
            PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
          SelectedEntity->AnimPlayer->ModelSpaceMatrices =
            PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
          SelectedEntity->AnimPlayer->HierarchicalModelSpaceMatrices =
            PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
        }
        else if(SelectedEntity->AnimPlayer)
        {
          static bool s_ShowAnimtionPlayerComponent = true;
          bool        RemovedAnimPlayer         = false;

          UI::TreeNode("Animation Player Component", &s_ShowAnimtionPlayerComponent);

          if(GameState->SelectedEntityIndex != GameState->AnimEditor.EntityIndex)
          {
            UI::PushID("Remove Anim Player");
            UI::SameLine();
            if(UI::Button("Remove"))
            {
              RemovedAnimPlayer = true;
              RemoveAnimationPlayerComponent(GameState, &GameState->Resources,
                                             GameState->SelectedEntityIndex);
            }
            UI::PopID();
          }

          if(s_ShowAnimtionPlayerComponent && !RemovedAnimPlayer)
          {
#if 0
            if(UI::Button("Animate Selected Entity"))
            {
              GameState->SelectionMode = SELECT_Bone;
              AttachEntityToAnimEditor(GameState, &GameState->AnimEditor,
                                       GameState->SelectedEntityIndex);
              // s_ShowAnimationEditor = true;
            }
#endif
            {
              // Outside data used
              mm_entity_data&             MMEntityData        = GameState->MMEntityData;
              int                         SelectedEntityIndex = GameState->SelectedEntityIndex;
              const spline_system&        SplineSystem        = GameState->SplineSystem;
              Resource::resource_manager* Resources           = &GameState->Resources;
              const Anim::skeleton*       Skeleton = SelectedEntity->AnimPlayer->Skeleton;
              entity*                     Entities = GameState->Entities;

              bool RemovedMatchingAnimPlayer = false;
              // Actual UI
              int32_t     MMControllerIndex           = -1;
              static bool s_ShowMMControllerComponent = true;
              if((MMControllerIndex = GetEntityMMDataIndex(SelectedEntityIndex, &MMEntityData)) ==
                 -1)
              {
                if(MMEntityData.Count < MM_CONTROLLER_MAX_COUNT &&
                   UI::Button("Add Matched Animation Controller"))
                {
                  RemoveReferencesAndResetAnimPlayer(Resources, SelectedEntity->AnimPlayer);

                  MMEntityData.Count++;
                  mm_aos_entity_data MMControllerData =
                    GetAOSMMDataAtIndex(MMEntityData.Count - 1, &MMEntityData);

                  SetDefaultMMControllerFileds(&MMControllerData);
                  *MMControllerData.EntityIndex = SelectedEntityIndex;
                }
              }
              else
              {
                UI::TreeNode("Matched Anim. Controller Component", &s_ShowMMControllerComponent);
                UI::SameLine();
                UI::PushID("Remove Matching Anim. Controller");
                RemovedMatchingAnimPlayer = UI::Button("Remove");
                UI::PopID();

                if(s_ShowMMControllerComponent)
                {
                  mm_aos_entity_data MMControllerData =
                    GetAOSMMDataAtIndex(MMControllerIndex, &MMEntityData);

                  {
                    // Pick the mm controller
                    static int32_t ShownPathIndex = -1;
                    int32_t        UsedPathIndex  = -1;
                    if(MMControllerData.MMControllerRID->Value > 0)
                    {
                      UsedPathIndex =
                        Resources->GetMMControllerPathIndex(*MMControllerData.MMControllerRID);
                    }
                    bool ClickedAdd = false;
                    {
                      if(ShownPathIndex != UsedPathIndex)
                      {
                        UI::PushColor(UI::COLOR_ButtonNormal, vec4{ 0.8f, 0.8f, 0.4f, 1 });
                        UI::PushColor(UI::COLOR_ButtonHovered, vec4{ 1, 1, 0.6f, 1 });
                      }
                      UI::PushID(s_ShowMMControllerComponent);
                      ClickedAdd = UI::Button("Add");
                      UI::PopID();
                      if(ShownPathIndex != UsedPathIndex)
                      {
                        UI::PopColor();
                        UI::PopColor();
                      }
                    }
                    UI::SameLine();
                    UI::Combo("Controller", &ShownPathIndex, &Resources->MMControllerPaths,
                              Resources->MMControllerPathCount, PathArrayToString);
                    if(ClickedAdd)
                    {
                      if(ShownPathIndex != -1)
                      {
                        rid NewRID = Resources->ObtainMMControllerPathRID(
                          Resources->MMControllerPaths[ShownPathIndex].Name);
                        mm_controller_data* MMController = Resources->GetMMController(NewRID);
                        if(MMController->Params.FixedParams.Skeleton.BoneCount ==
                           Skeleton->BoneCount)
                        {
                          if(MMControllerData.MMControllerRID->Value > 0 &&
                             MMControllerData.MMControllerRID->Value != NewRID.Value)
                          {
                            Resources->MMControllers.RemoveReference(
                              *MMControllerData.MMControllerRID);
                          }
                          *MMControllerData.MMControllerRID = NewRID;
                          Resources->MMControllers.AddReference(NewRID);
                        }
                      }
                      else if(MMControllerData.MMControllerRID->Value > 0)
                      {
                        Resources->MMControllers.RemoveReference(*MMControllerData.MMControllerRID);
                        *MMControllerData.MMControllerRID = {};
                      }
                    }
                  }

                  char TempBuffer[40];
                  sprintf(TempBuffer, "MM Controller Index: %d", MMControllerIndex);
                  UI::Text(TempBuffer);

									static bool s_ShowInputControlParameters = false;
                  if(UI::TreeNode("Movement Control Options", &s_ShowInputControlParameters))
                  {
                    UI::SliderFloat("Maximum Speed", &MMControllerData.InputController->MaxSpeed,
                                    0.0f, 5.0f);
                    UI::Checkbox("Strafe", &MMControllerData.InputController->UseStrafing);
                    UI::Checkbox("Use Smoothed Goal",
                                 &MMControllerData.InputController->UseSmoothGoal);
                    static bool s_ShowSmoothTrajectoryParams = false;
                    if(MMControllerData.InputController->UseSmoothGoal &&
                       UI::TreeNode("Smooth Goal Params", &s_ShowSmoothTrajectoryParams))
                    {
                      UI::SliderFloat("Position Bias",
                                      &MMControllerData.InputController->PositionBias, 0.0f, 5.0f);
                      UI::SliderFloat("Direction Bias",
                                      &MMControllerData.InputController->DirectionBias, 0.0f, 5.0f);
                      UI::TreePop();
                    }
                    UI::Checkbox("Use Trajectory Control", MMControllerData.FollowSpline);

                    if(*MMControllerData.FollowSpline == true)
                    {
                      /*static bool s_ShowTrajectoryControlParameters = true;
                      if(UI::TreeNode("Trajectory Control Params",
                                      &s_ShowTrajectoryControlParameters))
                      {*/
                      UI::Combo("Trajectory Index", &MMControllerData.SplineState->SplineIndex,
                                (const char**)&g_SplineIndexNames[0], SplineSystem.Splines.Count);
                      /*UI::Checkbox("Loop Back To Start", &MMControllerData.SplineState->Loop);
                      UI::Checkbox("Following Positive Direction",
                                   &MMControllerData.SplineState->MovingInPositive);

                      UI::TreePop();
                    }*/
                    }
                    UI::TreePop();
                  }
                  UI::TreePop();
                }
                if(RemovedMatchingAnimPlayer)
                {
                  RemoveMMControllerDataAtIndex(Entities, MMControllerIndex, Resources,
                                                &MMEntityData);
                }
              }
            }
            UI::TreePop();
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
        if(0 < AttachedEntity->AnimPlayer->AnimStateCount)
        {
          Anim::animation* Animation = AttachedEntity->AnimPlayer->Animations[0];
          if(UI::Button("Edit Attached Animation"))
          {
            int32_t AnimationPathIndex = GameState->Resources.GetAnimationPathIndex(
              AttachedEntity->AnimPlayer->AnimationIDs[0]);
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

          transform* Transform =
            &GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
               .Transforms[GameState->AnimEditor.CurrentBone];
          mat4 Mat4Transform = TransformToGizmoMat4(Transform);
          UI::DragFloat3("Translation", &Transform->T.X, -INFINITY, INFINITY, 10.0f);

          {
            vec3 RotationForEditing = Math::QuatToEuler(Transform->R);
            UI::DragFloat3("Rotation", &RotationForEditing.X, -INFINITY, INFINITY, 720.0f);
            Transform->R = Math::EulerToQuat(RotationForEditing);
          }
          UI::DragFloat3("Scale", &Transform->S.X, -INFINITY, INFINITY, 10.0f);
        }
      }
    }
  }
}

void
TrajectoryGUI(game_state* GameState, bool& s_ShowTrajectoryEditor)
{
  if(UI::CollapsingHeader("Trajectory Editor", &s_ShowTrajectoryEditor))
  {
    if(!GameState->SplineSystem.Splines.Full())
    {
      if(UI::Button("Create Spline"))
      {
        GameState->SplineSystem.Splines.Push({});
        GameState->SplineSystem.SelectedSplineIndex   = GameState->SplineSystem.Splines.Count - 1;
        GameState->SplineSystem.SelectedWaypointIndex = -1;
      }
    }
    else
    {
      UI::Dummy(0, 20);
    }
    // Loop over
    int DeleteSplineIndex = -1;
    UI::PushID("Spline");
    for(int i = 0; i < GameState->SplineSystem.Splines.Count; i++)
    {
      UI::PushID(i);
      UI::Text("Spline");
      UI::SameLine();
      if(UI::Button("Delete"))
      {
        DeleteSplineIndex                             = i;
        GameState->SplineSystem.SelectedSplineIndex   = -1;
        GameState->SplineSystem.SelectedWaypointIndex = -1;
      }
      else
      {
        UI::SameLine();
        bool IsCurrentSplineSelected = i == GameState->SplineSystem.SelectedSplineIndex;
        if(IsCurrentSplineSelected)
        {
          UI::PushColor(UI::COLOR_ButtonNormal, vec4{ 1, 0.2f, 0.2f, 1 });
          UI::PushColor(UI::COLOR_ButtonHovered, vec4{ 1, 0.5f, 0.5f, 1 });
        }

        if(UI::Button("Select"))
        {
          if(GameState->SplineSystem.SelectedSplineIndex != i)
          {
            GameState->SplineSystem.SelectedWaypointIndex = -1;
          }
          GameState->SplineSystem.SelectedSplineIndex = i;
        }
        if(IsCurrentSplineSelected)
        {
          UI::PopColor();
          UI::PopColor();
        }
      }
      UI::PopID();
    }
    UI::PopID();
    if(DeleteSplineIndex != -1)
    {
      GameState->SplineSystem.Splines.Remove(DeleteSplineIndex);
    }
    int CurrentSplineIndex  = GameState->SplineSystem.SelectedSplineIndex;
    int DeleteWaypointIndex = -1;
    UI::PushID("Waypoint");
    for(int i = 0; CurrentSplineIndex != -1 &&
                   i < GameState->SplineSystem.Splines[CurrentSplineIndex].Waypoints.Count;
        i++)
    {
      UI::PushID(i);
      UI::Text("Waypoint");
      UI::SameLine();
      if(UI::Button("Delete"))
      {
        DeleteWaypointIndex                           = i;
        GameState->SplineSystem.SelectedWaypointIndex = -1;
      }
      else
      {
        UI::SameLine();
        bool IsCurrentWaypointsSelected = i == GameState->SplineSystem.SelectedWaypointIndex;
        if(IsCurrentWaypointsSelected)
        {
          UI::PushColor(UI::COLOR_ButtonNormal, vec4{ 0.2f, 0.2f, 1, 1 });
          UI::PushColor(UI::COLOR_ButtonHovered, vec4{ 0.5f, 0.5f, 1, 1 });
        }
        if(UI::Button("Select"))
        {
          GameState->SplineSystem.SelectedWaypointIndex = i;
        }
        if(IsCurrentWaypointsSelected)
        {
          UI::PopColor();
          UI::PopColor();
        }
      }
      UI::PopID();
    }
    UI::PopID();
    if(DeleteWaypointIndex != -1)
    {
      GameState->SplineSystem.Splines[CurrentSplineIndex].Waypoints.Remove(DeleteWaypointIndex);
    }

    bool NotPlacing = GameState->SplineSystem.IsWaypointPlacementMode;
    if(NotPlacing)
		{
			UI::PushColor(UI::COLOR_ButtonNormal, vec4{ 0.8f, 0.8f, 0.4f, 1 });
			UI::PushColor(UI::COLOR_ButtonHovered, vec4{ 0.9f, 0.9f, 0.5f, 1 });
		}
    if(CurrentSplineIndex != -1 &&
       !GameState->SplineSystem.Splines[CurrentSplineIndex].Waypoints.Full() &&
       UI::Button("Place New Waypoint"))
    {
      GameState->SplineSystem.IsWaypointPlacementMode = true;
    }
		if(NotPlacing)
		{
			UI::PopColor();
			UI::PopColor();
		}
  }
}

// TODO(Lukas) Add bit mask checkbox to the UI API
void
RenderingGUI(game_state* GameState, bool& s_ShowRenderingSettings, bool& s_ShowLightSettings,
             bool& s_ShowDisplaySet, bool& s_ShowCameraSettings, bool& s_ShowSceneSettings,
             bool& s_ShowPostProcessingSettings)
{
  if(UI::CollapsingHeader("Rendering Settings", &s_ShowRenderingSettings))
  {
    if(UI::TreeNode("Camera", &s_ShowCameraSettings))
    {
      UI::SliderFloat("FieldOfView", &GameState->Camera.FieldOfView, 0, 180);
      UI::SliderFloat("Near CLip Plane", &GameState->Camera.NearClipPlane, 0.01f, 500);
      UI::SliderFloat("Far  Clip Plane", &GameState->Camera.FarClipPlane,
                      GameState->Camera.NearClipPlane, 500);
      UI::SliderFloat("Speed", &GameState->Camera.Speed, 0, 100);
      UI::TreePop();
    }
    if(UI::TreeNode("Lighting", &s_ShowLightSettings))
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
      UI::TreePop();
    }

    if(UI::TreeNode("Post-processing", &s_ShowPostProcessingSettings))
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
      UI::TreePop();
    }

    if(UI::TreeNode("What To Draw", &s_ShowDisplaySet))
    {
      UI::Checkbox("Cubemap", &GameState->DrawCubemap);
      UI::Checkbox("Draw Gizmos", &GameState->DrawGizmos);
      UI::Checkbox("Draw Debug Lines", &GameState->DrawDebugLines);
      UI::Checkbox("Draw Debug Spheres", &GameState->DrawDebugSpheres);
      UI::Checkbox("Draw Actor Meshes", &GameState->DrawActorMeshes);
      UI::Checkbox("Draw Actor Skeletons", &GameState->DrawActorSkeletons);
      UI::Checkbox("Draw Shadowmap Cascade Volumes", &GameState->DrawShadowCascadeVolumes);
      // UI::Checkbox("Timeline", &GameState->DrawTimeline);
      UI::TreePop();
    }
  }
}

struct test_gui_state
{
  int32_t SelectedEntityIndex; // Always up to date with game state, used to reset UI, when changed
  int32_t SelectedBoneIndex;
  int32_t SelectedAnimationIndex;

  bool Expanded;
  bool ExpandedFootSkate;
  bool ExpandedBonesToMeasure;
  bool ExpandedSplineFollowing;
  bool ExpandedDirectionChanging;
  bool ExpandedActiveTests;

  foot_skate_test FootSkateTest;
  follow_test     FollowTest;
  facing_test     FacingTest;
};

test_gui_state GetDefaultTestGUIState()
{
  test_gui_state GUI   ={};
  {
    GUI.SelectedEntityIndex = -1;
    GUI.SelectedBoneIndex   = -1;
    GUI.FacingTest          = GetDefaultFacingTest();
  }
	return GUI;
}

void
TestGUI(game_state* GameState)
{
  testing_system&       Tests = GameState->TestingSystem;
  static test_gui_state GUI   = GetDefaultTestGUIState();

  // Reseting GUI when selected entity changes
  if(GameState->SelectedEntityIndex != GUI.SelectedEntityIndex)
  {
    GUI.SelectedEntityIndex     = -1;
    GUI.SelectedBoneIndex       = -1;
    GUI.FootSkateTest           = {};
    GUI.FootSkateTest.TopMargin = 0.1f;
    GUI.FollowTest              = {};
    GUI.FacingTest              = GetDefaultFacingTest();
    GUI.SelectedEntityIndex     = GameState->SelectedEntityIndex;
  };

  entity* SelectedEntity;
  GetSelectedEntity(GameState, &SelectedEntity);

  const bool EntityWithPlayerIsSelected = SelectedEntity && SelectedEntity->AnimPlayer;
  if((EntityWithPlayerIsSelected || !Tests.ActiveTests.Empty()) &&
     UI::CollapsingHeader("Test Editor", &GUI.Expanded))
  {
    UI::PushID("Test");
    if(EntityWithPlayerIsSelected)
    {
      int32_t MMEntityIndex =
        GetEntityMMDataIndex(GameState->SelectedEntityIndex, &GameState->MMEntityData);
      if(UI::TreeNode("Test Foot Skate", &GUI.ExpandedFootSkate))
      {
        UI::PushID("Skate");

        if(UI::TreeNode("Bones To Measure", &GUI.ExpandedBonesToMeasure))
        {
          bool ClickedAddBone = UI::Button("AddBone");
          UI::SameLine();
          UI::PushWidth(-UI::GetWindowWidth() * 0.35f);
          UI::Combo("Bone", &GUI.SelectedBoneIndex, SelectedEntity->AnimPlayer->Skeleton->Bones,
                    SelectedEntity->AnimPlayer->Skeleton->BoneCount, BoneArrayToString);
          UI::PopWidth();

          if(ClickedAddBone && GUI.SelectedBoneIndex != -1 &&
             !GUI.FootSkateTest.TestBoneIndices.Full())
          {
            GUI.FootSkateTest.TestBoneIndices.Push(GUI.SelectedBoneIndex);
          }

          int RemoveIndex = -1;
          for(int i = 0; i < GUI.FootSkateTest.TestBoneIndices.Count; i++)
          {
            UI::PushID(i);
            RemoveIndex = UI::Button("Remove") ? i : RemoveIndex;

            UI::SameLine();
            UI::Text(
              SelectedEntity->AnimPlayer->Skeleton->Bones[GUI.FootSkateTest.TestBoneIndices[i]]
                .Name);

            UI::PopID();
          }
          if(RemoveIndex != -1)
          {
            GUI.FootSkateTest.TestBoneIndices.Remove(RemoveIndex);
          }

          UI::TreePop();
        }

        if(MMEntityIndex == -1)
        {
          bool ClickedAddAnimation = UI::Button("Set");
          UI::SameLine();
          UI::Combo("Animation", &GUI.SelectedAnimationIndex, GameState->Resources.AnimationPaths,
                    GameState->Resources.AnimationPathCount, PathArrayToString);
          if(GUI.SelectedAnimationIndex >= 0 && ClickedAddAnimation)
          {
            rid NewRID = GameState->Resources.ObtainAnimationPathRID(
              GameState->Resources.AnimationPaths[GUI.SelectedAnimationIndex].Name);
            if(GameState->Resources.GetAnimation(NewRID)->ChannelCount ==
               SelectedEntity->AnimPlayer->Skeleton->BoneCount)
            {
              GUI.FootSkateTest.AnimationRID = NewRID;
            }
          }
        }

        UI::SliderFloat("Bottom Range", &GUI.FootSkateTest.BottomMargin, 0, 0.2f);
        UI::SliderFloat("Top Range", &GUI.FootSkateTest.TopMargin, 0, 0.2f);

        if(!Tests.ActiveTests.Full() && GUI.FootSkateTest.TestBoneIndices.Count == 2)
        {
          int32_t AnimTestIndex =
            Tests.GetEntityTestIndex(GameState->SelectedEntityIndex, TEST_AnimationFootSkate);
          int32_t ControllerTestIndex =
            Tests.GetEntityTestIndex(GameState->SelectedEntityIndex, TEST_ControllerFootSkate);

          assert(AnimTestIndex == -1 || ControllerTestIndex == -1);
          if(MMEntityIndex == -1)
          {
            UI::PushID("A");
            if(GUI.FootSkateTest.AnimationRID.Value > 0 && UI::Button("Start"))
            {
              Tests.CreateAnimationFootSkateTest(&GameState->Resources, GUI.FootSkateTest,
                                                 GameState->SelectedEntityIndex);
            }
            UI::PopID();
          }
          else
          {
            mm_aos_entity_data MMEntity =
              GetAOSMMDataAtIndex(MMEntityIndex, &GameState->MMEntityData);
            rid ControllerRID = *MMEntity.MMControllerRID;
            UI::PushID("C");
            if(ControllerRID.Value > 0 && UI::Button("Start"))
            {
              Tests.CreateControllerFootSkateTest(&GameState->Resources, GUI.FootSkateTest,
                                                  ControllerRID, GameState->SelectedEntityIndex);
            }
            UI::PopID();
          }
        }

        UI::PopID();

        UI::TreePop();
      }

      if(MMEntityIndex != -1)
      {
        mm_aos_entity_data MMEntity = GetAOSMMDataAtIndex(MMEntityIndex, &GameState->MMEntityData);
        if(MMEntity.MMControllerRID->Value != 0)
        {
          if(UI::TreeNode("Test Direction Changing", &GUI.ExpandedDirectionChanging))
          {
            UI::PushID("Dir");

            UI::SliderFloat("Test Angle Threshold", &GUI.FacingTest.TargetAngleThreshold, 0, 180);
            UI::SliderFloat("Maximal Wait Time", &GUI.FacingTest.MaxWaitTime, 0, 5);
            UI::SliderFloat("Maximal Test Angle", &GUI.FacingTest.MaxTestAngle, 0, 180);
            UI::SliderInt("Total Angle Test Count", &GUI.FacingTest.RemainingCaseCount, 0, 200);
            /*UI::Checkbox("Test Left Side Turns", &GUI.FacingTest.TestLeftSideTurns);
            UI::SameLine();
            UI::Checkbox("Test Right Side Turns", &GUI.FacingTest.TestRightSideTurns);
            */
            int32_t FacingTestIndex =
              Tests.GetEntityTestIndex(GameState->SelectedEntityIndex, TEST_FacingChange);
            if(FacingTestIndex == -1)
            {
              if(UI::Button("Start"))
              {
                Tests.CreateFacingChangeTest(&GameState->Resources, *MMEntity.MMControllerRID,
                                             GUI.FacingTest, GameState->SelectedEntityIndex);
              }
            }

            UI::PopID();

            UI::TreePop();
          }
          if(*MMEntity.FollowSpline &&
             UI::TreeNode("Test Spline Following", &GUI.ExpandedSplineFollowing))
          {
            UI::PushID("Spline");

            int TestIndex =
              Tests.GetEntityTestIndex(GameState->SelectedEntityIndex, TEST_TrajectoryFollowing);
            if(TestIndex == -1)
            {
              if(*MMEntity.FollowSpline && MMEntity.SplineState->SplineIndex != -1)
              {
                if(UI::Button("Start"))
                {
                  Tests.CreateTrajectoryDeviationTest(&GameState->Resources,
                                                      *MMEntity.MMControllerRID, GUI.FollowTest,
                                                      GameState->SelectedEntityIndex);
                }
              }
            }

            UI::PopID();

            UI::TreePop();
          }
        }
      }
    }

    if(!Tests.ActiveTests.Empty() && UI::TreeNode("ActiveTests", &GUI.ExpandedActiveTests))
    {
      for(int i = 0; i < Tests.ActiveTests.Count; i++)
      {
        UI::PushID(i);

        char        TempBuffer[300];
        int         TestType = Tests.ActiveTests[i].Type;
        const char* TestTypeString =
          TestType == TEST_AnimationFootSkate
            ? "Anim Foot Skate"
            : (TestType == TEST_ControllerFootSkate
                 ? "Ctrl Foot Skate"
                 : (TestType == TEST_FacingChange ? "Facing Change" : "Trajectory Follow"));
        snprintf(TempBuffer, ArrayCount(TempBuffer), "#%d: Name: %s", i,
                 Tests.ActiveTests[i].DataTable.Name);
        UI::Text(TempBuffer);
        snprintf(TempBuffer, ArrayCount(TempBuffer), " Entity: #%d, Type: %s",
                 Tests.ActiveTests[i].EntityIndex, TestTypeString);
        UI::Text(TempBuffer);
        if(UI::Button("Stop"))
        {
          Tests.WriteTestToCSV(i);
          Tests.DestroyTest(&GameState->Resources, i);
          i--;
        }
        UI::SameLine();
        UI::PushColor(UI::COLOR_ButtonNormal, { 1, 0.2f, 0.2f, 1 });
        if(UI::Button("Abort"))
        {
          Tests.DestroyTest(&GameState->Resources, i);
          i--;
        }
        UI::PopColor();

        UI::PopID();
      }
      UI::TreePop();
    }
    UI::PopID();
  }
}

void
SceneGUI(game_state* GameState, bool& s_ShowSceneGUI)
{
  if(UI::CollapsingHeader("Scene", &s_ShowSceneGUI))
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
  }
}
