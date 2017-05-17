#pragma once

#if CACHE_TEST
#define TEST_COUNT 6
struct cache_test
{
  void (*TestFunction)(Text::font*);
  int ResultCount;
  int ExpectedResults[TEXTURE_CACHE_LINE_COUNT];
};

extern int32_t g_HitCounts[];
extern int32_t g_CachedTextureCount;

cache_test g_Tests[TEST_COUNT];

void
RunCacheTests(cache_test* Tests, Text::font* Font, int Count)
{
  printf("============================================\n");
  printf("Starting Tests:\n");
  int TestsPassed = 0;
  for(int i = 0; i < Count; i++)
  {
    Text::ResetCache();
    Tests[i].TestFunction(Font);
    bool TestFailed = false;
    for(int j = 0; j < Tests[i].ResultCount; j++)
    {
      int Diff = Tests[i].ExpectedResults[j] - g_HitCounts[j];
      if(Diff != 0)
      {
        TestFailed = true;
      }
    }
    if(TestFailed)
    {
      printf("Expected:\t");
      for(int j = 0; j < Tests[i].ResultCount; j++)
      {
        printf("%d\t", Tests[i].ExpectedResults[j]);
      }
      printf("\nActual:\t\t");
      for(int j = 0; j < g_CachedTextureCount; j++)
      {
        printf("%d\t", g_HitCounts[j]);
      }
      printf("\nDiff:\t\t");
      for(int j = 0; j < g_CachedTextureCount; j++)
      {
        int Diff = Tests[i].ExpectedResults[j] - g_HitCounts[j];
        printf("%d\t", Diff);
      }
    }
    else
    {
      ++TestsPassed;
    }
    if(Tests[i].ResultCount != g_CachedTextureCount)
    {
      printf("Expected test %d sample size is not equal to actual sample size!\n", i + 1);
      printf("============================================\n");
    }
    Text::ResetCache();
  }
  printf("Tests performed: %d/%d\n", TestsPassed, Count);
  printf("============================================\n");
}

void
RunCacheTest0(Text::font* Font)
{
  int   ParamCount = 1;
  vec4  Color[1]   = { { 1.0f, 1.0f, 1.0f, 1.0f } };
  char* Text[]     = { "Test1" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 20; i++)
  {
    float TextWidth  = 0.3f;
    float TextHeight = 0.1f;

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
RunCacheTest1(Text::font* Font)
{
  int  ParamCount = 3;
  vec4 Color[3]   = { { 1.0f, 1.0f, 1.0f, 1.0f },
                    { 1.0f, 0.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 0.0f, 1.0f } };
  char* Text[] = { "Testas", "Sec", "Third" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 40; i++)
  {
    float TextWidth  = 0.5f;
    float TextHeight = 0.2f;

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
RunCacheTest2(Text::font* Font)
{
  int  ParamCount = TEXTURE_CACHE_LINE_COUNT;
  vec4 Color;
  char Text[100];

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 2 * TEXTURE_CACHE_LINE_COUNT; i++)
  {
    float TextWidth  = 0.4f;
    float TextHeight = 0.1f;

    Color = { 1.0f, 1.0f - (float)(i % ParamCount) * 0.001f, (float)(i % ParamCount) * 0.002f,
              1.0f };
    sprintf(Text, "Texture %d", (i % ParamCount));

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)), Text, Color,
                       &TextureWidth, &TextureHeight);
  }
}

void
RunCacheTest3(Text::font* Font)
{
  int   ParamCount = 1;
  vec4  Color[1]   = { { 1.0f, 1.0f, 1.0f, 1.0f } };
  char* Text[]     = { "a" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 5; i++)
  {
    float TextWidth  = 0.3f;
    float TextHeight = 0.1f;

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
RunCacheTest4(Text::font* Font)
{
  int  ParamCount = 3;
  vec4 Color[3]   = { { 0.0f, 0.0f, 0.0f, 1.0f },
                    { 0.0f, 0.0f, 0.0f, 0.0f },
                    { 1.0f, 0.5f, 0.0f, 0.5f } };
  char* Text[] = { "White and visible", "Black and invisible", "Orange-ish and transparent" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 100; i++)
  {
    float    TextWidth  = 0.3f;
    float    TextHeight = 0.1f;
    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
RunCacheTest5(Text::font* Font)
{
  int   ParamCount = 1;
  vec4  Color[1]   = { { 0.75f, 0.75f, 0.75f, 1.0f } };
  char* Text[]     = { "Hmmmmm" };

  int32_t TextureWidth;
  int32_t TextureHeight;

  for(int i = 0; i < 256; i++)
  {
    float TextWidth  = 0.3f;
    float TextHeight = 0.1f;

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)),
                       Text[i % ParamCount], Color[i % ParamCount], &TextureWidth, &TextureHeight);
  }
}

void
SetUpAndCallCacheTests(Text::font* Font)
{
  g_Tests[0] = { RunCacheTest0, 1, { 20 } };
  g_Tests[1] = { RunCacheTest1, 3, { 14, 13, 13 } };
  g_Tests[2] = { RunCacheTest2, TEXTURE_CACHE_LINE_COUNT, {} };
  for(int i = 0; i < g_Tests[2].ResultCount; i++)
  {
    g_Tests[2].ExpectedResults[i] = 2;
  }
  g_Tests[3] = { RunCacheTest3, 1, { 5 } };
  g_Tests[4] = { RunCacheTest4, 3, { 34, 33, 33 } };
  g_Tests[5] = { RunCacheTest5, 1, { 256 } };

  RunCacheTests(g_Tests, Font, TEST_COUNT);
}
#endif
