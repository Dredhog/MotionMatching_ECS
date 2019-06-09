#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ScreenTex;

uniform float u_BloomLuminanceThreshold;

float GetColorLuminance(vec3 color)
{
  return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void
main()
{
  vec3  color_sample = texture(ScreenTex, TexCoords).rgb;
  float luminance    = GetColorLuminance(color_sample);

  if(luminance > u_BloomLuminanceThreshold)
  {
    FragColor = vec4(color_sample, 1.0);
  }
  else
  {
    FragColor = vec4(vec3(0), 1.0);
  }
}
