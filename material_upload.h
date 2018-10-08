#pragma once

#include "shader_def.h"

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
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderPhong, "material.ambientColor"), 1,
                 (float*)&Material->Phong.AmbientColor);
    glUniform4fv(glGetUniformLocation(GameState->R.ShaderPhong, "material.diffuseColor"), 1,
                 (float*)&Material->Phong.DiffuseColor);
    glUniform3fv(glGetUniformLocation(GameState->R.ShaderPhong, "material.specularColor"), 1,
                 (float*)&Material->Phong.SpecularColor);
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
    assert(
      ((Material->Phong.Flags & PHONG_UseDiffuseMap) && Material->Phong.DiffuseMapID.Value > 0) ||
      !(Material->Phong.Flags & PHONG_UseDiffuseMap));
    assert(
      ((Material->Phong.Flags & PHONG_UseSpecularMap) && Material->Phong.SpecularMapID.Value > 0) ||
      !(Material->Phong.Flags & PHONG_UseSpecularMap));
    assert(
      ((Material->Phong.Flags & PHONG_UseNormalMap) && Material->Phong.NormalMapID.Value > 0) ||
      !(Material->Phong.Flags & PHONG_UseNormalMap));

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
  else
  {
    struct shader_def* ShaderDef = NULL;
    assert(GetShaderDef(&ShaderDef, Material->Common.ShaderType));
    {

      int32_t CurrentGLTextureBindIndex  = GL_TEXTURE0;
      int32_t MaximalBoundGLTextureCount = 0;

      uint32_t CurrentShaderID = GetShaderID(ShaderDef);
      glUseProgram(CurrentShaderID);
      glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MaximalBoundGLTextureCount);

      named_shader_param_def ParamDef = {};

      ResetShaderDefIterator(ShaderDef);
      // TODO(Lukas): Move all glGetUniformLocation calls to shader_def.cpp and return actual
      // location variable inside of ParamDef
      while(GetNextShaderParam(&ParamDef, ShaderDef))
      {
        uint8_t* ParamPtr = (((uint8_t*)Material) + ParamDef.OffsetIntoMaterial);
        switch(ParamDef.Type)
        {
          case SHADER_PARAM_TYPE_Int:
          {
            int32_t Value = *((int32_t*)ParamPtr);
            glUniform1i(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), Value);
          }
          break;
          case SHADER_PARAM_TYPE_Bool:
          {
            bool    BoolValue = *((bool*)ParamPtr);
            int32_t Value     = (int32_t)BoolValue;
            glUniform1i(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), Value);
          }
          break;
          case SHADER_PARAM_TYPE_Float:
          {
            float Value = *((float*)ParamPtr);
            glUniform1f(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), Value);
          }
          break;
          case SHADER_PARAM_TYPE_Vec3:
          {
            vec3 Value = *((vec3*)ParamPtr);
            glUniform3fv(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), 1,
                         (float*)ParamPtr);
          }
          break;
          case SHADER_PARAM_TYPE_Vec4:
          {
            glUniform4fv(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), 1,
                         (float*)ParamPtr);
          }
          break;
          case SHADER_PARAM_TYPE_Map:
          {
            rid RIDValue = *((rid*)ParamPtr);

            assert(CurrentGLTextureBindIndex < (GL_TEXTURE0 + MaximalBoundGLTextureCount));
            glActiveTexture(CurrentGLTextureBindIndex);
            glBindTexture(GL_TEXTURE_2D, GameState->Resources.GetTexture(RIDValue));
            glUniform1i(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName),
                        CurrentGLTextureBindIndex - GL_TEXTURE0);

            glActiveTexture(GL_TEXTURE0);
            CurrentGLTextureBindIndex++;
          }
          break;
        }
      }
      return CurrentShaderID;
    }
  }
  return -1;
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
