#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform mat4 u_mat_sun_vp;
uniform mat4 u_mat_inv_v;

uniform sampler2D u_PositionMap;
uniform sampler2D u_ShadowMap;
uniform vec3 u_CameraPosition;

#define ITERATION_COUNT 50

void
main()
{
  vec2 tex_coords = TexCoords;
  //tex_coords = vec2(TexCoords.x, 1-TexCoords.y); //DELETE LINE
  FragColor = vec4(vec3(texture(u_ShadowMap, tex_coords).r), 1);
  //FragColor = vec4(vec3(-texture(u_PositionMap, tex_coords).z), 1);

  //Start Point
  vec4 homog_player_light_space_coords = u_mat_sun_vp*vec4(u_CameraPosition, 1);
  vec2 player_light_screen_device_norm = homog_player_light_space_coords.xy /= homog_player_light_space_coords.w;
  vec2 player_coords_in_light_screen_space = (player_light_screen_device_norm+vec2(1))/2.0;

  //End Point
  //SWAP LINES
  vec3 frag_view = texture(u_PositionMap, tex_coords).xyz;//vec3(0, 0, -2);
  //vec3 frag_view = texture(u_PositionMap, vec2(0.5)).xyz;//vec3(0, 0, -2);

  vec3 frag_world = (u_mat_inv_v*vec4(frag_view, 1)).xyz;
  vec4 frag_ligh_view_homog = u_mat_sun_vp * vec4(frag_world, 1);
  vec2 frag_light_screen_device_norm =   frag_ligh_view_homog.xy /= frag_ligh_view_homog.w;
  vec2 frag_light_screen_space = (frag_light_screen_device_norm+vec2(1))/2.0;

  float light_accumulator = 0;
  for(int i = 0; i < ITERATION_COUNT; i++)
  {
    vec4 sample_homog_pos = mix(homog_player_light_space_coords, frag_ligh_view_homog,float(i)/ITERATION_COUNT);
    sample_homog_pos.xyz /= sample_homog_pos.w;
    vec3 sample_tex_coord = (sample_homog_pos.xyz+vec3(1))/2.0;

    /*if(distance(sample_tex_coord, tex_coords) < 0.02){
       FragColor = vec4(1, 0, 0, 1);
    }*/

    float sample_depth = texture(u_ShadowMap, sample_tex_coord.xy).r;
    if(sample_depth > sample_tex_coord.z){
      light_accumulator += 1;
    }
  }
  FragColor = vec4(vec3(light_accumulator/ITERATION_COUNT), 1);
}
