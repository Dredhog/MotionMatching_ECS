#pragma once

#include <GL/glew.h>

#include "game.h"

enum quad_type
{
  QuadType_Colored  = 1 << 0,
  QuadType_Textured = 1 << 1,
  QuadType_Clip     = 1 << 2,
};

struct quad_instance
{
  vec4    Color;
  vec3    LowerLeft;
  int32_t Type;
  vec3    Dimensions;
  int32_t TextureID;
};

namespace Debug
{
  void PushTopLeftQuad(vec3 Position, float Width, float Height, vec4 Color);
  void PushTopLeftTexturedQuad(int32_t TextureID, vec3 Position, float Width, float Height);
  void PushGizmo(const camera* Camera, mat4 GizmoBase, vec3 Scale = { 1, 1, 1 });
  void PushShadedBone(mat4 GlobalBonePose, float Length);
  void PushWireframeSphere(vec3 Position, float Radius, vec4 Color = vec4{ 1, 0, 0, 1 });
  void PushLine(vec3 PointA, vec3 PointB, vec4 Color = { 1, 0, 0, 1 }, bool Overlay = true);
  void PushLineStrip(vec3* Points, int32_t PointCount, vec4 Color = { 1, 0, 0, 1 }, bool Overlay = true);

  // These are in y down and pixel space coordinates
  void UIPushQuad(vec3 Position, vec3 Size, vec4 Color = { 0.5f, 0.5f, 0.5f, 1.0f });
  void UIPushTexturedQuad(int32_t TextureID, vec3 BottomLeft, vec3 Size);
  void UIPushClipQuad(vec3 Position, vec3 Size, int32_t StencilValue);

  void SubmitShadedBoneMeshInstances(game_state* GameState, material Material);

  void DrawGizmos(game_state* GameState);
  void DrawQuads(game_state* GameState);
  void DrawTexturedQuads(game_state* GameState);
  void DrawWireframeSpheres(game_state* GameState);
  void DrawDepthTestedLines(game_state* GameState);
  void DrawOverlayLines(game_state* GameState);
  void ClearDrawArrays();
}

inline mat4
TransformToGizmoMat4(const transform* Transform)
{
  mat4 Result = Math::MulMat4(Math::Mat4Translate(Transform->T), Math::Mat4Rotate(Transform->R));
  return Result;
}
