#version 330 core

#define DIFFUSE 1
#define SPECULAR 2
#define NORMAL 4

struct Material
{
  vec4 diffuseColor;

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

uniform float ambientStrength;
uniform float specularStrength;
uniform Material material;
uniform Light light;

out vec4 out_color;

void
main()
{
  vec3 ambient = vec3(0.0f);
  if((frag.flags & DIFFUSE) != 0)
  {
    ambient = ambientStrength * vec3(texture(material.diffuseMap, frag.texCoord)) * light.ambient;
  }
  else
  {
    ambient = ambientStrength * material.diffuseColor.rgb * light.ambient;
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

  vec3 lightDir = vec3(0.0f);
  if((frag.flags & NORMAL) != 0)
  {
    lightDir = normalize(frag.tangentLightPos - frag.tangentFragPos);
  }
  else
  {
    lightDir = normalize(frag.lightPos - frag.position);
  }

  float diff    = max(dot(normal, lightDir), 0.0f);
  vec3  diffuse = vec3(0.0f);
  if((frag.flags & DIFFUSE) != 0)
  {
    diffuse = vec3(texture(material.diffuseMap, frag.texCoord)) * diff * light.diffuse;
  }
  else
  {
    diffuse = material.diffuseColor.rgb * diff * light.diffuse;
  }

  vec3 viewDir = vec3(0.0f);
  if((frag.flags & NORMAL) != 0)
  {
    viewDir = normalize(frag.tangentViewPos - frag.tangentFragPos);
  }
  else
  {
    viewDir = normalize(frag.cameraPos - frag.position);
  }

  vec3 halfwayDir = normalize(lightDir + viewDir);

  float spec     = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
  vec3  specular = vec3(0.0f);
  if((frag.flags & SPECULAR) != 0)
  {
    specular = vec3(texture(material.specularMap, frag.texCoord)) * spec * light.specular;
  }
  else
  {
    specular = specularStrength * spec * light.specular;
  }

  vec4 result = vec4(0.0f);
  if((frag.flags & DIFFUSE) != 0)
  {
    result =
      vec4((ambient + diffuse + specular) * vec3(texture(material.diffuseMap, frag.texCoord)),
           1.0f);
  }
  else
  {
    result =
      vec4((ambient + diffuse + specular) * material.diffuseColor.rgb, material.diffuseColor.a);
  }

  out_color = vec4(result);
}
