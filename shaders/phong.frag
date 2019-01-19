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
};

struct Sun	
{
  vec3 ambient;
  vec3 diffuse;
};

in VertexOut
{
  flat int flags;

  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 tangentLightPos;
  vec3 tangentSunPos;
  vec3 tangentViewPos;
  vec3 tangentFragPos;
  vec4 sunFragPos;
  float viewDepth;
}
frag;

uniform Material material;
uniform Light light;
uniform Sun sun;

uniform vec3 lightPosition;
uniform vec3 sunDirection;
uniform vec3 cameraPosition;

uniform sampler2D u_AmbientOcclusion;

#define SHADOWMAP_CASCADE_COUNT 4
uniform float u_CascadeFarPlanes[SHADOWMAP_CASCADE_COUNT];
uniform mat4 mat_sun_vp[SHADOWMAP_CASCADE_COUNT];
uniform sampler2D shadowMap_0;
uniform sampler2D shadowMap_1;
uniform sampler2D shadowMap_2;
uniform sampler2D shadowMap_3;
out vec4 out_color;

float
ShadowCalculation(vec3 fragWorldPos, int shadowmapIndex, vec3 normal,  sampler2D shadowMap)
{
  vec4 sunFragPos = mat_sun_vp[shadowmapIndex] * vec4(fragWorldPos, 1.0f);

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
    lightDir = normalize(lightPosition - frag.position);
    viewDir  = normalize(cameraPosition - frag.position);
  }

  int cascadeIndex = 0;
  for(int i = 0; i < SHADOWMAP_CASCADE_COUNT; i++)
  {
    if(frag.viewDepth < u_CascadeFarPlanes[i])
    {
      cascadeIndex = i;
      break;
    }
  }
  // --------SHADOW--------
  float shadow = ShadowCalculation(frag.position,  cascadeIndex, normal,
        (cascadeIndex == 0) ? shadowMap_0 :
                             (cascadeIndex == 1) ? shadowMap_1 :
																					(cascadeIndex == 2) ? shadowMap_2 : shadowMap_3);
  float lighting = 1.0f - shadow;

  vec3 point_half_vector = normalize(lightDir + viewDir);
  vec3 sun_half_vector = normalize(-sunDirection +viewDir);

  // --------AMBIENT--------
  vec3 ambient = light.ambient;
  ambient += sun.ambient;

  // --------DIFFUSE------
  float diffuse_intensity = max(dot(normal, lightDir), 0.0f);
  vec3  diffuse           = diffuse_intensity * light.diffuse;

  float sun_diffuse_intensity = max(dot(normal, -sunDirection), 0.0f);
  diffuse += sun_diffuse_intensity * sun.diffuse * lighting;

  if((frag.flags & DIFFUSE_MAP) != 0)
  {
    float offset_delta = 0.005f;
#if 0	
    vec3 diffuse_sample = texture(material.diffuseMap, frag.texCoord + vec2(offset_delta, offset_delta)).xyz + texture(material.diffuseMap, frag.texCoord+vec2(-offset_delta, offset_delta)).xyz + texture(material.diffuseMap, frag.texCoord+vec2(offset_delta, -offset_delta)).xyz + texture(material.diffuseMap, frag.texCoord+vec2(-offset_delta, -offset_delta)).xyz;
    diffuse_sample /= 4;
#else
    vec3 diffuse_sample = texture(material.diffuseMap, frag.texCoord).xyz;
#endif
    ambient *= diffuse_sample;			
    diffuse *= diffuse_sample;
  }
  else
  {
    ambient *= material.ambientColor;
    diffuse *= material.diffuseColor.rgb;
  }
  
  vec2 screen_tex_coords = gl_FragCoord.xy/textureSize(u_AmbientOcclusion, 0).xy; 
  float ambient_occlusion = texture(u_AmbientOcclusion, screen_tex_coords).r;
  ambient *= ambient_occlusion;

  // --------SPECULAR------
  float specular_intensity = pow(max(dot(normal, point_half_vector), 0.0f), material.shininess);
  specular_intensity *= (diffuse_intensity > 0.0f) ? 1.0f : 0.0f;
  vec3 specular = specular_intensity * light.diffuse;

  float sun_specular_intensity = pow(max(dot(normal, sun_half_vector), 0.0f), material.shininess);
  specular += sun_specular_intensity * sun.diffuse * lighting;
  if((frag.flags & SPECULAR_MAP) != 0)
  {
    specular *= vec3(texture(material.specularMap, frag.texCoord));
  }
  else
  {
    specular *= material.specularColor;
  }

  // --------FINAL----------
  vec4 result = vec4(0.0f);
  if((frag.flags & DIFFUSE_MAP) != 0)
  {
    result = vec4(diffuse + specular + ambient, 1.0f);
  }
  else
  {
    result = vec4(diffuse + specular + ambient, material.diffuseColor.a);
  }
//vec4((cascadeIndex == 0) ? vec3(1,0,0) :
//                             (cascadeIndex == 1) ? vec3(0,1,0) :
//																					//(cascadeIndex == 2) ? vec3(0,0,1) : vec3(1,0,1), 1);
  out_color = result;
}
