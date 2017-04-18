#pragma once

#include <GL/glew.h>

#include "game.h"
void DEBUGPushGizmo(const camera* Camera, const mat4* GizmoBase, bool Overlay = true);
void DEBUGPushWireframeSphere(const camera* Camera, vec3 Position, float Radius,
                              bool Overlay = false, vec4 Color = vec4{ 1, 0, 0, 1 });

void DEBUGDrawGizmos(game_state* GameState);
void DEBUGDrawWireframeSpheres(game_state* GameState);

void DEBUGDrawQuad(game_state* GameState, vec3 LowerLeft, float Width, float Height,
                   vec4 Color = { 1, 1, 1, 1 }, bool DepthEnabled = false);
void DEBUGDrawTopLeftQuad(game_state* GameState, vec3 LowerLeft, float Width, float Height,
                          vec4 Color = { 0, 0, 0, 1 }, bool DepthEnabled = false);
void DEBUGDrawCenteredQuad(game_state* GameState, vec3 Center, float Width, float Height,
                           vec4 Color = { 1, 1, 1, 1 }, bool DepthEnabled = false);
void DEBUGDrawTopLeftTexturedQuad(game_state* GameState, int32_t TextureID, vec3 LowerLeft,
                                  float Width, float Height, bool DepthEnabled = false);
void DEBUGDrawTexturedQuad(game_state* GameState, int32_t TextureID, vec3 LowerLeft, float Width,
                           float Height, bool DepthEnabled = false);
void DEBUGDrawUnflippedTexturedQuad(game_state* GameState, int32_t TextureID, vec3 LowerLeft,
                                    float Width, float Height, bool DepthEnabled = false);

inline mat4
TransformToGizmoMat4(const Anim::transform* Transform)
{
  mat4 Result = Math::MulMat4(Math::Mat4Translate(Transform->Translation),
                              Math::Mat4Rotate(Transform->Rotation));
	return Result;
}
