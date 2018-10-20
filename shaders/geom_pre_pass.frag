#version 330 core
layout (location = 0) out vec3 o_ViewPos;
layout (location = 1) out vec2 o_Velocity;

in vertex_out
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 tangentLightPos;
  vec3 tangentViewPos;
  vec3 tangentFragPos;
  vec4 position0;
  vec4 position1;
}
frag;

void
main()
{
  o_ViewPos = frag.position;
  //Computing frament position this and last frame in x right y up both rangin from 0 to 1
  vec2 current_screen_pos = (frag.position1.xy / frag.position1.w) * 0.5 + 0.5;
  vec2 prev_screen_pos = (frag.position0.xy / frag.position0.w) * 0.5 + 0.5;
  o_Velocity = (current_screen_pos - prev_screen_pos);
}
