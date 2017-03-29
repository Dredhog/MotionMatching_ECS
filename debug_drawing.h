#pragma once

#include "game.h"

void
DEBUGDrawGizmos(game_state* GameState, mat4* GizmoBases, int32_t GizmoCount,
                bool DepthEnabled = false)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  glUseProgram(GameState->ShaderGizmo);
  for(int g = 0; g < GizmoCount; g++)
  {
    mat4  MVMatrix   = Math::MulMat4(GameState->Camera.ViewMatrix, GizmoBases[g]);
    float GizmoDepth = Math::GetTranslationVec3(MVMatrix).Z;

    mat4 MVPMatrix = Math::MulMat4(GameState->Camera.ProjectionMatrix, MVMatrix);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderGizmo, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    glUniform1f(glGetUniformLocation(GameState->ShaderGizmo, "depth"), GizmoDepth);
    for(int i = 0; i < GameState->GizmoModel->MeshCount; i++)
    {
      glBindVertexArray(GameState->GizmoModel->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->GizmoModel->Meshes[i]->IndiceCount, GL_UNSIGNED_INT,
                     0);
    }
  }
  glBindVertexArray(0);
}

void
DEBUGDrawQuad(game_state* GameState, vec3 LowerLeft, float Width, float Height,
              vec3 Color = { 1, 1, 1 }, bool DepthEnabled = false)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  vec3 Dimension = vec3{ Width, Height, 0 } * 2.0f;
  LowerLeft *= 2.0f;
  glUseProgram(GameState->ShaderQuad);
  glUniform3fv(glGetUniformLocation(GameState->ShaderQuad, "g_color"), 1, (float*)&Color);
  glUniform3fv(glGetUniformLocation(GameState->ShaderQuad, "g_position"), 1, (float*)&LowerLeft);
  glUniform2fv(glGetUniformLocation(GameState->ShaderQuad, "g_dimension"), 1, (float*)&Dimension);
  glUseProgram(GameState->ShaderQuad);
  glBindVertexArray(GameState->QuadModel->Meshes[1]->VAO);
  glDrawElements(GL_TRIANGLES, GameState->QuadModel->Meshes[1]->IndiceCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void
DEBUGDrawCenteredQuad(game_state* GameState, vec3 Center, float Width, float Height,
                      vec3 Color = { 1, 1, 1 }, bool DepthEnabled = false)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  vec3 Dimension = vec3{ Width, Height, 0 } * 2.0f;
  Center *= 2.0f;
  glUseProgram(GameState->ShaderQuad);
  glUniform3fv(glGetUniformLocation(GameState->ShaderQuad, "g_color"), 1, (float*)&Color);
  glUniform3fv(glGetUniformLocation(GameState->ShaderQuad, "g_position"), 1, (float*)&Center);
  glUniform2fv(glGetUniformLocation(GameState->ShaderQuad, "g_dimension"), 1, (float*)&Dimension);
  glBindVertexArray(GameState->QuadModel->Meshes[0]->VAO);
  glDrawElements(GL_TRIANGLES, GameState->QuadModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void
DEBUGDrawTexturedQuad(game_state* GameState, int32_t TextureID, vec3 LowerLeft, float Width,
                      float Height, vec3 Color = { 1, 1, 1 }, bool DepthEnabled = false)
{
  if(!DepthEnabled)
  {
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  glBindTexture(GL_TEXTURE_2D, TextureID);
  vec3 Dimension = vec3{ Width, Height, 0 } * 2.0f;
  LowerLeft *= 2.0f;
  glUseProgram(GameState->ShaderTexturedQuad);
  glUniform3fv(glGetUniformLocation(GameState->ShaderTexturedQuad, "g_position"), 1,
               (float*)&LowerLeft);
  glUniform2fv(glGetUniformLocation(GameState->ShaderTexturedQuad, "g_dimension"), 1,
               (float*)&Dimension);
  glBindVertexArray(GameState->QuadModel->Meshes[1]->VAO);
  glDrawElements(GL_TRIANGLES, GameState->QuadModel->Meshes[1]->IndiceCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}
