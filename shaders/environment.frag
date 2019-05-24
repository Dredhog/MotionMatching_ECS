#version 330 core

#define REFRACTION 1 << 0
#define NORMAL_MAP 1 << 1
#define SKELETAL 1 << 2

in VertexOut
{
    flat int flags;

    vec3 position;
    vec3 normal;
    vec2 texCoord;
    vec3 cameraPos;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
} frag;

uniform samplerCube cubemap;
uniform sampler2D normalMap;
uniform float refractive_index;

out vec4 out_color;

void
main()
{
    vec3 normal = vec3(0.0f);
    vec3 invViewDir = vec3(0.0f);

    if((frag.flags & NORMAL_MAP) != 0)
    {
        normal = normalize(texture(normalMap, frag.texCoord).rgb);
        normal = normalize(normal * 2.0f - 1.0f);
        invViewDir = normalize(frag.tangentFragPos - frag.tangentViewPos);
    }
    else
    {
        normal = normalize(frag.normal);
        invViewDir = normalize(frag.position - frag.cameraPos);
    }

    vec3 I = normalize(invViewDir);
    vec3 R = vec3(0.0f);

    if((frag.flags & REFRACTION) != 0)
    {
        float ratio = 1.0f / refractive_index;
        R = refract(I, normalize(normal), ratio);
    }
    else
    {
        R = reflect(I, normalize(normal));
    }

    out_color = vec4(texture(cubemap, R).rgb, 1.0f);
}
