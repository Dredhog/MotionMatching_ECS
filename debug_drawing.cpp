#include "debug_drawing.h"
#include "camera.h"

#define SPHERE_MAX_COUNT 100
#define GIZMO_MAX_COUNT 100
#define TEXTURED_QUAD_MAX_COUNT 300
#define COLORED_QUAD_MAX_COUNT 300

mat4    g_SphereMatrices[SPHERE_MAX_COUNT];
vec4    g_SphereColors[SPHERE_MAX_COUNT];
int32_t g_SphereCount;

mat4    g_GizmoMatrices[GIZMO_MAX_COUNT];
float   g_GizmoDepths[GIZMO_MAX_COUNT];
int32_t g_GizmoCount;

struct QuadInstance
{
  vec4    Color;
  vec3    LowerLeft;
  vec3    Dimensions;
  int32_t TextureID;
};

QuadInstance g_ColoredQuads[TEXTURED_QUAD_MAX_COUNT];
int32_t      g_ColoredQuadCount;

QuadInstance g_TexturedQuads[COLORED_QUAD_MAX_COUNT];
int32_t      g_TexturedQuadCount;

void
Debug::PushWireframeSphere(const camera* Camera, vec3 Position, float Radius, vec4 Color)
{
  mat4 MVPMatrix = Math::MulMat4(Camera->VPMatrix, Math::MulMat4(Math::Mat4Translate(Position),
                                                                 Math::Mat4Scale(Radius)));
  assert(0 <= g_SphereCount && g_SphereCount < SPHERE_MAX_COUNT);
  g_SphereColors[g_SphereCount]     = Color;
  g_SphereMatrices[g_SphereCount++] = MVPMatrix;
}
void
Debug::PushGizmo(const camera* Camera, const mat4* GizmoBase)
{
  assert(0 <= g_GizmoCount && g_GizmoCount < GIZMO_MAX_COUNT);
  mat4  MVMatrix   = Math::MulMat4(Camera->ViewMatrix, *GizmoBase);
  mat4  MVPMatrix  = Math::MulMat4(Camera->ProjectionMatrix, MVMatrix);
  float GizmoDepth = Math::GetTranslationVec3(MVMatrix).Z;

  g_GizmoMatrices[g_GizmoCount] = MVPMatrix;
  g_GizmoDepths[g_GizmoCount]   = GizmoDepth;
  ++g_GizmoCount;
}

void
Debug::PushQuad(vec3 BottomLeft, float Width, float Height, vec4 Color)
{
  assert(0 <= g_ColoredQuadCount && g_ColoredQuadCount < COLORED_QUAD_MAX_COUNT);
  g_ColoredQuads[g_ColoredQuadCount]            = {};
  g_ColoredQuads[g_ColoredQuadCount].Dimensions = { Width, Height };
  g_ColoredQuads[g_ColoredQuadCount].LowerLeft  = BottomLeft;
  g_ColoredQuads[g_ColoredQuadCount].Color      = Color;
  ++g_ColoredQuadCount;
}

void
Debug::PushTexturedQuad(int32_t TextureID, vec3 BottomLeft, float Width, float Height)
{
  assert(0 <= g_TexturedQuadCount && g_TexturedQuadCount < TEXTURED_QUAD_MAX_COUNT);
  g_TexturedQuads[g_TexturedQuadCount]            = {};
  g_TexturedQuads[g_TexturedQuadCount].Dimensions = { Width, Height };
  g_TexturedQuads[g_TexturedQuadCount].LowerLeft  = BottomLeft;
  g_TexturedQuads[g_TexturedQuadCount].TextureID  = TextureID;
  ++g_TexturedQuadCount;
}

void
Debug::PushTopLeftQuad(vec3 TopLeft, float Width, float Height, vec4 Color)
{
  assert(0 <= g_ColoredQuadCount && g_ColoredQuadCount < COLORED_QUAD_MAX_COUNT);
  g_ColoredQuads[g_ColoredQuadCount]            = {};
  g_ColoredQuads[g_ColoredQuadCount].Dimensions = { Width, Height };
  g_ColoredQuads[g_ColoredQuadCount].LowerLeft  = TopLeft - vec3{ 0, Height };
  g_ColoredQuads[g_ColoredQuadCount].Color      = Color;
  ++g_ColoredQuadCount;
}

