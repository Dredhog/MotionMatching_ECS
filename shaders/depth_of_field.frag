#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform float g_FocusDepth;

uniform sampler2D g_ColorMap;
uniform sampler2D g_BlurrMap;
uniform sampler2D g_DepthMap;

void
main()
{
  vec3 color  = vec3(texture(g_ColorMap, TexCoord));
  vec3 blurr  = vec3(texture(g_BlurrMap, TexCoord));
  float depth = texture(g_DepthMap, TexCoord).x;

  const float clear_interval = 0.;
  const float focus_depth    = 0.3;
  const float fade_distance = 2;

  float distance_from_focus = abs(focus_depth - depth);
  float blurr_amount = 1;
  if(distance_from_focus < 0.5*clear_interval)
  {
    blurr_amount = 0;
  }

  //FragColor = vec4(mix(color, blurr, blurr_amount), 1.0);
  FragColor = vec4(blurr, 1.0);
}
