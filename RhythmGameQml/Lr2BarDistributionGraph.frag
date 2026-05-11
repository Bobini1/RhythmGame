#version 440
layout (location = 0) in vec2 vTexCoord;
layout (location = 1) in vec2 vLocalCoord;
layout (location = 2) in vec4 vTintAlpha;
layout (location = 3) in vec4 vFlags;
layout (location = 4) in vec4 vSourcePx;

layout (location = 0) out vec4 fragColor;

layout (std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    vec4 transColor;
    vec2 textureSize;
    float tolerance;
    float padding;
} ubuf;

layout (binding = 1) uniform sampler2D source;

void main(void)
{
    vec2 uv = vTexCoord;
    if (vFlags.z > 0.5) {
        vec2 textureSizePx = max(ubuf.textureSize, vec2(1.0));
        vec2 sourceStartPx = vSourcePx.xy;
        vec2 sourceSizePx = vSourcePx.zw;
        vec2 minPx = min(sourceStartPx, sourceStartPx + sourceSizePx);
        vec2 maxPx = max(sourceStartPx, sourceStartPx + sourceSizePx) - vec2(1.0);
        vec2 samplePx = sourceStartPx + vLocalCoord * sourceSizePx;
        samplePx = clamp(floor(samplePx), minPx, maxPx) + vec2(0.5);
        uv = samplePx / textureSizePx;
    }

    vec4 tex = texture(source, uv);
    if (vFlags.y > 0.5
            && abs(tex.r - ubuf.transColor.r) < ubuf.tolerance
            && abs(tex.g - ubuf.transColor.g) < ubuf.tolerance
            && abs(tex.b - ubuf.transColor.b) < ubuf.tolerance) {
        fragColor = vec4(0.0);
        return;
    }

    float a = tex.a * vTintAlpha.a * ubuf.qt_Opacity;
    if (abs(vFlags.x - 2.0) < 0.5) {
        fragColor = vec4(tex.rgb * vTintAlpha.rgb * a, 0.0);
    } else if (abs(vFlags.x - 10.0) < 0.5) {
        fragColor = vec4((1.0 - tex.rgb) * a, a);
    } else {
        fragColor = vec4(tex.rgb * vTintAlpha.rgb * a, a);
    }
}
