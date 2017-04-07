#pragma once

#include <GL/glew.h>

#include "game.h"

void DEBUGDrawGizmos(game_state* GameState, mat4* GizmoBases, int32_t GizmoCount,
                     bool DepthEnabled = false);
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
