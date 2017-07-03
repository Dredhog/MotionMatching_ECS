#include "ui.h"

namespace UI
{
  void
  TestGui(game_state* GameState, const game_input* Input)
  {
    UI::BeginFrame(GameState, Input);

    UI::BeginWindow("window A", { 300, 500 }, { 450, 400 });
    {
      static bool        s_HeaderExpanded = false;
      static int         s_CurrentItem    = -1;
      static const char* s_Items[]        = { "Cat", "Rat", "Hat", "Pat", "meet", "with", "dad" };
      static bool        s_Checkbox0      = false;
      static float       s_SliderValue    = 0.001f;

      if(UI::CollapsingHeader("Noise", &s_HeaderExpanded, { 350, 30 }))
      {
        UI::Checkbox("Toggle this", &s_Checkbox0);

        UI::Combo("Combo test", &s_CurrentItem, s_Items, ARRAY_SIZE(s_Items), 5);

        char TempBuff[20];
        snprintf(TempBuff, sizeof(TempBuff), "Wheel %d", Input->MouseWheelScreen);
        UI::ReleaseButton(TempBuff);

        UI::SliderFloat("FPS", &s_SliderValue, 20, 50, false);

        UI::Image(GameState->IDTexture, "material preview", { 200, 110 });
      }
    }
    UI::EndWindow();

    UI::BeginWindow("window C", { 800, 300 }, { 600, 500 });
    {
      UI::BeginChildWindow("Child Window A ", { 500, 400 });
      {
        static bool s_HeaderExpanded  = false;
        static bool s_HeaderExpanded1 = false;
        static bool s_HeaderExpanded2 = false;
        if(UI::CollapsingHeader("Header 1", &s_HeaderExpanded, { 350, 30 }))
        {
          UI::BeginChildWindow("Window 5", { 300, 200 });
          UI::ReleaseButton("realease me!");
          {
            static float s_SliderValue = 0.001f;
            char         TempBuff[20];
            snprintf(TempBuff, sizeof(TempBuff), "Wheel %d", Input->MouseWheelScreen);
            UI::SliderFloat("FPS", &s_SliderValue, 20, 50, false);
            UI::Image(GameState->IDTexture, "material preview", { 700, 400 });
          }
          UI::EndChildWindow();
        }
      }
      UI::EndChildWindow();
      UI::Image(GameState->IDTexture, "material preview", { 700, 400 });
    }
    UI::EndWindow();

    UI::EndFrame();
  }
}
