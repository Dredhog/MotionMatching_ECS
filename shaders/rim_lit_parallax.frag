#version 330 core

struct Material
{
  sampler2D diffuseMap;
  sampler2D normalMap;
  sampler2D depthMap;
  vec3 rimColor;
  float rimStrength;
};

uniform vec3 lightPosition;
uniform vec3 cameraPosition;

struct Light
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};	

in VertexOut
{
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texCoord;
} frag;

uniform Material material;
uniform Light light;

uniform float height_scale;

vec2 ComputeParallaxMappedCoord(vec2 texCoord, vec3 uv_space_cameraDir)
{
  float depth = texture(material.depthMap, texCoord).x;
  vec2 offset = (depth*height_scale/ uv_space_cameraDir.z) * -uv_space_cameraDir.xy ;
  return texCoord + offset;
}

vec2 ComputeParallaxMappedCoordV2(vec2 texCoord, vec3 uv_space_cameraDir)
{
  float depth_layer_count = mix(5, 20, 1-max(dot(vec3(0,0,1), uv_space_cameraDir), 0));
  float d_z = 1 / depth_layer_count;
  vec2  d_uv = height_scale* (-uv_space_cameraDir.xy);

  float ray_depth = 0;
  float texture_depth = texture(material.depthMap, texCoord).x;
   
  int i = 0;
  float prev_texture_depth = texture_depth;
  for(;ray_depth < texture_depth && i < depth_layer_count; i++)
  { 
    ray_depth += d_z;
    prev_texture_depth = texture_depth;
    texture_depth = texture(material.depthMap, texCoord + i*d_uv).x;
  }
  
  float depthAfter  = texture_depth - ray_depth;
  float depthBefore = prev_texture_depth - (ray_depth - d_z);

  float weight = depthAfter/(depthAfter-depthBefore);
  vec2 offset = mix((i-1)*d_uv, i*d_uv, 1-weight);
  return texCoord + offset;
}

out vec4 out_color;

void
main()
{
  vec3 lightDir = normalize(lightPosition - frag.position);
  vec3 cameraDir  = normalize(cameraPosition - frag.position);

  vec3 normal    = normalize(frag.normal);
  vec3 tangent   = normalize(frag.tangent);
  vec3 bitangent = cross(frag.normal, frag.tangent);

  mat3 uv_to_world = mat3(tangent, -bitangent, normal);
  vec2 texCoord = ComputeParallaxMappedCoordV2(frag.texCoord, inverse(uv_to_world)*cameraDir);
  /*if(texCoord.x < 0 || 1 < texCoord.x || texCoord.y < 0 || 1 < texCoord.y)
    discard;*/
  vec3 uv_normal   = normalize(vec3(texture(material.normalMap, texCoord)*2-1));
  vec3 worldNormal = uv_to_world * uv_normal;
  
  float diffuseAmount = max(dot(lightDir, worldNormal), 0);
  diffuseAmount *= max(dot(lightDir, frag.normal), 0);
  vec3 diffuse   = diffuseAmount*vec3(texture(material.diffuseMap, texCoord));
  
  vec3 reflectedDir    = reflect(-lightDir, worldNormal);
  float specularAmount = pow(max(dot(reflectedDir, cameraDir), 0), 20);
  specularAmount      *=  max(dot(lightDir, frag.normal), 0);
  vec3 specular        = specularAmount*vec3(1,1,1);

  float ambientAmount = 0.2;
  vec3 ambient = ambientAmount*vec3(texture(material.diffuseMap, texCoord));
  
  float view_dot_normal = max(dot(cameraDir, worldNormal), 0);
  float rim_light_amount = pow(1- pow(view_dot_normal, material.rimStrength), 1/material.rimStrength);

  vec3 parallax_lit_color = ambient+diffuse+specular;
  
  out_color =   vec4(mix(parallax_lit_color, material.rimColor, rim_light_amount), 1);
}
