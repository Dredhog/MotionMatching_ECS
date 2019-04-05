#include "debug_drawing.h"
#include "basic_data_structures.h"

#define SPHERE_MAX_COUNT 500
#define GIZMO_MAX_COUNT 300
#define TEXTURED_QUAD_MAX_COUNT 350
#define COLORED_QUAD_MAX_COUNT 500
#define LINE_INSTRUCTION_COUNT 500
#define LINE_POINT_COUNT 1000
#define SHADED_BONE_MAX_COUNT 200

mat4    g_SphereMatrices[SPHERE_MAX_COUNT];
vec4    g_SphereColors[SPHERE_MAX_COUNT];
int32_t g_SphereCount;

mat4    g_GizmoMatrices[GIZMO_MAX_COUNT];
vec3    g_GizmoScales[GIZMO_MAX_COUNT];
float   g_GizmoDepths[GIZMO_MAX_COUNT];
int32_t g_GizmoCount;

quad_instance g_DrawQuads[TEXTURED_QUAD_MAX_COUNT];
int32_t       g_DrawQuadCount;

struct line_instruction
{
  vec4    Color;
  int32_t StartIndex;
  int32_t PointCount;
};

fixed_array<line_instruction, LINE_INSTRUCTION_COUNT> g_LineInstructions;
fixed_array<vec3, LINE_POINT_COUNT>                   g_LinePoints;

fixed_array<mat4, SHADED_BONE_MAX_COUNT> g_ShadedBoneMatrices;

void
Debug::PushShadedBone(mat4 GlobalBonePose, float Length)
{
  g_ShadedBoneMatrices.Append(Math::MulMat4(GlobalBonePose, Math::Mat4Scale(Length)));
}

void
Debug::SubmitShadedBoneMeshInstances(game_state* GameState, material Material)
{
	static material s_Material = Material;

	Render::mesh* BoneDiamondMesh= GameState->Resources.GetModel(GameState->BoneDiamondModelID)->Meshes[0];
  for(int i = 0; i < g_ShadedBoneMatrices.Count; i++)
  {
    mesh_instance BoneMeshInstance = {};
    BoneMeshInstance.Material     = &s_Material;
    BoneMeshInstance.Mesh         = BoneDiamondMesh;
    BoneMeshInstance.MVP          = Math::MulMat4(GameState->Camera.VPMatrix, g_ShadedBoneMatrices[i]);
    BoneMeshInstance.PrevMVP      = BoneMeshInstance.MVP;
    AddMeshInstance(&GameState->R, BoneMeshInstance);
  }
}

void
Debug::PushWireframeSphere(vec3 Position, float Radius, vec4 Color)
{
  mat4 ModelMatrix = Math::MulMat4(Math::Mat4Translate(Position), Math::Mat4Scale(Radius));
  assert(0 <= g_SphereCount && g_SphereCount < SPHERE_MAX_COUNT);
  g_SphereColors[g_SphereCount]     = Color;
  g_SphereMatrices[g_SphereCount++] = ModelMatrix;
}

void
Debug::PushGizmo(const camera* Camera, const mat4* GizmoBase, vec3 Scale)
{
  assert(0 <= g_GizmoCount && g_GizmoCount < GIZMO_MAX_COUNT);
  mat4  MVMatrix   = Math::MulMat4(Camera->ViewMatrix, *GizmoBase);
  mat4  MVPMatrix  = Math::MulMat4(Camera->ProjectionMatrix, MVMatrix);
  float GizmoDepth = Math::GetTranslationVec3(MVMatrix).Z;

  g_GizmoMatrices[g_GizmoCount] = MVPMatrix;
  g_GizmoScales[g_GizmoCount]   = Scale;
  g_GizmoDepths[g_GizmoCount]   = GizmoDepth;
  ++g_GizmoCount;
}

void
Debug::PushTopLeftQuad(vec3 TopLeft, float Width, float Height, vec4 Color)
{
  assert(0 <= g_DrawQuadCount && g_DrawQuadCount < COLORED_QUAD_MAX_COUNT);
  g_DrawQuads[g_DrawQuadCount]            = {};
  g_DrawQuads[g_DrawQuadCount].Type       = QuadType_Colored;
  g_DrawQuads[g_DrawQuadCount].Dimensions = { Width, Height };
  g_DrawQuads[g_DrawQuadCount].LowerLeft  = TopLeft - vec3{ 0, Height };
  g_DrawQuads[g_DrawQuadCount].Color      = Color;
  ++g_DrawQuadCount;
}

