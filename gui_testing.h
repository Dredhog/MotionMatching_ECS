#include "ui.h"

namespace UI
{
  void
  TestGui(game_state* GameState, const game_input* Input)
  {
    UI::BeginFrame(GameState, Input);

    UI::BeginWindow("window A", { 300, 500 }, { 450, 400 });
    {
      static bool        s_HeaderExpanded = true;
      static int         s_CurrentItem    = -1;
      static const char* s_Items[]        = { "Cat", "Rat", "Hat", "Pat", "meet", "with", "dad" };
      static bool        s_Checkbox0      = false;
      static bool        s_Checkbox1      = false;

      if(UI::CollapsingHeader("Noise", &s_HeaderExpanded))
      {
        {
          gui_style& Style = *UI::GetStyle();
          UI::SliderFloat("Horizontal Padding", &Style.Vars[UI::VAR_BoxPaddingX], 0, 10);
          UI::SliderFloat("Vertical Padding", &Style.Vars[UI::VAR_BoxPaddingY], 0, 10);
          UI::SliderFloat("Border Thickness", &Style.Vars[UI::VAR_BorderThickness], 0, 10);
        }
        UI::Combo("Combo test", &s_CurrentItem, s_Items, ARRAY_SIZE(s_Items), 5);

        char TempBuff[20];
        snprintf(TempBuff, sizeof(TempBuff), "Wheel %d", Input->MouseWheelScreen);
        UI::Text(TempBuff);

        UI::Checkbox("Show Image", &s_Checkbox0);
        if(s_Checkbox0)
        {
          UI::Checkbox("Put image in frame", &s_Checkbox1);
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

    UI::BeginWindow("window B", { 800, 300 }, { 600, 500 });
    {
      UI::BeginChildWindow("Child Window A ", { 500, 400 });
      {
        UI::ReleaseButton("realease me!");
        {
          static float s_SliderValue = 0.001f;
          char         TempBuff[20];
          snprintf(TempBuff, sizeof(TempBuff), "Wheel %d", Input->MouseWheelScreen);
          UI::SliderFloat("FPS", &s_SliderValue, 20, 50, false);
          UI::Image(GameState->IDTexture, "material preview", { 700, 400 });
        }
      }
      UI::EndChildWindow();
      UI::Image(GameState->IDTexture, "material preview", { 700, 400 });
    }
    UI::EndWindow();

    UI::EndFrame();
  }
}
