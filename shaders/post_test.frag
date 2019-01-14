#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float u_Time;
uniform sampler2D   u_ScreenTex;
uniform sampler2D   u_AuxiliaryTex;
uniform samplerCube u_CubeTex;

void main()
{
    vec3 input_color  = texture(u_ScreenTex, TexCoords).rgb;
    FragColor = vec4(input_color, 1);
}	
