#include "ui.h"

namespace UI
{
  void
  TestGui(game_state* GameState, const game_input* Input)
  {
    UI::BeginFrame(GameState, Input);

    UI::BeginWindow("window A", { 800, 400 }, { 400, 500 });
    {
      UI::ReleaseButton("realease me!", { 150, 20 });
      UI::ReleaseButton("Mee too!", { 300, 30 });
      UI::ReleaseButton("And me!", { 200, 50 });
      UI::EndWindow();
    }

    UI::BeginWindow("window B", { 1500, 200 }, { 350, 200 });
    {
      UI::ReleaseButton("Mee too!", { 300, 30 });
      UI::ReleaseButton("realease me!", { 150, 20 });
      UI::ReleaseButton("And me!", { 200, 50 });
      UI::EndWindow();
    }

    UI::BeginWindow("window C", { 200, 100 }, { 450, 400 });
    {
      static bool s_HeaderExpanded  = false;
      static bool s_HeaderExpanded1 = false;

      UI::ReleaseButton("Mee too!", { 300, 30 });
      UI::ReleaseButton("realease me!", { 150, 20 });
      UI::ReleaseButton("Mee too!", { 300, 30 });
      UI::ReleaseButton("realease me!", { 150, 20 });
      UI::ReleaseButton("And me!", { 200, 50 });
      if(UI::CollapsingHeader("I'm a header", &s_HeaderExpanded, { 350, 30 }))
      {
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("And me!", { 200, 50 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
      }
      UI::ReleaseButton("And me!", { 200, 50 });
      if(UI::CollapsingHeader("I'm a header!", &s_HeaderExpanded1, { 350, 30 }))
      {
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("Mee too!", { 300, 30 });
        UI::ReleaseButton("realease me!", { 150, 20 });
        UI::ReleaseButton("And me!", { 200, 50 });
        UI::ReleaseButton("realease me!", { 150, 20 });
      }
      UI::EndWindow();
    }

    UI::EndFrame();
  }
}
