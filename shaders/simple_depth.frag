#version 330 core

uniform float cameraNearPlane;
uniform float cameraFarPlane;

float LinearizeDepth(float Depth)
{
    float Z = Depth * 2.0 - 1.0;
    return (2.0 * cameraNearPlane * cameraFarPlane) / (cameraFarPlane + cameraNearPlane - Z * (cameraFarPlane - cameraNearPlane));
}

void
main()
{
    // gl_FragDepth = LinearizeDepth(gl_FragCoord.z) / cameraFarPlane;
    // gl_FragDepth = gl_FragCoord.z;
}
