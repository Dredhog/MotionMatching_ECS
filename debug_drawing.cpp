#include "debug_drawing.h"
#include "camera.h"

#define SPHERE_MAX_COUNT 100
#define GIZMO_MAX_COUNT 100
#define TEXTURED_QUAD_MAX_COUNT 100
#define COLORED_QUAD_MAX_COUNT 100

mat4    g_OverlaySphereMatrices[SPHERE_MAX_COUNT];
vec4    g_OverlaySphereColors[SPHERE_MAX_COUNT];
int32_t g_OverlaySphereCount;

mat4    g_SphereMatrices[SPHERE_MAX_COUNT];
vec4    g_SphereColors[SPHERE_MAX_COUNT];
int32_t g_SphereCount;

mat4    g_OverlayGizmoMatrices[GIZMO_MAX_COUNT];
float   g_OverlayGizmoDepths[GIZMO_MAX_COUNT];
int32_t g_OverlayGizmoCount;

mat4    g_GizmoMatrices[GIZMO_MAX_COUNT];
int32_t g_GizmoCount;

mat4 g_OverlayTexturedQuadMatrices[TEXTURED_QUAD_MAX_COUNT];
mat4 g_OverlayColoredQuadMatrices[COLORED_QUAD_MAX_COUNT];

mat4 g_TexturedQuadMatrices[TEXTURED_QUAD_MAX_COUNT];
mat4 g_ColoredQuadMatrices[COLORED_QUAD_MAX_COUNT];

void
DEBUGPushWireframeSphere(const camera* Camera, vec3 Position, float Radius, bool Overlay,
                         vec4 Color)
{
  mat4 MVPMatrix = Math::MulMat4(Camera->VPMatrix, Math::MulMat4(Math::Mat4Translate(Position),
                                                                 Math::Mat4Scale(Radius)));
  if(Overlay)
  {
    assert(0 <= g_OverlaySphereCount && g_OverlaySphereCount < SPHERE_MAX_COUNT);
    g_OverlaySphereColors[g_OverlaySphereCount]     = Color;
    g_OverlaySphereMatrices[g_OverlaySphereCount++] = MVPMatrix;
  }
  else
  {
    assert(0 <= g_SphereCount && g_SphereCount < SPHERE_MAX_COUNT);
    g_SphereColors[g_SphereCount]     = Color;
    g_SphereMatrices[g_SphereCount++] = MVPMatrix;
  }
}
void
DEBUGPushGizmo(const camera* Camera, const mat4* GizmoBase, bool Overlay)
{
  if(Overlay)
  {
    assert(0 <= g_OverlayGizmoCount && g_OverlayGizmoCount < GIZMO_MAX_COUNT);
    mat4  MVMatrix   = Math::MulMat4(Camera->ViewMatrix, *GizmoBase);
    mat4  MVPMatrix  = Math::MulMat4(Camera->ProjectionMatrix, MVMatrix);
    float GizmoDepth = Math::GetTranslationVec3(MVMatrix).Z;

    g_OverlayGizmoMatrices[g_OverlayGizmoCount] = MVPMatrix;
    g_OverlayGizmoDepths[g_OverlayGizmoCount++] = GizmoDepth;
  }
}

void
DEBUGDrawGizmos(game_state* GameState)
{
  glUseProgram(GameState->R.ShaderGizmo);
  for(int g = 0; g < g_OverlayGizmoCount; g++)
  {
    glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderGizmo, "mat_mvp"), 1, GL_FALSE,
                       g_OverlayGizmoMatrices[g].e);
    glUniform1f(glGetUniformLocation(GameState->R.ShaderGizmo, "depth"), g_OverlayGizmoDepths[g]);
    for(int i = 0; i < GameState->GizmoModel->MeshCount; i++)
    {
      glBindVertexArray(GameState->GizmoModel->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->GizmoModel->Meshes[i]->IndiceCount, GL_UNSIGNED_INT,
                     0);
    }
  }
  g_OverlayGizmoCount = 0;
  g_GizmoCount        = 0;
  glBindVertexArray(0);
}

void
DEBUGDrawWireframeSpheres(game_state* GameState)
{
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glUseProgram(GameState->R.ShaderColor);
  for(int i = 0; i < g_SphereCount; i++)
  {
    glUniform4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_color"), 1,
                 (float*)&g_SphereColors[i]);
    glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "mat_mvp"), 1, GL_FALSE,
                       g_SphereMatrices[i].e);
    glBindVertexArray(GameState->SphereModel->Meshes[0]->VAO);
    glDrawElements(GL_TRIANGLES, GameState->SphereModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT,
                   0);
  }
  glBindVertexArray(0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  g_SphereCount        = 0;
  g_OverlaySphereCount = 0;
}

