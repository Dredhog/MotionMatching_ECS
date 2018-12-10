#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_ScreenTex;
uniform sampler2D u_BloomTex;

uniform float u_ApplyBloom;
uniform float u_Exposure;
//const float u_Exposure = 1;

void
main()
{
  vec3 screen_color = texture(u_ScreenTex, TexCoords).rgb;
  vec3 bloom_color  = (u_ApplyBloom > 0.5f) ? texture(u_BloomTex, TexCoords).rgb : vec3(0);

  vec3 final_hdr_color = bloom_color/3 + screen_color;
  	vec3 tone_mapped_color = vec3(1.0) - exp(-final_hdr_color * u_Exposure);
  //vec3 tone_mapped_color = final_hdr_color / (final_hdr_color + vec3(1.0f));
  FragColor = vec4(tone_mapped_color, 1);
}
