#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_ScreenTex;
uniform sampler2D u_BloomTex;

uniform float u_ApplyBloom;


void
main()
{
  vec3 screen_color = texture(u_ScreenTex, TexCoords).rgb;
  vec3 bloom_color  = (u_ApplyBloom > 0.5f) ? texture(u_BloomTex, TexCoords).rgb : vec3(0);
  //FragColor = vec4((u_ApplyBloom < 0.5f) ? screen_color : bloom_color, 1);
  FragColor = vec4(bloom_color/3 + screen_color, 1);
}
