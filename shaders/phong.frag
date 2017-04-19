#version 330 core

in vec3 frag_position;
in vec3 frag_normal;
in vec2 frag_texCoord;

uniform bool  blinn;
uniform float ambient_strength;
uniform vec3 light_position;
uniform vec3  light_color;
uniform float specular_strength;
uniform vec3 camera_position;

out vec4 out_color;

uniform sampler2D Texture;

void
main()
{
  vec3 ambient = ambient_strength * light_color;

  vec3 unit_normal = normalize(frag_normal);
  vec3 light_dir   = normalize(light_position - frag_position);

  float diff    = max(dot(unit_normal, light_dir), 0.0f);
  vec3  diffuse = diff * light_color;

  vec3 view_dir = normalize(camera_position - frag_position);

  vec4 texture_sample = texture(Texture, frag_texCoord);

  float spec = 0.0f;

  if(blinn)
  {
    vec3 halfway_dir = normalize(light_dir + view_dir);
    spec             = pow(max(dot(frag_normal, halfway_dir), 0.0f), 32.0f);
  }
  else
  {
    vec3 reflect_dir = reflect(-light_dir, unit_normal);
    spec             = pow(max(dot(view_dir, reflect_dir), 0.0f), 32.0f);
  }

  vec3 specular = (specular_strength * texture_sample.a) * spec * light_color;

  out_color = vec4(texture_sample.xyz * (ambient + diffuse + specular), 1.0f);
}
