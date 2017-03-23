void
DEBUGDrawGizmo(game_state* GameState, mat4* GizmoBases, int32_t GizmoCount)
{
  glDisable(GL_DEPTH_TEST);

  glUseProgram(GameState->ShaderGizmo);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
  glEnable(GL_DEPTH_TEST);
}

static float
ClampFloat(float Min, float T, float Max)
{
  if(T < Min)
  {
    return Min;
  }
  if(T > Max)
  {
    return Max;
  }
  return T;
}

void
UpdateCamera(camera* Camera, const game_input* Input)
{
  Camera->Speed = 2.0f;
  if(Input->LeftCtrl.EndedDown)
  {
    Camera->Speed = 0.2f;
  }
  if(Input->LeftShift.EndedDown)
  {
    if(Input->w.EndedDown)
    {
      Camera->P += Input->dt * Camera->Speed * Camera->Up;
    }
    if(Input->s.EndedDown)
    {
      Camera->P -= Input->dt * Camera->Speed * Camera->Up;
    }
  }
  else
  {
    if(Input->w.EndedDown)
    {
      Camera->P += Input->dt * Camera->Speed * Camera->Forward;
    }
    if(Input->s.EndedDown)
    {
      Camera->P -= Input->dt * Camera->Speed * Camera->Forward;
    }
  }
  if(Input->a.EndedDown)
  {
    Camera->P -= Input->dt * Camera->Speed * Camera->Right;
  }
  if(Input->d.EndedDown)
  {
    Camera->P += Input->dt * Camera->Speed * Camera->Right;
  }
  Camera->Rotation.X -= 0.05f * (float)Input->dMouseY;
  Camera->Rotation.Y -= 0.05f * (float)Input->dMouseX;

  Camera->Rotation.X = ClampFloat(-Camera->MaxTiltAngle, Camera->Rotation.X, Camera->MaxTiltAngle);
  Camera->Forward =
    Math::MulMat3Vec3(Math::Mat4ToMat3(Math::Mat4Rotate(Camera->Rotation)), { 0, 0, -1 });
  Camera->Right   = Math::Cross(Camera->Forward, Camera->Up);
  Camera->Forward = Math::Normalized(Camera->Forward);
  Camera->Right   = Math::Normalized(Camera->Right);
  Camera->Up      = Math::Normalized(Camera->Up);

  Camera->ViewMatrix = Math::Mat4Camera(Camera->P, Camera->Forward, Camera->Up);
  Camera->ProjectionMatrix =
    Math::Mat4Perspective(Camera->FieldOfView, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
                          Camera->NearClipPlane, Camera->FarClipPlane);
  Camera->VPMatrix = Math::MulMat4(Camera->ProjectionMatrix, Camera->ViewMatrix);
}
