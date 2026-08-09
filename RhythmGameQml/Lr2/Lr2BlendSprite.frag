#version 440

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 tint;
    vec4 transColor;
    float colorKeyEnabled;
    float blendMode;
} ubuf;

layout(binding = 1) uniform sampler2D source;

void main()
{
    vec4 tex = texture(source, vTexCoord);
    if (ubuf.colorKeyEnabled > 0.5
            && abs(tex.r - ubuf.transColor.r) < 0.001
            && abs(tex.g - ubuf.transColor.g) < 0.001
            && abs(tex.b - ubuf.transColor.b) < 0.001) {
        discard;
    }

    float opacity = tex.a * ubuf.qt_Opacity;
    vec3 color = tex.rgb * ubuf.tint.rgb;
    if (abs(ubuf.blendMode - 4.0) < 0.5) {
        // The destination-color blend factor performs the multiplication.
        // Fold source/DST opacity into the multiplier so fades approach white
        // (the identity value) instead of unexpectedly darkening the scene.
        color = mix(vec3(1.0), color, opacity);
    }
    fragColor = vec4(color, opacity);
}
