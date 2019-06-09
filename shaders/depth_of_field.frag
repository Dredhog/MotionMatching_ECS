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

  const float clear_interval = 5;
  const float fade_distance  = 2;

  float focus_depth = texture(u_PositionMap, vec2(0.5f, 0.5f)).z;

  vec3 temp_color = regular_color;

  float blur_delta = abs(position.z - focus_depth);
  float blur_amount = 1;
  if(blur_delta < 0.5*clear_interval)
  {
    temp_color = vec3(1, 0, 0);
    blur_amount = 0;
  }
  else if(blur_delta < 0.5*clear_interval + fade_distance)
  {
    blur_amount = 1-(0.5*clear_interval+fade_distance - blur_delta)/fade_distance;
  }
  vec3 output_color = mix(regular_color, blurred_color, blur_amount);

  FragColor = vec4(output_color, 1.0);
}
