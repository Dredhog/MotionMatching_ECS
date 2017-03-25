#version 330 core

in vec3 fragColor;
in vec2 fragTexCoord;

out vec4 color;

uniform sampler2D Texture;

void
main()
{
  color = texture(Texture, fragTexCoord);
}
