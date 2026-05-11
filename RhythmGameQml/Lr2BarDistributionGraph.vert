#version 440
layout (location = 0) in vec2 inVertex;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec2 inLocalCoord;
layout (location = 3) in vec4 inTintAlpha;
layout (location = 4) in vec4 inFlags;
layout (location = 5) in vec4 inSourcePx;

layout (location = 0) out vec2 vTexCoord;
layout (location = 1) out vec2 vLocalCoord;
layout (location = 2) out vec4 vTintAlpha;
layout (location = 3) out vec4 vFlags;
layout (location = 4) out vec4 vSourcePx;

layout (std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    vec4 transColor;
    vec2 textureSize;
    float tolerance;
    float padding;
} ubuf;

void main(void)
{
    vTexCoord = inTexCoord;
    vLocalCoord = inLocalCoord;
    vTintAlpha = inTintAlpha;
    vFlags = inFlags;
    vSourcePx = inSourcePx;
    gl_Position = ubuf.qt_Matrix * vec4(inVertex, 0.0, 1.0);
}
