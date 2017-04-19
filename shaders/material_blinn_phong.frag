#version 330 core

#define COLOR 1
#define TEXTURE 2
#define SPECULAR 4
#define NORMAL 8

struct Material
{
  vec3      color;
  sampler2D diffuseMap;
  sampler2D specularMap;
  sampler2D normalMap;

  float shininess;
};

struct Light
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

in VertexOut
{
  flat int flags;

  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 lightPos;
  vec3 cameraPos;
  vec3 tangentLightPos;
  vec3 tangentViewPos;
  vec3 tangentFragPos;
}
frag;

uniform vec3 ambient_strength;
uniform vec3 specular_strength;
uniform Material material;
uniform Light light;

out vec4 out_color;

void
main()
{
  vec3 ambient = ambient_strength * light.ambient;
  if(((frag.flags & TEXTURE) != 0) && (frag.flags >= SPECULAR))
  {
    ambient = vec3(texture(material.diffuseMap, frag.texCoord)) * light.ambient;
  }

  vec3 normal = vec3(0.0f);
  if((frag.flags & NORMAL) != 0)
  {
    normal = texture(material.normalMap, frag.texCoord).rgb;
    normal = normalize(normal * 2.0f - 1.0f);
  }
  else
  {
    normal = normalize(frag.normal);
  }

  vec3 light_dir = vec3(0.0f);
  if((frag.flags & NORMAL) != 0)
  {
    light_dir = normalize(frag.tangentLightPos - frag.tangentFragPos);
  }
  else
  {
    light_dir = normalize(frag.lightPos - frag.position);
  }

  float diff    = max(dot(normal, light_dir), 0.0);
  vec3  diffuse = vec3(0.0f);
  if((frag.flags & COLOR) != 0)
  {
    diffuse = material.color * diff * light.diffuse;
  }
  else if((frag.flags & TEXTURE) != 0)
  {
    diffuse = vec3(texture(material.diffuseMap, frag.texCoord)) * diff * light.diffuse;
  }

  vec3 view_dir = vec3(0.0f);
  if((frag.flags & NORMAL) != 0)
  {
    view_dir = normalize(frag.tangentViewPos - frag.tangentFragPos);
  }
  else
  {
    view_dir = normalize(frag.cameraPos - frag.position);
  }

  vec3 halfway_dir = normalize(light_dir + view_dir);

  float spec     = pow(max(dot(normal, halfway_dir), 0.0), material.shininess);
  vec3  specular = vec3(0.0f);
  if((frag.flags & SPECULAR) != 0)
  {
    specular = vec3(texture(material.specularMap, frag.texCoord)) * spec * light.specular;
  }
  else
  {
    specular = specular_strength * spec * light.specular;
  }

  vec3 result = vec3(0.0f);
  if((frag.flags & COLOR) != 0)
  {
    result = (ambient + diffuse + specular) * material.color;
  }
  else if((frag.flags & TEXTURE) != 0)
  {
    result = (ambient + diffuse + specular) * vec3(texture(material.diffuseMap, frag.texCoord));
  }

  out_color = vec4(result, 1.0f);
}
