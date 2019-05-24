#version 330 core


struct dir_light
{
  vec3 specular;
  vec3 diffuse;
  vec3 ambient;
  vec3 direction;
};

uniform dir_light sun;
uniform vec3      cameraPosition;
uniform vec3      u_AlbedoColor;
uniform sampler2D u_NormalMap;

in VertexOut
{
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texCoord;
} frag;


out vec4 out_color;

void
main()
{
  vec3 cameraDir  = normalize(cameraPosition - frag.position);
  vec3 sun_direction = -sun.direction;

  vec3 normal    = normalize(frag.normal);
  vec3 tangent   = normalize(frag.tangent);
  vec3 bitangent = cross(frag.normal, frag.tangent);

  mat3 uv_to_world = mat3(tangent, -bitangent, normal);
  vec2 texCoord    = frag.texCoord;
  vec3 uv_normal   = normalize(vec3(texture(u_NormalMap, texCoord)*2-1));
  vec3 worldNormal = uv_to_world * uv_normal;
  
  float diffuseAmount = max(dot(sun_direction, worldNormal), 0);
  diffuseAmount *= max(dot(sun_direction, frag.normal), 0);
  vec3 diffuse   = diffuseAmount*u_AlbedoColor;
  
  vec3 reflectedDir    = reflect(-sun_direction, worldNormal);
  float specularAmount = pow(max(dot(reflectedDir, cameraDir), 0), 20);
  specularAmount      *=  max(dot(sun_direction, frag.normal), 0);
  vec3 specular        = specularAmount*vec3(1,1,1);

  float ambientAmount = 0.2;
  vec3 ambient = ambientAmount*u_AlbedoColor;
  
  out_color     = vec4(ambient+diffuse+specular, 1.0);
  //out_color = vec4(normalize(sun_direction), 1.0);
}