void
Debug::PushTopLeftTexturedQuad(int32_t TextureID, vec3 TopLeft, float Width, float Height)
{
  assert(0 <= g_DrawQuadCount && g_DrawQuadCount < TEXTURED_QUAD_MAX_COUNT);
  g_DrawQuads[g_DrawQuadCount]            = {};
  g_DrawQuads[g_DrawQuadCount].Type       = QuadType_Textured;
  g_DrawQuads[g_DrawQuadCount].Dimensions = { Width, Height };
  g_DrawQuads[g_DrawQuadCount].LowerLeft  = TopLeft - vec3{ 0, Height };
  g_DrawQuads[g_DrawQuadCount].TextureID  = TextureID;
  ++g_DrawQuadCount;
}

void
Debug::UIPushQuad(vec3 Position, vec3 Size, vec4 Color)
{
  vec3 ScreenSize = { SCREEN_WIDTH, SCREEN_HEIGHT };
  Debug::PushTopLeftQuad({ Position.X / ScreenSize.X, 1.0f - Position.Y / ScreenSize.Y },
                         Size.X / ScreenSize.X, Size.Y / ScreenSize.Y, Color);
}
void
Debug::UIPushTexturedQuad(int32_t TextureID, vec3 Position, vec3 Size)
{
  vec3 ScreenSize = { SCREEN_WIDTH, SCREEN_HEIGHT };
  Debug::PushTopLeftTexturedQuad(TextureID,
                                 { Position.X / ScreenSize.X, 1.0f - Position.Y / ScreenSize.Y },
                                 Size.X / ScreenSize.X, Size.Y / ScreenSize.Y);
}

void
Debug::UIPushClipQuad(vec3 Position, vec3 Size, int32_t StencilValue)
{

  Debug::UIPushQuad(Position, Size, { 1, 0, 1, (float)StencilValue });
  g_DrawQuads[g_DrawQuadCount - 1].Type = QuadType_Clip;
}

void
Debug::DrawGizmos(game_state* GameState)
{
  Render::model* GizmoModel    = GameState->Resources.GetModel(GameState->GizmoModelID);
  GLint          GizmoShaderID = GameState->Resources.GetShader(GameState->R.ShaderGizmo);
  glUseProgram(GizmoShaderID);
  for(int g = 0; g < g_GizmoCount; g++)
  {
    glUniformMatrix4fv(glGetUniformLocation(GizmoShaderID, "mat_mvp"), 1, GL_FALSE,
                       g_GizmoMatrices[g].e);
    glUniform3fv(glGetUniformLocation(GizmoShaderID, "scale"), 1, (float*)&g_GizmoScales[g]);
    glUniform1f(glGetUniformLocation(GizmoShaderID, "depth"), g_GizmoDepths[g]);
    for(int i = 0; i < GizmoModel->MeshCount; i++)
    {
      glBindVertexArray(GizmoModel->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GizmoModel->Meshes[i]->IndiceCount, GL_UNSIGNED_INT, 0);
    }
  }
  glBindVertexArray(0);
  g_GizmoCount = 0;
}

void
Debug::DrawWireframeSpheres(game_state* GameState)
{
  GLint ColorShaderID = GameState->Resources.GetShader(GameState->R.ShaderColor);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_DEPTH_TEST);
  glUseProgram(ColorShaderID);
  Render::model* SphereModel = GameState->Resources.GetModel(GameState->LowPolySphereModelID);
  glBindVertexArray(SphereModel->Meshes[0]->VAO);

  // So as not to corrupt the position by the old bone data
  {
    mat4 Mat4Zeros = {};
    glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "g_boneMatrices"), 1, GL_FALSE,
                       Mat4Zeros.e);
  }
  for(int i = 0; i < g_SphereCount; i++)
  {
    glUniform4fv(glGetUniformLocation(ColorShaderID, "g_color"), 1, (float*)&g_SphereColors[i]);
    g_SphereMatrices[i] = Math::MulMat4(GameState->Camera.VPMatrix, g_SphereMatrices[i]);
    glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "mat_mvp"), 1, GL_FALSE,
                       g_SphereMatrices[i].e);
    glDrawElements(GL_TRIANGLES, SphereModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT, 0);
  }
  glBindVertexArray(0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST);
  g_SphereCount = 0;
}

void
Debug::PushLine(vec3 PointA, vec3 PointB, vec4 Color)
{
  g_LinePoints.Append(PointA);
  g_LinePoints.Append(PointB);

  line_instruction Instruction = {};
  Instruction.Color            = Color;
  Instruction.StartIndex       = g_LinePoints.Count - 2;
  Instruction.PointCount       = 2;
  g_LineInstructions.Append(Instruction);
}

