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
    float red = 0.10 + 0.50 * phase;
    float green = 0.20 + 0.25 * phase;
    float blue = 0.80 - 0.50 * phase;
    fragColor = vec4(red, green, blue, 1.00) * qt_Opacity;
}
