#version 330 core

out vec4 o_FragColor;

in vec2 TexCoord;

const float MAX_BLUR_SAMPLES = 10;

//uniform float u_VelocityScale;
uniform sampler2D u_InputMap;
uniform sampler2D u_VelocityMap;

void
main()
{
  vec2 texel_size = 1.0f / vec2(textureSize(u_InputMap, 0));
  vec2 velocity      = vec2(texture(u_VelocityMap, TexCoord));

  float u_VelocityScale = 3.0f; //Set locally for now
  
  velocity *= u_VelocityScale;

  float speed = length(velocity / texel_size);
  int sample_count = int(clamp(int(speed), 1, MAX_BLUR_SAMPLES));

  o_FragColor = texture(u_InputMap, TexCoord);
  for(int i = 1; i < sample_count; i++)
  {
    vec2 offset_from_current = velocity * ((float(i) / float(sample_count - 1)) - 0.5f);
    o_FragColor += texture(u_InputMap, TexCoord + offset_from_current);
  }

  o_FragColor /= sample_count;
  o_FragColor.a = 1.0f;
}
