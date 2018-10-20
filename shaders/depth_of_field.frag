#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_BlurredMap;
uniform sampler2D u_InputMap;
uniform sampler2D u_PositionMap;

void
main()
{
  vec3 regular_color = vec3(texture(u_InputMap, TexCoord));
  vec3 blurred_color = vec3(texture(u_BlurredMap, TexCoord));
  vec3 position      = texture(u_PositionMap, TexCoord).xyz;
  float depth = position.z;

  const float clear_interval = 5;
  const float focus_depth    = 8;
  const float fade_distance  = 2;

  FragColor = vec4(position, 1.0);
}
