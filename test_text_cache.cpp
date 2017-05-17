#include "test_text_cache.h"

#define ArraySize(Array) sizeof(Array) / sizeof((Array)[0])

extern int32_t g_HitCounts[];
extern int32_t g_RequestsPerFrame[];
extern int32_t g_CachedTextureCount;

static void RunCacheTest0(Text::font* Font);
static void RunCacheTest1(Text::font* Font);
static void RunCacheTest3(Text::font* Font);
static void RunCacheTest4(Text::font* Font);
static void RunCacheTest5(Text::font* Font);
static void RunCacheTest6(Text::font* Font);

cache_test g_Tests[] =
  { { RunCacheTest0, 1, { 20 } },  { RunCacheTest1, 3, { 14, 13, 13 } },
    { RunCacheTest3, 1, { 5 } },   { RunCacheTest4, 3, { 34, 33, 33 } },
    { RunCacheTest5, 1, { 256 } }, { RunCacheTest6, 6, { 0, 2, 2, 2, 2, 2 } } };

static void
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

static void
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

static void
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

static void
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

static void
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

static void
RunCacheTest6(Text::font* Font)
{
  int   ParamCount  = 1;
  vec4  Color[1]    = { { 0.75f, 0.75f, 0.75f, 1.0f } };
  char* TextLines[] = {
    "0", "1", "2", "3", "4", "5", "0", "1", "2", "3", "4", "5", "6",
  };

  int32_t TextureWidth;
  int32_t TextureHeight;

  float TextWidth  = 0.3f;
  float TextHeight = 0.1f;

  for(int i = 0; i < ArraySize(TextLines); i++)
  {
    float TextWidth  = 0.3f;
    float TextHeight = 0.1f;
    if(i % TEXTURE_CACHE_LINE_COUNT == 0)
    {
      for(int t = 0; t < TEXTURE_CACHE_LINE_COUNT; t++)
      {
        g_RequestsPerFrame[t] = 0;
      }
    }

    uint32_t TextureID =
      GetTextTextureID(Font, (int32_t)(10 * ((float)TextWidth / (float)TextHeight)), TextLines[i],
                       Color[0], &TextureWidth, &TextureHeight);
  }
}

void
Test::RunCacheTests(cache_test* Tests, Text::font* Font, int Count)
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
Test::SetUpAndRunCacheTests(Text::font* Font)
{
  RunCacheTests(g_Tests, Font, ArraySize(g_Tests));
}

