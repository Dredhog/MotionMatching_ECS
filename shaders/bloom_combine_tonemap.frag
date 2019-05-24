#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_ScreenTex;
uniform sampler2D u_BloomTex;
uniform sampler2D u_ScatteredLightMap;

uniform bool u_ApplyBloom;
uniform bool u_ApplyVolumetricScattering;

uniform bool  u_ApplyTonemapping;
uniform float u_Exposure;

void
main()
{
  vec3 screen_color = texture(u_ScreenTex, TexCoords).rgb;
  vec3 bloom_color  = (u_ApplyBloom) ? texture(u_BloomTex, TexCoords).rgb : vec3(0);
  vec3 scattered_light = (u_ApplyVolumetricScattering) ? texture(u_ScatteredLightMap, TexCoords).rgb : vec3(0);

  vec3 final_hdr_color = scattered_light/2 + bloom_color/3 + screen_color;
	vec3 tone_mapped_color = (u_ApplyTonemapping) ? (vec3(1.0) - exp(-final_hdr_color * u_Exposure)) : final_hdr_color;

  //vec3 tone_mapped_color = final_hdr_color / (final_hdr_color + vec3(1.0f));

  FragColor = vec4(tone_mapped_color, 1);
}
