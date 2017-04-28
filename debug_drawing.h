#pragma once

#include <GL/glew.h>

#include "game.h"

namespace Debug
{
  void PushQuad(vec3 Position, float Width, float Height, vec4 Color = { 0.5f, 0.5f, 0.5f, 1.0f });
  void PushTexturedQuad(int32_t TextureID, vec3 BottomLeft, float Width, float Height);
  void PushTopLeftQuad(vec3 Position, float Width, float Height, vec4 Color);
  void PushTopLeftTexturedQuad(int32_t TextureID, vec3 Position, float Width, float Height);
  void PushGizmo(const camera* Camera, const mat4* GizmoBase);
  void PushWireframeSphere(const camera* Camera, vec3 Position, float Radius,
                           vec4 Color = vec4{ 1, 0, 0, 1 });

  void DrawGizmos(game_state* GameState);
  void DrawColoredQuads(game_state* GameState);
  void DrawTexturedQuads(game_state* GameState);
  void DrawWireframeSpheres(game_state* GameState);
  void ClearDrawArrays();
}

inline mat4
TransformToGizmoMat4(const Anim::transform* Transform)
{
  mat4 Result = Math::MulMat4(Math::Mat4Translate(Transform->Translation),
                              Math::Mat4Rotate(Transform->Rotation));
  return Result;
}
