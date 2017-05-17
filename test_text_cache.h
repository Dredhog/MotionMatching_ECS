#pragma once

#include "text.h"

struct cache_test
{
  void (*TestFunction)(Text::font*);
  int ResultCount;
  int ExpectedResults[TEXTURE_CACHE_LINE_COUNT];
};

namespace Test
{
  void RunCacheTests(cache_test* Tests, Text::font* Font, int Count);
  void SetUpAndRunCacheTests(Text::font* Font);
}