void
Debug::PushTopLeftTexturedQuad(int32_t TextureID, vec3 TopLeft, float Width, float Height)
{
  assert(0 <= g_TexturedQuadCount && g_TexturedQuadCount < TEXTURED_QUAD_MAX_COUNT);
  g_TexturedQuads[g_TexturedQuadCount]            = {};
  g_TexturedQuads[g_TexturedQuadCount].Dimensions = { Width, Height };
  g_TexturedQuads[g_TexturedQuadCount].LowerLeft  = TopLeft - vec3{ 0, Height };
  g_TexturedQuads[g_TexturedQuadCount].TextureID  = TextureID;
  ++g_TexturedQuadCount;
}

void
Debug::DrawGizmos(game_state* GameState)
{
  Render::model* GizmoModel = GameState->Resources.GetModel(GameState->GizmoModelID);
  glUseProgram(GameState->R.ShaderGizmo);
  for(int g = 0; g < g_GizmoCount; g++)
  {
    glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderGizmo, "mat_mvp"), 1, GL_FALSE,
                       g_GizmoMatrices[g].e);
    glUniform1f(glGetUniformLocation(GameState->R.ShaderGizmo, "depth"), g_GizmoDepths[g]);
    for(int i = 0; i < GizmoModel->MeshCount; i++)
    {
      glBindVertexArray(GizmoModel->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GizmoModel->Meshes[i]->IndiceCount, GL_UNSIGNED_INT, 0);
    }
  }
  glBindVertexArray(0);
  g_GizmoCount = 0;
}

void
Debug::DrawWireframeSpheres(game_state* GameState)
{
  if(GameState->DrawDebugSpheres)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(GameState->R.ShaderColor);
    Render::model* SphereModel = GameState->Resources.GetModel(GameState->SphereModelID);
    glBindVertexArray(SphereModel->Meshes[0]->VAO);

    // So as not to corrupt the position by the old bone data
    {
      mat4 Mat4Zeros = {};
      glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderID, "g_boneMatrices"), 1, GL_FALSE,
                         Mat4Zeros.e);
    }
    for(int i = 0; i < g_SphereCount; i++)
    {
      glUniform4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_color"), 1,
                   (float*)&g_SphereColors[i]);
      glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "mat_mvp"), 1, GL_FALSE,
                         g_SphereMatrices[i].e);
      glDrawElements(GL_TRIANGLES, SphereModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  g_SphereCount = 0;
}

void
Debug::DrawColoredQuads(game_state* GameState)
{
  glUseProgram(GameState->R.ShaderQuad);
  Render::model* QuadModel = GameState->Resources.GetModel(GameState->QuadModelID);
  glBindVertexArray(QuadModel->Meshes[1]->VAO);
  for(int i = 0; i < g_ColoredQuadCount; i++)
  {
    glUniform4fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_color"), 1,
                 (float*)&g_ColoredQuads[i].Color);
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_position"), 1,
                 (float*)&g_ColoredQuads[i].LowerLeft);
    glUniform2fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_dimension"), 1,
                 (float*)&g_ColoredQuads[i].Dimensions);
    glDrawElements(GL_TRIANGLES, QuadModel->Meshes[1]->IndiceCount, GL_UNSIGNED_INT, 0);
  }
  glBindVertexArray(0);
  g_ColoredQuadCount = 0;
}

void
Debug::DrawTexturedQuads(game_state* GameState)
{
  glEnable(GL_BLEND);
  Render::model* QuadModel = GameState->Resources.GetModel(GameState->QuadModelID);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindVertexArray(QuadModel->Meshes[1]->VAO);
  for(int i = 0; i < g_TexturedQuadCount; i++)
  {
    glBindTexture(GL_TEXTURE_2D, g_TexturedQuads[i].TextureID);
    glUseProgram(GameState->R.ShaderTexturedQuad);
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderTexturedQuad, "g_position"), 1,
                 (float*)&g_TexturedQuads[i].LowerLeft);
    glUniform2fv(glGetUniformLocation(GameState->R.ShaderTexturedQuad, "g_dimension"), 1,
                 (float*)&g_TexturedQuads[i].Dimensions);
    glDrawElements(GL_TRIANGLES, QuadModel->Meshes[1]->IndiceCount, GL_UNSIGNED_INT, 0);
  }
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
  g_TexturedQuadCount = 0;
}
