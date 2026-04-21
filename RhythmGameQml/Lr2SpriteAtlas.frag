#version 440
layout (location = 0) in vec2 qt_TexCoord0;
layout (location = 0) out vec4 fragColor;

layout (std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    vec4 sourceRect;
    vec4 tint;
    vec4 transColor;
    float blendMode;
    float colorKeyEnabled;
    float tolerance;
} ubuf;

layout (binding = 1) uniform sampler2D source;

void main(void) {
    vec2 uv = ubuf.sourceRect.xy + qt_TexCoord0 * ubuf.sourceRect.zw;
    vec4 tex = texture(source, uv);

    if (ubuf.colorKeyEnabled > 0.5 && abs(tex.r - ubuf.transColor.r) < ubuf.tolerance
    && abs(tex.g - ubuf.transColor.g) < ubuf.tolerance
    && abs(tex.b - ubuf.transColor.b) < ubuf.tolerance) {
        fragColor = vec4(0.0);
        return;
    }

    float a = tex.a * ubuf.qt_Opacity;

    if (abs(ubuf.blendMode - 2.0) < 0.5) {
        fragColor = vec4(tex.rgb * ubuf.tint.rgb * a, 0.0);
    } else if (abs(ubuf.blendMode - 10.0) < 0.5) {
        fragColor = vec4((1.0 - tex.rgb) * a, a);
    } else {
        fragColor = vec4(tex.rgb * ubuf.tint.rgb * a, a);
    }
}
