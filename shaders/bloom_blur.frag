#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

#define KERNEL_SIZE 11
const float Kernel[KERNEL_SIZE] = float[]( 0.1, 0.15, 0.3, 0.5, 0.75, 0.9, 0.75, 0.5, 0.3, 0.15, 0.1);

uniform bool u_Horizontal;

uniform sampler2D ScreenTex;

void
main()
{
  vec2 tex_delta = 1.0f / vec2(textureSize(ScreenTex, 0));

  if(u_Horizontal)
  {
    tex_delta.y = 0;
  }
  else
  {
    tex_delta.x = 0;
  }

  vec3 final_color = vec3(0);
  float kernel_sum = 0.0f;
  for(int i = -5; i <= 5; ++i)
  {
    final_color += Kernel[i+5] * texture(ScreenTex, TexCoords + float(i) * tex_delta).rgb;
    kernel_sum += Kernel[i+5];
  }
  vec3 sample_color = texture(ScreenTex, TexCoords.st).rgb;
   
  FragColor = vec4(final_color/kernel_sum, 1);
}
