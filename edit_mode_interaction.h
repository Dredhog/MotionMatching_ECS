#include "transform.h"

void
EditWorldAndInteractWithGUI(game_state* GameState, const game_input* Input)
{
  TIMED_BLOCK(Editor);
  UI::BeginFrame(GameState, Input);
  TestGui(GameState, Input);

  if(Input->MouseRight.EndedDown && Input->MouseRight.Changed)
  {
    TIMED_BLOCK(SelectionDrawing);
    // Draw entities to ID buffer
    // SORT_MESH_INSTANCES(ByEntity);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
    glClearColor(0.9f, 0.9f, 0.9f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLuint EntityIDShaderID = GameState->Resources.GetShader(GameState->R.ShaderID);
    glUseProgram(EntityIDShaderID);
    for(int e = 0; e < GameState->EntityCount; e++)
    {
      entity* CurrentEntity;
      assert(GetEntityAtIndex(GameState, &CurrentEntity, e));
      glUniformMatrix4fv(glGetUniformLocation(EntityIDShaderID, "mat_mvp"), 1, GL_FALSE,
                         GetEntityMVPMatrix(GameState, e).e);
      if(CurrentEntity->AnimController)
      {
        glUniformMatrix4fv(glGetUniformLocation(EntityIDShaderID, "g_boneMatrices"),
                           CurrentEntity->AnimController->Skeleton->BoneCount, GL_FALSE,
                           (float*)CurrentEntity->AnimController->HierarchicalModelSpaceMatrices);
      }
      else
      {
        mat4 Mat4Zeros = {};
        glUniformMatrix4fv(glGetUniformLocation(EntityIDShaderID, "g_boneMatrices"), 1, GL_FALSE,
                           Mat4Zeros.e);
      }
      Render::model* CurrentModel = GameState->Resources.GetModel(GameState->Entities[e].ModelID);
      for(int m = 0; m < CurrentModel->MeshCount; m++)
      {
        glBindVertexArray(CurrentModel->Meshes[m]->VAO);
        assert(e < USHRT_MAX);
        assert(m < USHRT_MAX);
        uint16_t EntityID = (uint16_t)e;
        uint16_t MeshID   = (uint16_t)m;
        uint32_t R        = (EntityID & 0x00FF) >> 0;
        uint32_t G        = (EntityID & 0xFF00) >> 8;
        uint32_t B        = (MeshID & 0x00FF) >> 0;
        uint32_t A        = (MeshID & 0xFF00) >> 8;

        vec4 EntityColorID = { (float)R / 255.0f, (float)G / 255.0f, (float)B / 255.0f,
                               (float)A / 255.0f };
        glUniform4fv(glGetUniformLocation(EntityIDShaderID, "g_id"), 1, (float*)&EntityColorID);
        glDrawElements(GL_TRIANGLES, CurrentModel->Meshes[m]->IndiceCount, GL_UNSIGNED_INT, 0);
      }
    }
    glFlush();
    glFinish();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    uint16_t IDColor[2] = {};
    glReadPixels(Input->MouseX, Input->MouseY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, IDColor);
    GameState->SelectedEntityIndex = (uint32_t)IDColor[0];
    GameState->SelectedMeshIndex   = (uint32_t)IDColor[1];
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  {
    entity* SelectedEntity = {};
    if(GetSelectedEntity(GameState, &SelectedEntity))
    {
      UI::MoveGizmo(&SelectedEntity->Transform, false);
      // Testing translation manipulator (second argument is used coordinate axes: world/local)
      // UI::MoveGizmo(&TestTransform.T);
      // UI::TranslationPlane(&TestTransform, 1, true, parametric_plane Optioal);
      // UI::RotationGizmo(&TestTransform, false);

      // Only translates in world space
      // UI::TranslationGizmo(&TestTransform.Translation);
      // UI::TranslationPlane(&TestTransform.Translation, 1);
    }
  }
	
  if(GameState->TrajectorySystem.SelectedSplineIndex >= 0 &&
     GameState->TrajectorySystem.SelectedWaypointIndex >= 0)
  {
    vec3* WaypointPositionPtr =
      &GameState->TrajectorySystem.Splines[GameState->TrajectorySystem.SelectedSplineIndex]
         .Waypoints[GameState->TrajectorySystem.SelectedWaypointIndex]
         .Position;
    UI::MoveGizmo(WaypointPositionPtr);
  }

  // Waypoint debug visualizaiton
  for(int i = 0; i < GameState->TrajectorySystem.Splines.Count; i++)
  {
    waypoint PreviousWaypoint = {};
    for(int j = 0; j < GameState->TrajectorySystem.Splines[i].Waypoints.Count; j++)
    {
      waypoint CurrentWaypoint = GameState->TrajectorySystem.Splines[i].Waypoints[j];
			if(j > 0)
			{
        Debug::PushLine(PreviousWaypoint.Position, CurrentWaypoint.Position);
      }
      vec4 WaypointColor = { 1, 0, 0, 1 };
      if(GameState->TrajectorySystem.SelectedSplineIndex == i && GameState->TrajectorySystem.SelectedWaypointIndex == j)
			{
        WaypointColor = { 0, 0, 1, 1 };
      }
      if(UI::SelectSphere(&CurrentWaypoint.Position, 0.1f, WaypointColor))
			{
        GameState->TrajectorySystem.SelectedSplineIndex   = i;
        GameState->TrajectorySystem.SelectedWaypointIndex = j;
      }
      PreviousWaypoint = CurrentWaypoint;
    }
  }


  // Entity creation
  if(GameState->IsEntityCreationMode && Input->MouseLeft.EndedDown && Input->MouseLeft.Changed &&
     GameState->CurrentModelID.Value != 0)
  {
    TIMED_BLOCK(EntityCreation);

    GameState->IsEntityCreationMode = false;
    vec3 RayDir =
      GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                           GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
    raycast_result RaycastResult =
      RayIntersectPlane(GameState->Camera.Position, RayDir, {}, { 0, 1, 0 });
    if(RaycastResult.Success)
    {
      transform NewTransform = {};
      NewTransform.R         = Math::QuatIdent();
      NewTransform.T         = RaycastResult.IntersectP;
      NewTransform.S         = { 1, 1, 1 };

      Render::model* Model       = GameState->Resources.GetModel(GameState->CurrentModelID);
      rid*           MaterialIDs = PushArray(GameState->PersistentMemStack, Model->MeshCount, rid);
      if(GameState->CurrentMaterialID.Value > 0)
      {
        for(int m = 0; m < Model->MeshCount; m++)
        {
          MaterialIDs[m] = GameState->CurrentMaterialID;
        }
      }
      AddEntity(GameState, GameState->CurrentModelID, MaterialIDs, NewTransform);
    }
  }
	// Waypoint creation
  else if(GameState->TrajectorySystem.IsWaypointPlacementMode && Input->MouseLeft.EndedDown &&
          Input->MouseLeft.Changed)
  {
    vec3 RayDir =
      GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                           GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
    raycast_result RaycastResult =
      RayIntersectPlane(GameState->Camera.Position, RayDir, {}, { 0, 1, 0 });
    if(RaycastResult.Success)
		{
      waypoint NewWaypoint = {
        .Position = RaycastResult.IntersectP,
        .Facing   = 0.0f,
        .Velocity = 1.0f,
      };
			assert(GameState->TrajectorySystem.SelectedSplineIndex != -1);
      GameState->TrajectorySystem.Splines[GameState->TrajectorySystem.SelectedSplineIndex]
        .Waypoints.Push(NewWaypoint);
      GameState->TrajectorySystem.IsWaypointPlacementMode = false;
    }
	}
  UI::EndFrame();
}

void
AnimationEditorInteraction(game_state* GameState, const game_input* Input)
{
  if(Input->Space.EndedDown && Input->Space.Changed)
  {
    GameState->IsAnimationPlaying = !GameState->IsAnimationPlaying;
  }
  if(GameState->IsAnimationPlaying)
  {
    EditAnimation::PlayAnimation(&GameState->AnimEditor, Input->dt);
  }
  if(Input->i.EndedDown && Input->i.Changed)
  {
    InsertBlendedKeyframeAtTime(&GameState->AnimEditor, GameState->AnimEditor.PlayHeadTime);
  }
  if(Input->LeftShift.EndedDown)
  {
    if(Input->n.EndedDown && Input->n.Changed)
    {
      EditAnimation::EditPreviousBone(&GameState->AnimEditor);
    }
    if(Input->ArrowLeft.EndedDown && Input->ArrowLeft.Changed)
    {
      EditAnimation::JumpToPreviousKeyframe(&GameState->AnimEditor);
    }
    if(Input->ArrowRight.EndedDown && Input->ArrowRight.Changed)
    {
      EditAnimation::JumpToNextKeyframe(&GameState->AnimEditor);
    }
  }
  else
  {
    if(Input->n.EndedDown && Input->n.Changed)
    {
      EditAnimation::EditNextBone(&GameState->AnimEditor);
    }
    if(Input->ArrowLeft.EndedDown)
    {
      EditAnimation::AdvancePlayHead(&GameState->AnimEditor, -1 * Input->dt);
    }
    if(Input->ArrowRight.EndedDown)
    {
      EditAnimation::AdvancePlayHead(&GameState->AnimEditor, 1 * Input->dt);
    }
  }
  if(Input->LeftCtrl.EndedDown)
  {
    if(Input->c.EndedDown && Input->c.Changed)
    {
      EditAnimation::CopyKeyframeToClipboard(&GameState->AnimEditor,
                                             GameState->AnimEditor.CurrentKeyframe);
    }
    else if(Input->x.EndedDown && Input->x.Changed)
    {
      EditAnimation::CopyKeyframeToClipboard(&GameState->AnimEditor,
                                             GameState->AnimEditor.CurrentKeyframe);
      DeleteCurrentKeyframe(&GameState->AnimEditor);
    }
    else if(Input->v.EndedDown && Input->v.Changed && GameState->AnimEditor.Skeleton)
    {
      EditAnimation::InsertKeyframeFromClipboardAtTime(&GameState->AnimEditor,
                                                       GameState->AnimEditor.PlayHeadTime);
    }
  }
  if(Input->Delete.EndedDown && Input->Delete.Changed)
  {
    EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
  }
  if(GameState->AnimEditor.KeyframeCount > 0)
  {
    EditAnimation::CalculateHierarchicalmatricesAtTime(&GameState->AnimEditor);
  }

  float CurrentlySelectedDistance = INFINITY;
  // Bone Selection
  for(int i = 0; i < GameState->AnimEditor.Skeleton->BoneCount; i++)
  {
    mat4 Mat4Bone =
      Math::MulMat4(TransformToMat4(*GameState->AnimEditor.Transform),
                    Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices[i],
                                  GameState->AnimEditor.Skeleton->Bones[i].BindPose));

    const float BoneSphereRadius = 0.03f;

    vec3 Position = Math::GetMat4Translation(Mat4Bone);
    vec3 RayDir =
      GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                           GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
    raycast_result RaycastResult =
      RayIntersectSphere(GameState->Camera.Position, RayDir, Position, BoneSphereRadius);
    if(RaycastResult.Success)
    {
      Debug::PushWireframeSphere(Position, BoneSphereRadius, { 1, 1, 0, 1 });
      float DistanceToIntersection =
        Math::Length(RaycastResult.IntersectP - GameState->Camera.Position);

      if(Input->MouseRight.EndedDown && Input->MouseRight.Changed &&
         DistanceToIntersection < CurrentlySelectedDistance)
      {
        EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, i);
        CurrentlySelectedDistance = DistanceToIntersection;
      }
    }
    else
    {
      Debug::PushWireframeSphere(Position, BoneSphereRadius);
    }
  }
  DrawSkeleton(GameState->AnimEditor.Skeleton, GameState->AnimEditor.HierarchicalModelSpaceMatrices,
               TransformToMat4(*GameState->AnimEditor.Transform), GameState->BoneSphereRadius,
               false);
  if(GameState->AnimEditor.Skeleton)
  {
    mat4 Mat4Bone = Math::MulMat4(TransformToMat4(*GameState->AnimEditor.Transform),
                                  Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices
                                                  [GameState->AnimEditor.CurrentBone],
                                                GameState->AnimEditor.Skeleton
                                                  ->Bones[GameState->AnimEditor.CurrentBone]
                                                  .BindPose));
    Debug::PushGizmo(&GameState->Camera, Mat4Bone);
  }
  // Copy editor poses to entity anim controller
  assert(0 <= GameState->AnimEditor.EntityIndex &&
         GameState->AnimEditor.EntityIndex < GameState->EntityCount);
  {
    memcpy(GameState->Entities[GameState->AnimEditor.EntityIndex]
             .AnimController->HierarchicalModelSpaceMatrices,
           GameState->AnimEditor.HierarchicalModelSpaceMatrices,
           sizeof(mat4) * GameState->AnimEditor.Skeleton->BoneCount);
  }
}
