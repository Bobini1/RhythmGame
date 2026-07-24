#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float phase;
};

void main()
{
    float glow = 0.2 + 0.15 * sin(phase * 6.28318530718);
    fragColor = vec4(glow, glow * 1.4, glow * 2.0, 1.0) * qt_Opacity;
}
