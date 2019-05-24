#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in ivec4 a_boneIndices;
layout(location = 5) in vec4 a_boneWeights;

uniform mat4 mat_mvp;
uniform mat4 mat_model;
uniform mat4 g_boneMatrices[20];
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform int  flags;

void
main()
{
  mat4 modelMatrix = mat_model;
  mat4 mvpMatrix   = mat_mvp;

  gl_Position = mvpMatrix * vec4(a_position, 1.0f);
}
