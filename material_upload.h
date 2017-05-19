#pragma once

void
UnsetMaterial(render_data* RenderData, int32_t MaterialIndex)
{
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  glDepthFunc(GL_LESS);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
}

uint32_t
SetMaterial(game_state* GameState, camera* Camera, material* Material)
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
    glUseProgram(GameState->R.ShaderPhong);
    glUniform1i(glGetUniformLocation(GameState->R.ShaderPhong, "flags"), Material->Phong.Flags);
    glUniform4fv(glGetUniformLocation(GameState->R.ShaderPhong, "material.diffuseColor"), 1,
                 (float*)&Material->Phong.DiffuseColor);
    glUniform1i(glGetUniformLocation(GameState->R.ShaderPhong, "material.diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(GameState->R.ShaderPhong, "material.specularMap"), 1);
    glUniform1i(glGetUniformLocation(GameState->R.ShaderPhong, "material.normalMap"), 2);
    glUniform1f(glGetUniformLocation(GameState->R.ShaderPhong, "material.shininess"),
                Material->Phong.Shininess);
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderPhong, "lightPosition"), 1,
                 (float*)&GameState->R.LightPosition);
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderPhong, "light.ambient"), 1,
                 (float*)&GameState->R.LightAmbientColor);
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderPhong, "light.diffuse"), 1,
                 (float*)&GameState->R.LightDiffuseColor);
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderPhong, "light.specular"), 1,
                 (float*)&GameState->R.LightSpecularColor);
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderPhong, "cameraPosition"), 1,
                 (float*)&Camera->Position);
    uint32_t DiffuseTexture = (Material->Phong.Flags & PHONG_UseDiffuseMap)
                                ? GameState->Resources.GetTexture(Material->Phong.DiffuseMapID)
                                : 0;
    uint32_t SpecularTexture = (Material->Phong.Flags & PHONG_UseSpecularMap)
                                 ? GameState->Resources.GetTexture(Material->Phong.SpecularMapID)
                                 : 0;
    uint32_t NormalTexture = (Material->Phong.Flags & PHONG_UseNormalMap)
                               ? GameState->Resources.GetTexture(Material->Phong.NormalMapID)
                               : 0;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, SpecularTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, NormalTexture);
    glActiveTexture(GL_TEXTURE0);
    return GameState->R.ShaderPhong;
  }
  else if(Material->Common.ShaderType == SHADER_Color)
  {
    glUseProgram(GameState->R.ShaderColor);
    glUniform4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_color"), 1,
                 (float*)&Material->Color.Color);
    return GameState->R.ShaderColor;
  }
  return -1;
}

uint32_t
SetMaterial(game_state* GameState, camera* Camera, int32_t MaterialIndex)
{
  assert(0 <= MaterialIndex && MaterialIndex < GameState->R.MaterialCount);
  return SetMaterial(GameState, Camera, &GameState->R.Materials[MaterialIndex]);
}

#if 0
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
