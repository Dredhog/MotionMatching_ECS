#version 330 core

struct Material
{
  sampler2D diffuseMap;
  sampler2D normalMap;
  sampler2D depthMap;
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

out vec4 out_color;

void
main()
{
  vec3 normal = normalize(frag.normal);
  vec3 tangent = normalize(frag.tangent);
  vec3 bitangent = cross(frag.normal, frag.tangent);

  mat3 uv_to_world = mat3(tangent, bitangent, normal);
  vec3 uv_normal = normalize(vec3(texture(material.normalMap, frag.texCoord)*2-1));
  uv_normal.g *= -1;
  vec3 worldNormal = uv_to_world * uv_normal;
  
  vec3 lightDir = normalize(lightPosition - frag.position);
  vec3 viewDir  = normalize(cameraPosition - frag.position);

  float diffuseAmount = max(dot(lightDir, worldNormal), 0);
  //if(max(dot(lightDir, frag.normal), 0) == 0)
  //  diffuseAmount = 0;
  diffuseAmount *= max(dot(lightDir, frag.normal), 0);
  vec3 diffuse  = diffuseAmount*vec3(texture(material.diffuseMap, frag.texCoord));
  
  out_color     = vec4(diffuse, 1.0);
}
