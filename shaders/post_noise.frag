#version 330 core

#define PI 3.14159265359

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ScreenTex;

uniform float u_Time;

// Modified 2D random from "The Book of Shaders"
// https://thebookofshaders.com
float
Random(vec2 st)
{
    return 0.5 * sin(u_Time - 0.5 * u_Time * PI) + fract(sin(u_Time - 0.2 * u_Time * PI) + sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// 2D Noise based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float
Noise(in vec2 st)
{
    vec2 IntPart = floor(st);
    vec2 FloatPart = fract(st);
    
    // 2D tile corners
    float CornerA = Random(IntPart);
    float CornerB = Random(IntPart + vec2(1.0, 0.0));
    float CornerC = Random(IntPart + vec2(0.0, 1.0));
    float CornerD = Random(IntPart + vec2(1.0, 1.0));
    
    // Smooth Interpolation
    vec2 Smooth = smoothstep(0.0, 1.0, FloatPart);
    
    // Mix 4 coorners percentages
    return mix(CornerA, CornerB, Smooth.x) + (CornerC - CornerA) * Smooth.y * (1.0 - Smooth.x) + (CornerD - CornerB) * Smooth.x * Smooth.y;
}

// Function from Iñigo Quiles
// www.iquilezles.org/www/articles/functions/functions.htm
float
Parabola(float x, float k)
{
    return pow(4.0 * x * (1.0 - x), k);
}

void main()
{
    vec2 st = TexCoords;
    vec2 pos = vec2(st * 5.0);
    float n = Noise(pos);
    n *= sin(PI * st.x * st.y + u_Time);
    vec3 TexColor = vec3(texture(ScreenTex, TexCoords));
    vec3 Color = mix(vec3(0.5), TexColor, n);
    vec3 FinalColor = mix(Color, TexColor, 0.2 * st.x);
    float Scale = Parabola(st.x, 1.0) * Parabola(st.y, 1.0);
    Scale = clamp(Scale, 0.0, 1.0);
    FragColor = vec4(mix(FinalColor, TexColor, Scale), 1.0);
}
