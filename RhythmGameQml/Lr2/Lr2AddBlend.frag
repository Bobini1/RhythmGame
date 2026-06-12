#version 440
layout (location = 0) in vec2 qt_TexCoord0;
layout (location = 0) out vec4 fragColor;
layout (std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
} ubuf;

layout (binding = 1) uniform sampler2D source;

// LR2 blend mode 2 (ADD).
//
// Qt Quick's ShaderEffect uses premultiplied-alpha blending:
//     out = src.rgb + dst.rgb * (1 - src.a)
// By emitting a fragment with alpha = 0, the (1 - src.a) term is 1 and the
// blend collapses to pure additive:
//     out = src.rgb + dst.rgb
// We still need to scale RGB by the item's own alpha * qt_Opacity so that
// fade in/out animations work correctly.
void main(void) {
    vec4 tex = texture(source, qt_TexCoord0);
    float a = tex.a * ubuf.qt_Opacity;
    fragColor = vec4(tex.rgb * a, 0.0);
}
