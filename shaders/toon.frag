#version 330 core

struct Material
{
  vec3 ambientColor;
  vec4 diffuseColor;
  vec3 specularColor;

  float shininess;
};

struct Light
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct Sun
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

in VertexOut
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 lightPos;
  vec3 sunDir;
  vec3 cameraPos;
  vec4 sunFragPos;
}
frag;

uniform Material material;
uniform Light light;
uniform Sun sun;
uniform int levelCount;

uniform sampler2D shadowMap;

out vec4 out_color;

float
ShadowCalculation(vec4 sunFragPos, vec3 normal, vec3 lightDir)
{
  vec3 projCoords = sunFragPos.xyz / sunFragPos.w;
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

float
Categorize(float brightness, int levelCount)
{
  float level = floor(brightness * levelCount);
  return level / levelCount;
}

void
main()
{
  vec3 normal   = normalize(frag.normal);
  vec3 lightDir = normalize(frag.lightPos - frag.position);
  vec3 viewDir  = normalize(frag.cameraPos - frag.position);

  // --------SHADOW--------
  float shadow = ShadowCalculation(frag.sunFragPos, normal, frag.sunDir);
  float lighting = 1.0f - shadow;

  vec3 sunDir = normalize(-frag.sunDir);
  vec3 half_vector = normalize(lightDir + viewDir);
  vec3 reflectDir = reflect(-sunDir, normal);

  // --------DIFFUSE------
  float diffuse_intensity = max(dot(normal, lightDir), 0.0f);

  float sun_diffuse_intensity = max(dot(normal, sunDir), 0.0f);

  // --------SPECULAR------
  float specular_intensity = pow(max(dot(normal, half_vector), 0.0f), material.shininess);
  specular_intensity *= (diffuse_intensity > 0.0f) ? 1.0f : 0.0f;

  float sun_specular_intensity = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);

  float DiffuseLight = diffuse_intensity + sun_diffuse_intensity;
  float SpecularLight = specular_intensity + sun_specular_intensity;
  float TotalLight = (DiffuseLight + SpecularLight) / 2.0;
  TotalLight = Categorize(TotalLight, levelCount) - 0.5 * Categorize(shadow, levelCount);

  vec3 Color = TotalLight * material.diffuseColor.rgb;

  // --------FINAL----------
  out_color  = vec4(Color, material.diffuseColor.a);
}
