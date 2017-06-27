#include "ui.h"

namespace UI
{
  void
  TestGui(game_state* GameState, const game_input* Input)
  {
    UI::BeginFrame(GameState, Input);

    UI::BeginWindow("window A", { 300, 500 }, { 450, 400 });
    {
      static bool s_HeaderExpanded  = false;
      static bool s_HeaderExpanded1 = false;
      static bool s_HeaderExpanded2 = false;
      static bool s_HeaderExpanded3 = false;
      static bool s_HeaderExpanded4 = false;
      static bool s_HeaderExpanded5 = false;

      if(UI::CollapsingHeader("Wide Inside", &s_HeaderExpanded, { 350, 30 }))
      {
        UI::ReleaseButton("realease me!", { 500, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("And me!", { 200, 50 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
      }
      UI::ReleaseButton("And me!", { 200, 50 });
      if(UI::CollapsingHeader("Header 2", &s_HeaderExpanded1, { 350, 30 }))
      {
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("And me!", { 200, 50 });
        UI::ReleaseButton("realease me!", { 150, 20 });
      }
      UI::ReleaseButton("Mee too!", { 300, 30 });
      UI::ReleaseButton("realease me!", { 150, 20 });
      UI::ReleaseButton("Mee too!", { 300, 30 });
      UI::ReleaseButton("realease me!", { 150, 20 });
      UI::ReleaseButton("And me!", { 200, 50 });
      if(UI::CollapsingHeader("Header 3", &s_HeaderExpanded2, { 350, 30 }))
      {
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("And me!", { 200, 50 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
      }
      if(UI::CollapsingHeader("Header 4", &s_HeaderExpanded3, { 350, 30 }))
      {
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("And me!", { 200, 50 });
        UI::ReleaseButton("realease me!", { 150, 20 });
      }
      if(UI::CollapsingHeader("Header 5", &s_HeaderExpanded4, { 350, 30 }))
      {
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("And me!", { 200, 50 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
      }
      if(UI::CollapsingHeader("I'm header 6!", &s_HeaderExpanded5, { 350, 30 }))
      {
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("And me!", { 200, 50 });
        UI::ReleaseButton("realease me!", { 150, 20 });
      }
      UI::EndWindow();
    }

    UI::BeginWindow("window B", { 1500, 200 }, { 350, 200 });
    {
      static float s_SliderValue = 0.001f;
      char         TempBuff[20];
      snprintf(TempBuff, sizeof(TempBuff), "Wheel %d", Input->MouseWheelScreen);
      UI::ReleaseButton(TempBuff, { 300, 30 });
      UI::ReleaseButton("realease me!", { 150, 20 });
      UI::ReleaseButton("And me!", { 400, 50 });
      UI::SliderFloat("FPS", &s_SliderValue, 10, 50, false, { 300, 30 }, s_SliderValue);
      UI::EndWindow();
    }
    UI::BeginWindow("window C", { 800, 400 }, { 400, 500 });
    {
      UI::ReleaseButton("realease me!", { 150, 20 });
      UI::ReleaseButton("Mee too!", { 300, 30 });
      UI::ReleaseButton("And me!", { 200, 50 });
      UI::EndWindow();
    }

    UI::EndFrame();
  }
}
