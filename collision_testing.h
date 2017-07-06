#pragma once

#include "collision.h"

void
Reverse3Simplex(vec3* Simplex)
{
  vec3 Temp  = Simplex[3];
  Simplex[3] = Simplex[0];
  Simplex[0] = Temp;
  Temp       = Simplex[2];
  Simplex[2] = Simplex[1];
  Simplex[1] = Temp;
}

void
DrawModel(game_state* GameState, Render::model* Model)
{
  vec4            Color        = { 1.0f, 0.0f, 0.0f, 1.0f };
  Anim::transform NewTransform = {};
  mat4            MVP          = Math::MulMat4(GameState->Camera.VPMatrix, TransformToMat4(&NewTransform));

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glUseProgram(GameState->R.ShaderColor);
  glBindVertexArray(Model->Meshes[0]->VAO);

  // So as not to corrupt the position by the old bone data
  {
    mat4 Mat4Zeros = {};
    glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderID, "g_boneMatrices"), 1, GL_FALSE, Mat4Zeros.e);
  }
  glUniform4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_color"), 1, (float*)&Color);
  glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "mat_mvp"), 1, GL_FALSE, MVP.e);
  glDrawElements(GL_TRIANGLES, Model->Meshes[0]->IndiceCount, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void
CollisionTesting(Render::model* ModelA, Render::model* ModelB)
{
  vec3 Simplex[50];

  for(int i = 0; i < ModelA->MeshCount; i++)
  {
    for(int j = 0; j < ModelB->MeshCount; j++)
    {
      bool IsColliding = GJK(Simplex, ModelA->Meshes[i], ModelB->Meshes[j]);

      if(IsColliding)
      {
        printf("Collision detected between ModelA.Mesh[%d] and ModelB.Mesh[%d]!\n", i, j);

        vec3 SolutionVector = EPA(Simplex, 4, ModelA->Meshes[i], ModelB->Meshes[j]);

        printf("SolutionVector = { %f, %f, %f }\n", SolutionVector.X, SolutionVector.Y, SolutionVector.Z);
      }
      else
      {
        // printf("No collision detected.\n");
      }
    }
  }
}
