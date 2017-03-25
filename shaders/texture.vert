#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord;

uniform mat4 mat_mvp;

out vec3 fragColor;
out vec2 fragTexCoord;

void
main()
{
  gl_Position  = mat_mvp * vec4(position, 1.0f);
  fragColor    = color;
  fragTexCoord = texCoord;
}
