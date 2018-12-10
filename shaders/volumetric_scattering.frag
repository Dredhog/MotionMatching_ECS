#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform mat4 u_mat_sun_vp;

uniform sampler2D u_PositionMap;
uniform sampler2D u_ShadowMap;

void
main()
{
  FragColor = vec4(texture(u_PositionMap, TexCoords).rgb, 1);
  // FragColor = vec4(texture(u_ShadowMap, TexCoords).rgb, 1);
  // FragColor = vec4(vec3(u_mat_sun_vp.e[0], u_mat_sun_vp.e[1], u_mat_sun_vp.e[2]), 1);
}
