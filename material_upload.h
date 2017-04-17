#pragma once

void
UnsetMaterial(render_data* RenderData, int32_t MaterialIndex)
{
  uint32_t ShaderType = RenderData->Materials[MaterialIndex].Common.ShaderType;
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  glDepthFunc(GL_LESS);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
}

uint32_t
SetMaterial(render_data* RenderData, camera* Camera, material* Material)
{
  if(Material->Common.UseBlending)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  if(Material->Common.ShaderType == SHADER_Phong)
  {
    glUseProgram(RenderData->ShaderPhong);
    glUniform1f(glGetUniformLocation(RenderData->ShaderPhong, "ambient_strength"),
                Material->Phong.AmbientStrength);
    glUniform1f(glGetUniformLocation(RenderData->ShaderPhong, "specular_strength"),
                Material->Phong.SpecularStrength);
    glUniform3fv(glGetUniformLocation(RenderData->ShaderPhong, "light_position"), 1,
                 (float*)&RenderData->LightPosition);
    glUniform3fv(glGetUniformLocation(RenderData->ShaderPhong, "light_color"), 1,
                 (float*)&RenderData->LightColor);
    glUniform3fv(glGetUniformLocation(RenderData->ShaderPhong, "camera_position"), 1,
                 (float*)&Camera->Position);
    glBindTexture(GL_TEXTURE_2D, RenderData->Textures[Material->Phong.TextureIndex0]);
    return RenderData->ShaderPhong;
  }
  else if(Material->Common.ShaderType == SHADER_Color)
  {
    glUseProgram(RenderData->ShaderColor);
    glUniform4fv(glGetUniformLocation(RenderData->ShaderColor, "g_color"), 1,
                 (float*)&Material->Color.Color);
    return RenderData->ShaderColor;
  }
  return -1;
}

uint32_t
SetMaterial(render_data* RenderData, camera* Camera, int32_t MaterialIndex)
{
  assert(0 <= MaterialIndex && MaterialIndex < RenderData->MaterialCount);
  return SetMaterial(RenderData, Camera, &RenderData->Materials[MaterialIndex]);
}

#if 0
void
SetUpSkeletalPhongShader()
{
  glUseProgram(GameState->ShaderSkeletalPhong);
  glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "g_bone_matrices"), 20,
                     GL_FALSE, (float*)GameState->AnimEditor.HierarchicalModelSpaceMatrices);
  glUniform1f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "ambient_strength"),
              GameState->AmbientStrength);
  glUniform1f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "specular_strength"),
              GameState->SpecularStrength);
  glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "light_position"),
              GameState->LightPosition.X, GameState->LightPosition.Y, GameState->LightPosition.Z);
  glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "light_color"),
              GameState->LightColor.X, GameState->LightColor.Y, GameState->LightColor.Z);
  glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "camera_position"),
              GameState->Camera.Position.X, GameState->Camera.Position.Y,
              GameState->Camera.Position.Z);
}

void
SetUpBoneColorShader()
{
  glUseProgram(GameState->ShaderSkeletalBoneColor);
  glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "g_bone_matrices"),
                     20, GL_FALSE, (float*)GameState->AnimEditor.HierarchicalModelSpaceMatrices);
  glUniform3fv(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "g_bone_colors"), 20,
               (float*)&g_BoneColors);
}

void
SetUpCubemapShader(render_data* RenderData)
{
  glDepthFunc(GL_LEQUAL);
  glUseProgram(GameState->ShaderCubemap);
  glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderCubemap, "mat_projection"), 1, GL_FALSE,
                     RenderData->Camera.ProjectionMatrix.e);
  glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderCubemap, "mat_view"), 1, GL_FALSE,
                     Math::Mat3ToMat4(Math::Mat4ToMat3(GameState->Camera.ViewMatrix)).e);
  glBindTexture(GL_TEXTURE_CUBE_MAP, GameState->CubemapTexture);
  for(int i = 0; i < GameState->Cubemap->MeshCount; i++)
  {
    glBindVertexArray(GameState->Cubemap->Meshes[i]->VAO);
    glDrawElements(GL_TRIANGLES, GameState->Cubemap->Meshes[i]->IndiceCount, GL_UNSIGNED_INT, 0);
  }
}
#endif
