#version 330 core

uniform vec3 u_AlbedoColor;
uniform sampler2D u_NormalMap;

uniform vec3 cameraPosition;

struct dir_light
{
  vec3 specular;
  vec3 diffuse;
  vec3 ambient;
  vec3 direction;
};

uniform dir_light sun;

in VertexOut
{
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texCoord;
} frag;

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
  vec3 uv_normal   = normalize(vec3(texture(u_NormalMap, texCoord)*2-1));
  vec3 worldNormal = uv_to_world * uv_normal;
  
  float diffuseAmount = max(dot(sun.direction, worldNormal), 0);
  diffuseAmount *= max(dot(sun.direction, frag.normal), 0);
  vec3 diffuse   = diffuseAmount*u_AlbedoColor;
  
  vec3 reflectedDir    = reflect(-sun.direction, worldNormal);
  float specularAmount = pow(max(dot(reflectedDir, cameraDir), 0), 20);
  specularAmount      *=  max(dot(sun.direction, frag.normal), 0);
  vec3 specular        = specularAmount*vec3(1,1,1);

  float ambientAmount = 0.2;
  vec3 ambient = ambientAmount*u_AlbedoColor;
  
  out_color     = vec4(ambient+diffuse+specular, 1.0);
}
