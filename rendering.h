// Note(Lukas): All rendering functions should leave the GL state in a usable condition fefore
// returning

void
DrawDebugBoxMesh(box_mesh BoxMesh, vec4 Color = { 0, 0, 1, 1 })
{
  Debug::PushLineStrip(BoxMesh.Points, 4, Color);
  Debug::PushLineStrip(BoxMesh.Points + 4, 4, Color);
  Debug::PushLine(BoxMesh.Points[0], BoxMesh.Points[3], Color);
  Debug::PushLine(BoxMesh.Points[4], BoxMesh.Points[7], Color);
  Debug::PushLine(BoxMesh.Points[0], BoxMesh.Points[4], Color);
  Debug::PushLine(BoxMesh.Points[1], BoxMesh.Points[5], Color);
  Debug::PushLine(BoxMesh.Points[2], BoxMesh.Points[6], Color);
  Debug::PushLine(BoxMesh.Points[3], BoxMesh.Points[7], Color);
}

void
RenderGBufferDataToTextures(game_state* GameState)
{
  TIMED_BLOCK(GBufferPass);
  glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.GBufferFBO);
  glClearColor(0.0f, 0.0f, -GameState->Camera.FarClipPlane, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // TODO(Lukas) SORT(MeshInstances,  ByMesh);
  {
    Render::mesh* PreviousMesh   = nullptr;
    uint32_t GeomPrePassShaderID = GameState->Resources.GetShader(GameState->R.ShaderGeomPreePass);
    glUseProgram(GeomPrePassShaderID);
    for(int i = 0; i < GameState->R.MeshInstanceCount; i++)
    {
			const mesh_instance* MeshInstance = &GameState->R.MeshInstances[i];
      Render::mesh* CurrentMesh = MeshInstance->Mesh;

      if(CurrentMesh != PreviousMesh)
      {
        glBindVertexArray(CurrentMesh->VAO);
        PreviousMesh = CurrentMesh;
      }

      // TODO(Lukas) Add logic for bone matrix submission

      glUniformMatrix4fv(glGetUniformLocation(GeomPrePassShaderID, "mat_mvp"), 1, GL_FALSE,
                         MeshInstance->MVP.e);
      glUniformMatrix4fv(glGetUniformLocation(GeomPrePassShaderID, "mat_prev_mvp"), 1, GL_FALSE,
                         MeshInstance->PrevMVP.e);
      glUniformMatrix4fv(glGetUniformLocation(GeomPrePassShaderID, "mat_model"), 1, GL_FALSE,
                         Math::MulMat4(GameState->Camera.InvVPMatrix, MeshInstance->MVP).e);
      glUniformMatrix4fv(glGetUniformLocation(GeomPrePassShaderID, "mat_view"), 1, GL_FALSE,
                         GameState->Camera.ViewMatrix.e);
      glDrawElements(GL_TRIANGLES, CurrentMesh->IndiceCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // RENDER DEPTH INFORMATION TO TEXTURE
  {
    GLuint RenderDepthMapShaderID = GameState->Resources.GetShader(GameState->R.RenderDepthMap);
    glUseProgram(RenderDepthMapShaderID);
    glUniform1f(glGetUniformLocation(RenderDepthMapShaderID, "cameraNearPlane"),
                GameState->Camera.NearClipPlane);
    glUniform1f(glGetUniformLocation(RenderDepthMapShaderID, "cameraFarPlane"),
                GameState->Camera.FarClipPlane);

    glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.DepthTextureFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    {
      int TexIndex = 1;

      glActiveTexture(GL_TEXTURE0 + TexIndex);
      glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferDepthTexID);
      glUniform1i(glGetUniformLocation(RenderDepthMapShaderID, "DepthMap"), TexIndex);
      DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
      glActiveTexture(GL_TEXTURE0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

// TODO(Lukas) Add blurring to ssao shader as well as a randomization kernel buffer and more
// samples
void
RenderSSAOToTexture(game_state* GameState)
{
  TIMED_BLOCK(SSAOPass);

  uint32_t SSAOShaderID = GameState->Resources.GetShader(GameState->R.ShaderSSAO);
  glUseProgram(SSAOShaderID);
  {
    glUniform3fv(glGetUniformLocation(SSAOShaderID, "u_SampleVectors"), SSAO_SAMPLE_VECTOR_COUNT,
                 (float*)&GameState->R.SSAOSampleVectors);
  }
  {
    glUniformMatrix4fv(glGetUniformLocation(SSAOShaderID, "u_mat_projection"), 1, GL_FALSE,
                       GameState->Camera.ProjectionMatrix.e);
  }
  {
    glUniform1f(glGetUniformLocation(SSAOShaderID, "u_SamplingRadius"),
                GameState->R.SSAOSamplingRadius);
  }
  {
    int tex_index = 1;
    glActiveTexture(GL_TEXTURE0 + tex_index);
    glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferNormalTexID);
    glUniform1i(glGetUniformLocation(SSAOShaderID, "u_NormalMap"), tex_index);
  }
  {
    int tex_index = 2;
    glActiveTexture(GL_TEXTURE0 + tex_index);
    glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferPositionTexID);
    glUniform1i(glGetUniformLocation(SSAOShaderID, "u_PositionMap"), tex_index);
  }
  glActiveTexture(GL_TEXTURE0);
  DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
}

void
RenderShadowmapCascadesToTextures(game_state* GameState)
{
  TIMED_BLOCK(ShadowmapPass);
  // CASCADED SHADOW MAP GENERATION
  if(GameState->R.RealTimeDirectionalShadows || GameState->R.RecomputeDirectionalShadows)
  {
    {
      {
        float DegToRad = 3.14159f / 180.0f;
        // The 0.01f is for avoiding crossing the up vector with itself later on
        GameState->R.Sun.Direction = Math::Normalized(
          Math::MulMat3Vec3(Math::Mat3RotateY(GameState->R.Sun.RotationY),
                            Math::MulMat3Vec3(Math::Mat3RotateZ(GameState->R.Sun.RotationZ + 0.01f),
                                              vec3{ -1, 0, 0 })));
      }
      // Compute shadowmap cascade VP matrices
      {
        frustum_def Frustum = {};
#if 1
        Frustum.Forward = Math::Normalized(GameState->Camera.Forward);
        Frustum.Right   = Math::Normalized(Math::Cross(Frustum.Forward, vec3{ 0, 1, 0 }));
        Frustum.Up      = Math::Cross(Frustum.Right, Frustum.Forward);
        Frustum.Origin  = GameState->Camera.Position;
#else
        Frustum.Forward = vec3{ 0, 0, -1 };
        Frustum.Right   = vec3{ 1, 0, 0 };
        Frustum.Up      = vec3{ 0, 1, 0 };
        Frustum.Origin  = {};
#endif

        Frustum.ViewAngle = GameState->Camera.FieldOfView;
        Frustum.Aspect    = SCREEN_WIDTH / (float)SCREEN_HEIGHT;

        for(int i = 0; i < SHADOWMAP_CASCADE_COUNT; i++)
        {
          Frustum.Near = (i == 0) ? GameState->Camera.NearClipPlane
                                  : GameState->R.Sun.CascadeFarPlaneDistances[i - 1];

          Frustum.Far = GameState->R.Sun.CascadeFarPlaneDistances[i];

          box_mesh FrustumMesh = GetFrustumBoxMesh(Frustum);

          obb_def OBB = GetFrustumOBBFromDir(GameState->R.Sun.Direction, FrustumMesh);

          if(GameState->DrawShadowCascadeVolumes)
          {
            Debug::PushWireframeSphere(OBB.NearCenter, 0.1f, { 1, 0, 1, 1 });
            DrawDebugBoxMesh(FrustumMesh, { 1, 0, 0, 1 });
            DrawDebugBoxMesh(GetOBBBoxMesh(OBB), { 0, 0, 1, 1 });
          }

          mat4 ProjectionMatrix =
            Math::Mat4Orthogonal(-OBB.HalfWidth, OBB.HalfWidth, -OBB.HalfHeight, OBB.HalfHeight,
                                 -ClampFloat(20, Frustum.Far, 20), OBB.NearFarDist);
          mat4 ViewMatrix =
            Math::Mat4Camera(OBB.NearCenter, GameState->R.Sun.Direction, vec3{ 0.0f, 1.0f, 0.0f });
          GameState->R.Sun.CascadeVP[i] = Math::MulMat4(ProjectionMatrix, ViewMatrix);
        }
      }
    }

    for(int i = 0; i < SHADOWMAP_CASCADE_COUNT; i++)
    {
      uint32_t SunDepthShaderID = GameState->Resources.GetShader(GameState->R.ShaderSunDepth);
      glUseProgram(SunDepthShaderID);
      glUniformMatrix4fv(glGetUniformLocation(SunDepthShaderID, "mat_sun_vp"), 1, GL_FALSE,
                         GameState->R.Sun.CascadeVP[i].e);
      glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
      glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.ShadowMapFBOs[i]);
      glClear(GL_DEPTH_BUFFER_BIT);
#if FIGHT_PETER_PAN
      glDisable(GL_CULL_FACE);
      // glEnable(GL_CULL_FACE);
      // glCullFace(GL_FRONT);
#endif

      {
        Render::mesh* PreviousMesh = nullptr;
        for(int i = 0; i < GameState->R.MeshInstanceCount; i++)
        {
          Render::mesh* CurrentMesh      = GameState->R.MeshInstances[i].Mesh;
          mat4          CurrentMVPMatrix = GameState->R.MeshInstances[i].MVP;

          if(CurrentMesh != PreviousMesh)
          {
            glBindVertexArray(CurrentMesh->VAO);
            PreviousMesh = CurrentMesh;
          }

          glUniformMatrix4fv(glGetUniformLocation(SunDepthShaderID, "mat_model"), 1, GL_FALSE,
                             Math::MulMat4(GameState->Camera.InvVPMatrix, CurrentMVPMatrix).e);
          glDrawElements(GL_TRIANGLES, CurrentMesh->IndiceCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
      }

#if FIGHT_PETER_PAN
      glEnable(GL_CULL_FACE);
      // glCullFace(GL_BACK);
#endif
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    GameState->R.RecomputeDirectionalShadows = false;

    if(GameState->R.ClearDirectionalShadows)
    {
      for(int i = 0; i < SHADOWMAP_CASCADE_COUNT; i++)
      {
        glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.ShadowMapFBOs[i]);
        glClear(GL_DEPTH_BUFFER_BIT);
      }
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      GameState->R.ClearDirectionalShadows = false;
    }
  }
}

// RENDER VOLUMETRIC LIGHT SCATTERING TO TEXTURE
void
RenderVolumeLightingToTexture(game_state* GameState)
{
  TIMED_BLOCK(VolumetricScatteringPass);
  glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.LightScatterFBOs[0]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  {
    GLuint ShaderVolumetricScatteringID =
      GameState->Resources.GetShader(GameState->R.ShaderVolumetricScattering);

    glUseProgram(ShaderVolumetricScatteringID);

    glUniformMatrix4fv(glGetUniformLocation(ShaderVolumetricScatteringID, "u_mat_sun_vp"), 1,
                       GL_FALSE,
                       GameState->R.Sun.CascadeVP[GameState->R.Sun.CurrentCascadeIndex].e);
    glUniformMatrix4fv(glGetUniformLocation(ShaderVolumetricScatteringID, "u_mat_inv_v"), 1,
                       GL_FALSE, Math::InvMat4(GameState->Camera.ViewMatrix).e);
    glUniform3fv(glGetUniformLocation(ShaderVolumetricScatteringID, "u_CameraPosition"), 1,
                 (float*)&GameState->Camera.Position);
    {
      int tex_index = 0;
      glActiveTexture(GL_TEXTURE0 + tex_index);
      glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferPositionTexID);
      glUniform1i(glGetUniformLocation(ShaderVolumetricScatteringID, "u_PositionMap"), tex_index);
    }
    {
      int tex_index = 1;
      glActiveTexture(GL_TEXTURE0 + tex_index);
      glBindTexture(GL_TEXTURE_2D,
                    GameState->R.ShadowMapTextures[GameState->R.Sun.CurrentCascadeIndex]);
      glUniform1i(glGetUniformLocation(ShaderVolumetricScatteringID, "u_ShadowMap"), tex_index);
#if 0
#endif
    }
    DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
  }
  glBindTexture(GL_TEXTURE_2D, 0);
}

// Draw Cubemap
// TODO (rytis): Finish cubemap loading
void
RenderCubemap(game_state* GameState)
{
  TIMED_BLOCK(Cubemap);

  if(GameState->R.Cubemap.CubemapTexture == -1)
  {
    GameState->R.Cubemap.CubemapTexture =
      LoadCubemap(&GameState->Resources, GameState->R.Cubemap.FaceIDs);
  }
  glDepthFunc(GL_LEQUAL);
  GLuint CubemapShaderID = GameState->Resources.GetShader(GameState->R.ShaderCubemap);
  glUseProgram(CubemapShaderID);
  glUniformMatrix4fv(glGetUniformLocation(CubemapShaderID, "mat_projection"), 1, GL_FALSE,
                     GameState->Camera.ProjectionMatrix.e);
  Render::mesh* CubemapMesh = GameState->Resources.GetModel(GameState->CubemapModelID)->Meshes[0];
  glUniformMatrix4fv(glGetUniformLocation(CubemapShaderID, "mat_view"), 1, GL_FALSE,
                     Math::Mat3ToMat4(Math::Mat4ToMat3(GameState->Camera.ViewMatrix)).e);
  glBindVertexArray(CubemapMesh->VAO);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, GameState->R.Cubemap.CubemapTexture);
  glDrawElements(GL_TRIANGLES, CubemapMesh->IndiceCount, GL_UNSIGNED_INT, 0);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  glBindVertexArray(0);
}

// Main scene object rendering loop
// TODO(Lukas) SORT(MeshInstances, ByBlend, ByMaterial, ByMesh);
void
RenderMainSceneObjects(game_state* GameState)
{
  TIMED_BLOCK(RenderScene);

  BEGIN_GPU_TIMED_BLOCK(RenderScene);
  material*     PreviousMaterial = nullptr;
  Render::mesh* PreviousMesh     = nullptr;
  uint32_t      CurrentShaderID  = 0;
  for(int i = 0; i < GameState->R.MeshInstanceCount; i++)
  {
    mesh_instance*                MeshInstance          = &GameState->R.MeshInstances[i];
    material*                     CurrentMaterial       = MeshInstance->Material;
    Render::mesh*                 CurrentMesh           = MeshInstance->Mesh;
    Anim::animation_controller*   CurrentAnimController = MeshInstance->AnimController;

    if(CurrentMaterial != PreviousMaterial)
    {
      if(PreviousMaterial)
      {
        glBindTexture(GL_TEXTURE_2D, 0);
      }
      CurrentShaderID = SetMaterial(GameState, &GameState->Camera, CurrentMaterial);

      PreviousMaterial = CurrentMaterial;
    }
    if(CurrentMesh != PreviousMesh)
    {
      glBindVertexArray(CurrentMesh->VAO);
      PreviousMesh = CurrentMesh;
    }
    if(CurrentMaterial->Common.IsSkeletal && CurrentAnimController)
    {
      glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "g_boneMatrices"),
                         CurrentAnimController->Skeleton->BoneCount, GL_FALSE,
                         (float*)CurrentAnimController->HierarchicalModelSpaceMatrices);
    }
    else
    {
      mat4 Mat4Zeros = {};
      glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "g_boneMatrices"), 1, GL_FALSE,
                         Mat4Zeros.e);
    }
    glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_mvp"), 1, GL_FALSE,
                       MeshInstance->MVP.e);
    glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_model"), 1, GL_FALSE,
                       Math::MulMat4(GameState->Camera.InvVPMatrix, MeshInstance->MVP).e);
    glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_view"), 1, GL_FALSE,
                       GameState->Camera.ViewMatrix.e);
    glDrawElements(GL_TRIANGLES, CurrentMesh->IndiceCount, GL_UNSIGNED_INT, 0);
  }
  glBindVertexArray(0);
  END_GPU_TIMED_BLOCK(RenderScene);
}

