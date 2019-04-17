struct capsule
{
	vec3 A;
	vec3 B;
  float radius;
};

struct line
{
	vec3 P0;
	vec3 P1;
};

#define EPSILON 0.0001f
float
MinDistBetweenLines(float* sc, float* tc, line L1, line L2)
{
  vec3  u = L1.P1 - L1.P0;
  vec3  v = L2.P1 - L2.P0;
  vec3  w = L1.P0 - L2.P0;
  float a = Math::Dot(u, u); // always >= 0
  float b = Math::Dot(u, v);
  float c = Math::Dot(v, v); // always >= 0
  float d = Math::Dot(u, w);
  float e = Math::Dot(v, w);
  float D = a * c - b * b; // always >= 0

  // compute the line parameters of the two closest points
  if(D < EPSILON)
  { // the lines are almost parallel
    *sc = 0.0;
    *tc = (b > c ? d / b : e / c); // use the largest denominator
  }
  else
  {
    *sc = (b * e - c * d) / D;
    *tc = (a * e - b * d) / D;
  }

  // get the difference of the two closest points
  vec3 dP = w + (*sc * u) - (*tc * v); // =  L1(sc) - L2(tc)

  return Math::Length(dP); // return the closest distance
}

struct parametric_plane
{
  vec3 n;
  vec3 u;
  vec3 o;
};

bool
IntersectRayParametricPlane(float* u, vec3 o, vec3 dir, parametric_plane p,
                            float maxTestDist = 100000)
{
  float d = Math::Dot(p.n, p.o);
  float t = (d - Math::Dot(p.n, o)) / Math::Dot(p.n, dir);

  if(0 <= t && t <= maxTestDist)
	{
    vec3 planeSpaheHitP = (o + t * dir) - p.o;

    // Compute position in plane space
    *u = Math::Dot(p.u, planeSpaheHitP);
    return true;
  }

	return false;
}

float
ClosestPtSegmentSegment(float* outS, float* outT, vec3* outC1, vec3* outC2, vec3 p1, vec3 q1,
                        vec3 p2, vec3 q2)
{
  vec3  d1 = q1 - p1; // Direction vector of segment S1
  vec3  d2 = q2 - p2; // Direction vector of segment S2
  vec3  r  = p1 - p2;
  float a  = Math::Dot(d1, d1); // Squared length of segment S1, always nonnegative
  float e  = Math::Dot(d2, d2); // Squared length of segment S2, always nonnegative
  float f  = Math::Dot(d2, r);

  float s;
  float t;
  vec3  c1;
  vec3  c2;
  // Check if either or both segments degenerate into points
  if(a <= EPSILON && e <= EPSILON)
  {
    // Both segments degenerate into points
    s = t = 0.0f;
    c1    = p1;
    c2    = p2;
    return Math::Length(c1 - c2);
  }
  if(a <= EPSILON)
  {
    // First segment degenerates into a point
    s = 0.0f;
    t = f / e; // s = 0 => t = (b*s + f) / e = f / e
    t = ClampFloat(0.0f, t, 1.0f);
  }
  else
  {
    float c = Math::Dot(d1, r);
    if(e <= EPSILON)
    {
      // Second segment degenerates into a point
      t = 0.0f;
      s = ClampFloat(0.0f, -c / a, 1.0f); // t = 0 => s = (b*t - c) / a = -c / a
    }
    else
    {
      // The general nondegenerate case starts here
      float b     = Math::Dot(d1, d2);
      float denom = a * e - b * b; // Always nonnegative
      // If segments not parallel, compute closest point on L1 to L2 and
      // clamp to segment S1. Else pick arbitrary s (here 0)
      if(denom != 0.0f)
      {
        s = ClampFloat(0.0f, (b * f - c * e) / denom, 1.0f);
      }
      else
      {
        s = 0.0f;
      }
      // Compute point on L2 closest to S1(s) using
      // t = Dot((P1 + D1*s) - P2,D2) / Dot(D2,D2) = (b*s + f) / e
      t = (b * s + f) / e;
      // If t in [0,1] done. Else clamp t, recompute s for the new value
      // of t using s = Dot((P2 + D2*t) - P1,D1) / Dot(D1,D1)= (t*b - c) / a
      // and clamp s to [0, 1]
      if(t < 0.0f)
      {
        t = 0.0f;
        s = ClampFloat(0.0f, -c / a, 1.0f);
      }
      else if(t > 1.0f)
      {
        t = 1.0f;
        s = ClampFloat(0.0f, (b - c) / a, 1.0f);
      }
    }
  }
  c1 = p1 + d1 * s;
  c2 = p2 + d2 * t;

  *outS  = s;
  *outT  = t;
  *outC1 = c1;
  *outC2 = c2;
  return Math::Length(c1 - c2);
}

float
MinDistRaySegment(float* DistToClosest, vec3 RayOrig, vec3 RayDir, vec3 p0, vec3 p1)
{
  vec3 c0;
  vec3 c1;

  const float RayLength = 10000.0f;

  vec3  r0 = RayOrig;
  vec3  r1 = r0 + RayLength * RayDir;
	float s;
  float t;
  float Dist = ClosestPtSegmentSegment(&s, &t, &c0, &c1, r0, r1, p0, p1);
  *DistToClosest = Math::Length(c0 - r0);
  return Dist;
}

