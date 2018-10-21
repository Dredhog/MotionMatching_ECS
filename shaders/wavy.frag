#version 330 core

uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;

uniform vec3 cameraPosition;

dir_light
{
  vec3 specular;
  vec3 diffuse;
  vec3 ambient;
} uniform sun;

in VertexOut
{
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texCoord;
} frag;

uniform Material material;
uniform Light light;

uniform float u_height_scale;

out vec4 out_color;

void
main()
{
  vec3 cameraDir  = normalize(cameraPosition - frag.position);

  vec3 normal    = normalize(frag.normal);
  vec3 tangent   = normalize(frag.tangent);
  vec3 bitangent = cross(frag.normal, frag.tangent);

  mat3 uv_to_world = mat3(tangent, -bitangent, normal);
  vec2 texCoord    = frag.texCoord;
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
  
  out_color     = vec4(ambient+diffuse+specular, 1.0);
}
