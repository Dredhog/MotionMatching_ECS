#version 330 core

struct Material
{
  sampler2D diffuse;
  sampler2D specular;
  float     shininess;
};

struct Light
{
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

in vec3 frag_position;
in vec3 frag_normal;
in vec2 frag_texCoord;

uniform vec3 camera_position;
uniform Material material;
uniform Light light;

out vec4 out_color;

void
main()
{
  vec3 ambient = vec3(texture(material.diffuse, frag_texCoord)) * light.ambient;

  vec3 unit_normal = normalize(frag_normal);
  vec3 light_dir   = normalize(light.position - frag_position);

  float diff    = max(dot(unit_normal, light_dir), 0.0);
  vec3  diffuse = vec3(texture(material.diffuse, frag_texCoord)) * diff * light.diffuse;

  vec3 view_dir    = normalize(camera_position - frag_position);
  vec3 halfway_dir = normalize(light_dir + view_dir);

  float spec     = pow(max(dot(frag_normal, halfway_dir), 0.0), material.shininess);
  vec3  specular = vec3(texture(material.specular, frag_texCoord)) * spec * light.specular;

  out_color = vec4((ambient + diffuse + specular), 1.0f);
}