void
RenderObjectSelectionHighlighting(game_state* GameState, entity* SelectedEntity)
{
  TIMED_BLOCK(RenderSelection);
  if(SelectedEntity->AnimController && !GameState->DrawActorMeshes)
	{
    return;
	}

  // MESH SELECTION HIGHLIGHTING
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDepthFunc(GL_LEQUAL);
  vec4   ColorRed      = vec4{ 1, 1, 0, 1 };
  GLuint ColorShaderID = GameState->Resources.GetShader(GameState->R.ShaderColor);
  glUseProgram(ColorShaderID);
  glUniform4fv(glGetUniformLocation(ColorShaderID, "g_color"), 1, (float*)&ColorRed);
  glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "mat_mvp"), 1, GL_FALSE,
                     GetEntityMVPMatrix(GameState, GameState->SelectedEntityIndex).e);
  if(SelectedEntity->AnimController)
  {
    glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "g_boneMatrices"),
                       SelectedEntity->AnimController->Skeleton->BoneCount, GL_FALSE,
                       (float*)SelectedEntity->AnimController->HierarchicalModelSpaceMatrices);
  }
  else
  {
    mat4 Mat4Zeros = {};
    glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "g_boneMatrices"), 1, GL_FALSE,
                       Mat4Zeros.e);
  }
  if(GameState->SelectionMode == SELECT_Mesh)
  {
    Render::mesh* SelectedMesh = {};
    if(GetSelectedMesh(GameState, &SelectedMesh))
    {
      glBindVertexArray(SelectedMesh->VAO);

      glDrawElements(GL_TRIANGLES, SelectedMesh->IndiceCount, GL_UNSIGNED_INT, 0);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  }
  // MODEL SELECTION HIGHLIGHTING
  else if(GameState->SelectionMode == SELECT_Entity)
  {
    entity* SelectedEntity = {};
    if(GetSelectedEntity(GameState, &SelectedEntity))
    {
      Render::model* Model = GameState->Resources.GetModel(SelectedEntity->ModelID);
      for(int m = 0; m < Model->MeshCount; m++)
      {
        glBindVertexArray(Model->Meshes[m]->VAO);
        glDrawElements(GL_TRIANGLES, Model->Meshes[m]->IndiceCount, GL_UNSIGNED_INT, 0);
      }
    }
  }
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void
RenderMaterialPreviewToTexture(game_state* GameState)
{
  TIMED_BLOCK(RenderPreview);
  glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);

  material* PreviewMaterial = GameState->Resources.GetMaterial(GameState->CurrentMaterialID);
  glClearColor(0.7f, 0.7f, 0.7f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  uint32_t MaterialPreviewShaderID =
    SetMaterial(GameState, &GameState->PreviewCamera, PreviewMaterial);

  if(PreviewMaterial->Common.IsSkeletal)
  {
    mat4 Mat4Zeros = {};
    glUniformMatrix4fv(glGetUniformLocation(MaterialPreviewShaderID, "g_boneMatrices"), 1, GL_FALSE,
                       Mat4Zeros.e);
  }
  glEnable(GL_BLEND);
  mat4           PreviewSphereMatrix = Math::Mat4Ident();
  Render::model* UVSphereModel       = GameState->Resources.GetModel(GameState->UVSphereModelID);
  glBindVertexArray(UVSphereModel->Meshes[0]->VAO);
  glUniformMatrix4fv(glGetUniformLocation(MaterialPreviewShaderID, "mat_mvp"), 1, GL_FALSE,
                     Math::MulMat4(GameState->PreviewCamera.VPMatrix, Math::Mat4Ident()).e);
  glUniformMatrix4fv(glGetUniformLocation(MaterialPreviewShaderID, "mat_model"), 1, GL_FALSE,
                     PreviewSphereMatrix.e);
  glUniform3fv(glGetUniformLocation(MaterialPreviewShaderID, "lightPosition"), 1,
               (float*)&GameState->R.PreviewLightPosition);

  glDrawElements(GL_TRIANGLES, UVSphereModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
