#version 440
layout (location = 0) in vec2 qt_TexCoord0;
layout (location = 0) out vec4 fragColor;

layout (std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    vec4 qt_SubRect_source;
    vec4 sourceRect;
    vec4 tint;
    vec4 transColor;
    float blendMode;
    float colorKeyEnabled;
    float tolerance;
    float nearestMode;
    vec2 sourceSize;
} ubuf;

layout (binding = 1) uniform sampler2D source;

void main(void) {
    vec2 sourceSubRectSize = max(abs(ubuf.qt_SubRect_source.zw), vec2(0.000001));
    vec2 localCoord = (qt_TexCoord0 - ubuf.qt_SubRect_source.xy) / sourceSubRectSize;
    vec2 localUv = ubuf.sourceRect.xy + localCoord * ubuf.sourceRect.zw;
    vec2 uv = ubuf.qt_SubRect_source.xy + ubuf.qt_SubRect_source.zw * localUv;
    if (ubuf.nearestMode > 0.5) {
        vec2 textureSizePx = max(ubuf.sourceSize, vec2(1.0));
        vec2 sourceStartPx = ubuf.sourceRect.xy * textureSizePx;
        vec2 sourceSizePx = ubuf.sourceRect.zw * textureSizePx;
        vec2 minPx = min(sourceStartPx, sourceStartPx + sourceSizePx);
        vec2 maxPx = max(sourceStartPx, sourceStartPx + sourceSizePx) - vec2(1.0);
        vec2 samplePx;
        if (ubuf.nearestMode > 1.5) {
            vec2 destTexel = max(fwidth(localCoord), vec2(0.000001));
            vec2 destSizePx = max(vec2(1.0), vec2(1.0) / destTexel);
            vec2 destIndex = clamp(localCoord * destSizePx - vec2(0.5),
                vec2(0.0),
                max(destSizePx - vec2(1.0), vec2(0.0)));
            vec2 destSpan = max(destSizePx - vec2(1.0), vec2(1.0));
            vec2 sourceSpan = max(sourceSizePx - vec2(1.0), vec2(0.0));
            samplePx = sourceStartPx + destIndex * sourceSpan / destSpan;
            samplePx = floor(samplePx + vec2(0.5));
        } else {
            vec2 destTexel = fwidth(localCoord);
            samplePx = sourceStartPx
                + max(vec2(0.0), localCoord - destTexel * 0.5) * sourceSizePx;
            samplePx = floor(samplePx);
        }
        samplePx = clamp(samplePx, minPx, maxPx) + vec2(0.5);
        vec2 localSampleUv = samplePx / textureSizePx;
        uv = ubuf.qt_SubRect_source.xy + ubuf.qt_SubRect_source.zw * localSampleUv;
    }
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
