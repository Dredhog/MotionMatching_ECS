#version 330 core

struct Material
{
  sampler2D diffuseMap;
  sampler2D specularMap;
  sampler2D texture3;
  sampler2D texture4;
};

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
  vec2 texCoord;
  vec3 lightPos;
  vec3 cameraPos;
  vec3 tangentLightPos;
  vec3 tangentViewPos;
  vec3 tangentFragPos;
}
frag;

uniform Material material;
uniform Light light;
uniform vec3 uniform_vec;
uniform int  uniform_int;

uniform float uniform_float;

out vec4 out_color;

void
main()
{
  out_color = texture(material.diffuseMap, frag.texCoord) +
              uniform_float * texture(material.specularMap, frag.texCoord);
}