void
DEBUGDrawQuad(game_state* GameState, vec3 LowerLeft, float Width, float Height, vec4 Color,
              bool DepthEnabled)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  vec3 Dimension = vec3{ Width, Height, 0 } * 2.0f;
  LowerLeft *= 2.0f;
  glUseProgram(GameState->R.ShaderQuad);
  glUniform4fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_color"), 1, (float*)&Color);
  glUniform3fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_position"), 1, (float*)&LowerLeft);
  glUniform2fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_dimension"), 1, (float*)&Dimension);
  glBindVertexArray(GameState->QuadModel->Meshes[1]->VAO);
  glDrawElements(GL_TRIANGLES, GameState->QuadModel->Meshes[1]->IndiceCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void
DEBUGDrawTopLeftQuad(game_state* GameState, vec3 LowerLeft, float Width, float Height, vec4 Color,
                     bool DepthEnabled)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  vec3 Dimension = vec3{ Width, Height, 0 } * 2.0f;
  LowerLeft *= 2.0f;
  glUseProgram(GameState->R.ShaderQuad);
  glUniform4fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_color"), 1, (float*)&Color);
  glUniform3fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_position"), 1, (float*)&LowerLeft);
  glUniform2fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_dimension"), 1, (float*)&Dimension);
  glBindVertexArray(GameState->QuadModel->Meshes[2]->VAO);
  glDrawElements(GL_TRIANGLES, GameState->QuadModel->Meshes[2]->IndiceCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void
DEBUGDrawCenteredQuad(game_state* GameState, vec3 Center, float Width, float Height, vec4 Color,
                      bool DepthEnabled)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  vec3 Dimension = vec3{ Width, Height, 0 } * 2.0f;
  Center *= 2.0f;
  glUseProgram(GameState->R.ShaderQuad);
  glUniform4fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_color"), 1, (float*)&Color);
  glUniform3fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_position"), 1, (float*)&Center);
  glUniform2fv(glGetUniformLocation(GameState->R.ShaderQuad, "g_dimension"), 1, (float*)&Dimension);
  glBindVertexArray(GameState->QuadModel->Meshes[0]->VAO);
  glDrawElements(GL_TRIANGLES, GameState->QuadModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void
DEBUGDrawTopLeftTexturedQuad(game_state* GameState, int32_t TextureID, vec3 LowerLeft, float Width,
                             float Height, bool DepthEnabled)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  vec3 Dimension = vec3{ Width, Height, 0 } * 2.0f;
  LowerLeft *= 2.0f;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindTexture(GL_TEXTURE_2D, TextureID);
  glUseProgram(GameState->R.ShaderTexturedQuad);
  glUniform3fv(glGetUniformLocation(GameState->R.ShaderTexturedQuad, "g_position"), 1,
               (float*)&LowerLeft);
  glUniform2fv(glGetUniformLocation(GameState->R.ShaderTexturedQuad, "g_dimension"), 1,
               (float*)&Dimension);
  glBindVertexArray(GameState->QuadModel->Meshes[2]->VAO);
  glDrawElements(GL_TRIANGLES, GameState->QuadModel->Meshes[2]->IndiceCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
}

void
DEBUGDrawTexturedQuad(game_state* GameState, int32_t TextureID, vec3 LowerLeft, float Width,
                      float Height, bool DepthEnabled)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  vec3 Dimension = vec3{ Width, Height, 0 } * 2.0f;
  LowerLeft *= 2.0f;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindTexture(GL_TEXTURE_2D, TextureID);
  glUseProgram(GameState->R.ShaderTexturedQuad);
  glUniform3fv(glGetUniformLocation(GameState->R.ShaderTexturedQuad, "g_position"), 1,
               (float*)&LowerLeft);
  glUniform2fv(glGetUniformLocation(GameState->R.ShaderTexturedQuad, "g_dimension"), 1,
               (float*)&Dimension);
  glBindVertexArray(GameState->QuadModel->Meshes[1]->VAO);
  glDrawElements(GL_TRIANGLES, GameState->QuadModel->Meshes[1]->IndiceCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
}

void
DEBUGDrawUnflippedTexturedQuad(game_state* GameState, int32_t TextureID, vec3 LowerLeft,
                               float Width, float Height, bool DepthEnabled)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  vec3 Dimension = vec3{ Width, Height, 0 } * 2.0f;
  LowerLeft *= 2.0f;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindTexture(GL_TEXTURE_2D, TextureID);
  glUseProgram(GameState->R.ShaderTexturedQuad);
  glUniform3fv(glGetUniformLocation(GameState->R.ShaderTexturedQuad, "g_position"), 1,
               (float*)&LowerLeft);
  glUniform2fv(glGetUniformLocation(GameState->R.ShaderTexturedQuad, "g_dimension"), 1,
               (float*)&Dimension);
  glBindVertexArray(GameState->QuadModel->Meshes[3]->VAO);
  glDrawElements(GL_TRIANGLES, GameState->QuadModel->Meshes[3]->IndiceCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
}
