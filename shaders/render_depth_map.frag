#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D DepthMap;

void main()
{
    float DepthValue = texture(DepthMap, TexCoords).r;
    FragColor = vec4(vec3(DepthValue), 1.0);
}
