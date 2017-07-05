#include "ui.h"

namespace UI
{
  void
  TestGui(game_state* GameState, const game_input* Input)
  {
    UI::BeginFrame(GameState, Input);

    static int         s_CurrentItem = -1;
    static const char* s_Items[]     = { "Cat", "Rat", "Hat", "Pat", "meet", "with", "dad" };
    UI::BeginWindow("window A", { 800, 300 }, { 450, 400 });
    {
      static bool s_HeaderExpanded = true;
      if(UI::CollapsingHeader("Noise", &s_HeaderExpanded))
      {
        static bool s_Checkbox0 = false;
        static bool s_Checkbox1 = false;

        {
          gui_style& Style     = *UI::GetStyle();
          int32_t    Thickness = (int32_t)Style.Vars[UI::VAR_BorderThickness];
          UI::SliderInt("Border Thickness ", &Thickness, 0, 10);
          Style.Vars[UI::VAR_BorderThickness] = Thickness;

          // UI::DragFloat("Window Opacity", &Style.Colors[UI::COLOR_WindowBackground].X, 0, 1, 5);
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

    UI::BeginWindow("Window B", { 200, 300 }, { 350, 200 });
    {
      UI::Text("This is some text written with a special widget");
      UI::Combo("Combo test", &s_CurrentItem, s_Items, ARRAY_SIZE(s_Items), 5);
    }

    UI::EndWindow();

    UI::EndFrame();
  }
}
