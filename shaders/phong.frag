#version 330 core

#define DIFFUSE_MAP 1
#define SPECULAR_MAP 2
#define NORMAL_MAP 4
#define SKELETAL 8

struct Material
{
  vec3 ambientColor;
  vec4 diffuseColor;
  vec3 specularColor;

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
  vec4 lightFragPos;
}
frag;

uniform Material material;
uniform Light light;

uniform sampler2D shadowMap;

out vec4 out_color;

float
ShadowCalculation(vec4 lightFragPos, vec3 normal, vec3 lightDir)
{
  vec3 projCoords = lightFragPos.xyz / lightFragPos.w;
  projCoords = projCoords * 0.5f + 0.5f;

  float closestDepth = texture(shadowMap, projCoords.xy).r;
  float currentDepth = projCoords.z;

  float shadow = 0.0f;
  if(currentDepth <= 1.0f)
  {
    //float bias = max(0.005f * (1.0f - dot(normal, lightDir)), 0.005f);
    float bias = 0.005f;
    //shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

    vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
      for(int y = -1; y <= 1; ++y)
      {
        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0f : 0.0f;
      }
    }
    shadow /= 9.0f;
  }

  return shadow;
}

void
main()
{
  vec3 normal   = vec3(0.0f);
  vec3 lightDir = vec3(0.0f);
  vec3 viewDir  = vec3(0.0f);

  if((frag.flags & NORMAL_MAP) != 0)
  {
    normal   = normalize(texture(material.normalMap, frag.texCoord).rgb);
    normal   = normalize(normal * 2.0f - 1.0f);
    lightDir = normalize(frag.tangentLightPos - frag.tangentFragPos);
    viewDir  = normalize(frag.tangentViewPos - frag.tangentFragPos);
  }
  else
  {
    normal   = normalize(frag.normal);
    lightDir = normalize(frag.lightPos - frag.position);
    viewDir  = normalize(frag.cameraPos - frag.position);
  }

  vec3 half_vector = normalize(lightDir + viewDir);

  // --------AMBIENT--------
  vec3 ambient = light.ambient;

  // --------DIFFUSE------
  float diffuse_intensity = max(dot(normal, lightDir), 0.0f);
  vec3  diffuse           = diffuse_intensity * light.diffuse;
  if((frag.flags & DIFFUSE_MAP) != 0)
  {
    ambient *= vec3(texture(material.diffuseMap, frag.texCoord));
    diffuse *= vec3(texture(material.diffuseMap, frag.texCoord));
  }
  else
  {
    ambient *= material.ambientColor;
    diffuse *= material.diffuseColor.rgb;
  }

  // --------SPECULAR------
  float specular_intensity = pow(max(dot(normal, half_vector), 0.0f), material.shininess);
  specular_intensity *= (diffuse_intensity > 0.0f) ? 1.0f : 0.0f;
  vec3 specular = specular_intensity * light.specular;
  if((frag.flags & SPECULAR_MAP) != 0)
  {
    specular *= vec3(texture(material.specularMap, frag.texCoord));
  }
  else
  {
    specular *= material.specularColor;
  }

  // --------SHADOW--------
  float shadow = ShadowCalculation(frag.lightFragPos, normal, lightDir);
  float lighting = 1.0f - shadow;

  // --------FINAL----------
  vec4 result = vec4(0.0f);
  if((frag.flags & DIFFUSE_MAP) != 0)
  {
    result = vec4((lighting * (diffuse + specular) + ambient), 1.0f);
  }
  else
  {
    result = vec4((lighting * (diffuse + specular) + ambient), material.diffuseColor.a);
  }

  //out_color = vec4(vec3(lighting), 1.0f);
  out_color = result;
}
