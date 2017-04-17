#version 330 core

in vec3 frag_position;
in vec3 frag_normal;
in vec2 frag_texCoord;

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

  float diff    = max(dot(unit_normal, light_dir), 0.0);
  vec3  diffuse = diff * light_color;

  vec3 view_dir    = normalize(camera_position - frag_position);
  vec3 reflect_dir = reflect(-light_dir, unit_normal);

  vec4 texture_sample = texture(Texture, frag_texCoord);

  float spec     = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
  vec3  specular = (specular_strength * texture_sample.a) * spec * light_color;

  out_color = vec4(texture_sample.xyz * (ambient + diffuse + specular), 1.0f);
}
