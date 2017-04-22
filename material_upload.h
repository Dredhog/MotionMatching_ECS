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
    glUniform1i(glGetUniformLocation(RenderData->ShaderPhong, "flags"), Material->Phong.Flags);
    glUniform4fv(glGetUniformLocation(RenderData->ShaderPhong, "material.diffuseColor"), 1,
                 (float*)&Material->Phong.DiffuseColor);
    glUniform1i(glGetUniformLocation(RenderData->ShaderPhong, "material.diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(RenderData->ShaderPhong, "material.specularMap"), 1);
    glUniform1i(glGetUniformLocation(RenderData->ShaderPhong, "material.normalMap"), 2);
    glUniform1f(glGetUniformLocation(RenderData->ShaderPhong, "material.shininess"),
                Material->Phong.Shininess);
    glUniform3fv(glGetUniformLocation(RenderData->ShaderPhong, "lightPosition"), 1,
                 (float*)&RenderData->LightPosition);
    glUniform3fv(glGetUniformLocation(RenderData->ShaderPhong, "light.ambient"), 1,
                 (float*)&RenderData->LightAmbientColor);
    glUniform3fv(glGetUniformLocation(RenderData->ShaderPhong, "light.diffuse"), 1,
                 (float*)&RenderData->LightDiffuseColor);
    glUniform3fv(glGetUniformLocation(RenderData->ShaderPhong, "light.specular"), 1,
                 (float*)&RenderData->LightSpecularColor);
    glUniform3fv(glGetUniformLocation(RenderData->ShaderPhong, "cameraPosition"), 1,
                 (float*)&Camera->Position);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, RenderData->Textures[Material->Phong.DiffuseMapIndex]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, RenderData->Textures[Material->Phong.SpecularMapIndex]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, RenderData->Textures[Material->Phong.NormalMapIndex]);
    glActiveTexture(GL_TEXTURE0);
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
