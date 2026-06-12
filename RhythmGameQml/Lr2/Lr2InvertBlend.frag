#version 440
layout (location = 0) in vec2 qt_TexCoord0;
layout (location = 0) out vec4 fragColor;
layout (std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
} ubuf;

layout (binding = 1) uniform sampler2D source;

// LR2 blend mode 10 (INVSRC / ANTI_COLOR).
//
// LR2/DxLib draws the inverted source color with the usual source alpha.
// Qt Quick blends ShaderEffect output as premultiplied alpha, so emit
// (1-rgb) already multiplied by the effective alpha.
void main(void) {
    vec4 tex = texture(source, qt_TexCoord0);
    float a = tex.a * ubuf.qt_Opacity;
    fragColor = vec4((1.0 - tex.rgb) * a, a);
}
