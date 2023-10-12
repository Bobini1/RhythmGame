#version 440
layout (location = 0) in vec2 qt_TexCoord0;
layout (location = 0) out vec4 fragColor;
layout (std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    vec4 from;
    vec4 to;
    float tolerance;
} ubuf;

layout (binding = 1) uniform sampler2D source;
void main(void) {
    vec4 tex = texture(source, qt_TexCoord0);
    if (abs(tex.r - ubuf.from.r) < ubuf.tolerance
    && abs(tex.g - ubuf.from.g) < ubuf.tolerance
    && abs(tex.b - ubuf.from.b) < ubuf.tolerance
    ) {
        fragColor = ubuf.to;
    } else {
        fragColor = tex;
    }
}