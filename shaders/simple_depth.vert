#version 330 core

layout(location = 0) in vec3 in_Position;

uniform mat4 mat_light_vp;
uniform mat4 mat_model;

void
main()
{
    gl_Position = mat_light_vp * mat_model * vec4(in_Position, 1.0);
}