void
Debug::PushLineStrip(vec3* Points, int32_t PointCount, vec4 Color)
{
  line_instruction Instruction = {};
  Instruction.Color            = Color;
  Instruction.StartIndex       = g_LinePoints.Count;
  Instruction.PointCount       = PointCount;
  g_LineInstructions.Append(Instruction);

  for(int i = 0; i < PointCount; i++)
  {
    g_LinePoints.Append(Points[i]);
  }
}

void
Debug::DrawLines(game_state* GameState)
{
  static uint32_t s_VAO = 0;
  static uint32_t s_VBO = 0;

  // Init VAO and VBO
  if(s_VAO == 0)
  {
    glGenVertexArrays(1, &s_VAO);
    glBindVertexArray(s_VAO);

    glGenBuffers(1, &s_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, LINE_POINT_COUNT * sizeof(vec3), 0, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
  }
  assert(0 < s_VAO);
  assert(0 < s_VBO);

  glLineWidth(3);
  // Update line buffers
  glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, g_LinePoints.Count * sizeof(vec3), g_LinePoints.Elements);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  GLint ColorShaderID = GameState->Resources.GetShader(GameState->R.ShaderColor);
  glUseProgram(ColorShaderID);
  glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "mat_mvp"), 1, GL_FALSE,
                     GameState->Camera.VPMatrix.e);
  mat4 Mat4Zeros = {};
  glUniformMatrix4fv(glGetUniformLocation(ColorShaderID, "g_boneMatrices"), 1, GL_FALSE,
                     Mat4Zeros.e);

  // Draw lines
  glBindVertexArray(s_VAO);
  for(int i = 0; i < g_LineInstructions.Count; i++)
  {
    line_instruction Instruction = g_LineInstructions[i];
    glUniform4fv(glGetUniformLocation(ColorShaderID, "g_color"), 1, &Instruction.Color.X);
    glDrawArrays(GL_LINE_STRIP, Instruction.StartIndex, Instruction.PointCount);
  }
  glBindVertexArray(0);
}

void
Debug::DrawQuads(game_state* GameState)
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  Render::model* QuadModel = GameState->Resources.GetModel(GameState->QuadModelID);
  glBindVertexArray(QuadModel->Meshes[1]->VAO);

  GLint TexturedQuadShaderID = GameState->Resources.GetShader(GameState->R.ShaderTexturedQuad);
  GLint QuadShaderID         = GameState->Resources.GetShader(GameState->R.ShaderQuad);
  for(int i = 0; i < g_DrawQuadCount; i++)
  {
    const quad_instance& Quad = g_DrawQuads[i];
    const GLint          ShaderHandle =
      (Quad.Type == QuadType_Textured) ? TexturedQuadShaderID : QuadShaderID;

    if(Quad.Type == QuadType_Textured)
    {
      glBindTexture(GL_TEXTURE_2D, Quad.TextureID);
    }

    glUseProgram(ShaderHandle);
    glUniform3fv(glGetUniformLocation(ShaderHandle, "g_position"), 1, (float*)&Quad.LowerLeft);
    glUniform2fv(glGetUniformLocation(ShaderHandle, "g_dimension"), 1, (float*)&Quad.Dimensions);

    if(Quad.Type == QuadType_Colored)
    {
      glUniform4fv(glGetUniformLocation(ShaderHandle, "g_color"), 1, (float*)&Quad.Color);
    }

    if(Quad.Type == QuadType_Clip)
    {
      glStencilFunc(GL_NEVER, (int)Quad.Color.A, 0xFF);
      glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

      glDrawElements(GL_TRIANGLES, QuadModel->Meshes[1]->IndiceCount, GL_UNSIGNED_INT, 0);

      glStencilFunc(GL_EQUAL, (int)Quad.Color.A, 0xFF);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }
    else
    {
      glDrawElements(GL_TRIANGLES, QuadModel->Meshes[1]->IndiceCount, GL_UNSIGNED_INT, 0);
    }
  }

  glBindVertexArray(0);
  glDisable(GL_BLEND);
  glDisable(GL_STENCIL_TEST);
  g_DrawQuadCount = 0;
}

void
Debug::ClearDrawArrays()
{
  g_DrawQuadCount = 0;
  g_GizmoCount    = 0;
  g_SphereCount   = 0;
  g_LineInstructions.Clear();
  g_LinePoints.Clear();
  g_ShadedBoneMatrices.Clear();
}
