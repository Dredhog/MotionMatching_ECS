#version 330 core

out vec4 o_FragColor;

in vec2 TexCoords;

const int SAMPLE_VECTOR_COUNT = 9;

uniform vec3      u_SampleVectors[SAMPLE_VECTOR_COUNT];
uniform float     u_SamplingRadius;
uniform mat4      u_mat_projection;
uniform sampler2D u_NormalMap;
uniform sampler2D u_PositionMap;

void
main()
{
  vec3 normal = normalize(vec3(texture(u_NormalMap, TexCoords)));
  vec3 position = vec3(texture(u_PositionMap, TexCoords)); //NOTE: Positoin.z < 0.0f


  float SecondaryRadius = 3*u_SamplingRadius;
  o_FragColor = vec4(vec3(0), 1);
  float occlusion = 0;
  for(int i = 0; i < SAMPLE_VECTOR_COUNT; i++)
  {

    //vec3 random_dir = normalize(vec3(int(gl_FragCoord.x*3) % 100, int(gl_FragCoord.z*3) % 100, 0));
    vec3 random_dir = vec3(1,1,1);
    vec3 tangent = normalize(random_dir - normal*dot(random_dir, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);
    
    vec3 offset = u_SamplingRadius * (tbn * u_SampleVectors[i]);
    vec3 test_point_view_space = position + offset;
    vec4 test_point_uv_space = u_mat_projection * vec4(test_point_view_space, 1.0f);
    test_point_uv_space.xy = (test_point_uv_space.xy / test_point_uv_space.w) * 0.5 + 0.5;
    		
    if(0 <= test_point_uv_space.x && test_point_uv_space.x <= 1 &&
      0 <= test_point_uv_space.y && test_point_uv_space.y <= 1){
      float sample_depth = texture(u_PositionMap, test_point_uv_space.xy).z;
      float range_check = (abs(sample_depth - test_point_view_space.z) < SecondaryRadius) ? 1 : 0;
      occlusion += ((test_point_view_space.z < sample_depth) ? 1.0f : 0.0f) * range_check;
    }
  }
  occlusion = 1.0f - (occlusion/SAMPLE_VECTOR_COUNT);
  o_FragColor = vec4(vec3(occlusion), 1);
}