bool
TestRayAxis(float* outS, float* outT, vec3 RayOrig, vec3 RayDir, vec3 AxisOrig, vec3 AxisDir, float AxisPadding)
{
  assert(FLT_MIN < AxisPadding);
  RayDir  = Math::Normalized(RayDir);
  AxisDir = Math::Normalized(AxisDir);

  float s;
  float t;
  line  RayLine = { .P0 = RayOrig, .P1 = RayOrig + RayDir };
  line  AxisLine = { .P0 = AxisOrig, .P1 = AxisOrig + AxisDir };
  float MinDist  = MinDistBetweenLines(&s, &t, RayLine, AxisLine);
	if(outS)
	{
		*outS = s;
	}
	if(outT)
	{
    *outT = t;
  }
  return MinDist <= AxisPadding;
}

void
EditWorldAndInteractWithGUI(game_state* GameState, const game_input* Input)
{
  TIMED_BLOCK(Editor);
  // GUI
  TestGui(GameState, Input);

  /* // ANIMATION TIMELINE
  if(GameState->SelectionMode == SELECT_Bone && GameState->DrawTimeline &&
     GameState->AnimEditor.Skeleton)
  {
    VisualizeTimeline(GameState);
  }
  */

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
	

	//TESTING TRANSFORM MANIPULATOR
  {
    static vec3 TransformP            = {};

    float GizmoScale =
      GameState->R.GizmoScaleFactor * Math::Length(TransformP - GameState->Camera.Position);

    float       AxisRadius            = 0.08f * GizmoScale;

    static bool             ManipulatorIsActive = false;
    static parametric_plane AxisPlane           = {};
    static float            InitialU            = 0.0f;

    vec3 RayDir =
      GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                           GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
    vec3 RayOrig = GameState->Camera.Position + RayDir * GameState->Camera.NearClipPlane;


    if(Input->MouseRight.EndedDown && Input->MouseRight.Changed)
    {
      const vec3 GlobalAxes[3]{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

      float DistXOrig;
      float DistYOrig;
      float DistZOrig;
      float HitDistX = MinDistRaySegment(&DistXOrig, RayOrig, RayDir, TransformP,
                                         TransformP + GizmoScale * GlobalAxes[0]);
      float HitDistY = MinDistRaySegment(&DistYOrig, RayOrig, RayDir, TransformP,
                                         TransformP + GizmoScale * GlobalAxes[1]);
      float HitDistZ = MinDistRaySegment(&DistZOrig, RayOrig, RayDir, TransformP,
                                         TransformP + GizmoScale * GlobalAxes[2]);

//#define CORRECTED_MATH
      if(MinFloat(HitDistX, MinFloat(HitDistY, HitDistZ)) < AxisRadius)
      {
        int   AxisIndex     = -1;
        float MinOrigToAxis = FLT_MAX;
        if(HitDistX < AxisRadius && DistXOrig < MinOrigToAxis)
        {
          AxisIndex     = 0;
          MinOrigToAxis = DistXOrig;
        }
        if(HitDistY < AxisRadius && DistYOrig < MinOrigToAxis)
        {
          AxisIndex     = 1;
          MinOrigToAxis = DistYOrig;
        }
        if(HitDistZ < AxisRadius && DistZOrig < MinOrigToAxis)
        {
          AxisIndex     = 2;
          MinOrigToAxis = DistZOrig;
        }

        assert(AxisIndex != -1);
        AxisPlane.u = GlobalAxes[AxisIndex];
        vec3 tempV  = Math::Cross(AxisPlane.u, RayDir);
        AxisPlane.n = Math::Normalized(Math::Cross(AxisPlane.u, tempV));
        AxisPlane.o = TransformP;

        // TestRayAxis(NULL, &InitialT, RayOrig, RayDir, TransformP, ActiveAxis, AxisRadius);
        if(IntersectRayParametricPlane(&InitialU, RayOrig, RayDir, AxisPlane))
        {
          ManipulatorIsActive = true;
#ifdef CORRECTED_MATH
          InitialU /= GizmoScale;
#endif
        }
      }
    }
    else if(Input->MouseRight.EndedDown && ManipulatorIsActive)
    {
      // TestRayAxis(NULL, &CurrentT, RayOrig, RayDir, InitialP, ActiveAxis, AxisRadius);
      float CurrentU;
      if(IntersectRayParametricPlane(&CurrentU, RayOrig, RayDir, AxisPlane))
      {
        vec3 ClosestPOnAxis = AxisPlane.o + CurrentU * AxisPlane.u;
        Debug::PushWireframeSphere(ClosestPOnAxis, AxisRadius);

#ifdef CORRECTED_MATH
        float t0 = InitialU;
        float t1 = CurrentU;
        float a  = Math::Dot(RayDir, RayDir);
        vec3  e  = AxisPlane.o - GameState->Camera.Position;
        float b  = Math::Dot(RayDir, e);
        float c  = Math::Dot(e, e);

        float o1 = (sqrtf(t0 * t0 * (-a * c * t0 * t0 + b * b * t0 * t0 + 2 * b * t1 + c)) +
                    b * t0 * t0 + t1) /
                   (1 - a * t0 * t0);
        TransformP = AxisPlane.o + o1 * AxisPlane.u;
#else
        TransformP = AxisPlane.o + (CurrentU - InitialU) * AxisPlane.u;
#endif
      }
      else
      {
        TransformP = AxisPlane.o;
      }
    }
    else if(!Input->MouseRight.EndedDown)
    {
      ManipulatorIsActive = false;
    }

    Debug::PushGizmo(&GameState->Camera, Math::Mat4Translate(TransformP));
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
      Anim::transform NewTransform = {};
      NewTransform.Rotation        = Math::QuatIdent();
      NewTransform.Translation     = RaycastResult.IntersectP;
      NewTransform.Scale           = { 1, 1, 1 };

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

    const float BoneSphereRadius = 0.1f;

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
