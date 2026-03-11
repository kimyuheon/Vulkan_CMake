#version 450

layout(location = 0) in  vec2 fragUV;
layout(location = 0) out vec4 outColor;

// Final JFA distance field (rg = nearest seed UV)
layout(set = 0, binding = 0) uniform sampler2D jfaResult;

layout(push_constant) uniform Push {
    vec2  resolution;        // viewport size in pixels
    float outlineThickness;  // outline width in pixels (e.g. 2.0)
} push;

void main() {
    vec2 nearestSeedUV = texture(jfaResult, fragUV).rg;

    // Background: seed was never propagated → no outline
    if (nearestSeedUV.x < 0.0) discard;

    // Convert UV distance to pixel distance
    vec2 pixelCoord     = fragUV * push.resolution;
    vec2 seedPixelCoord = nearestSeedUV * push.resolution;
    float dist = distance(pixelCoord, seedPixelCoord);

    // Draw outline ring: pixels just outside the selection mask
    if (dist < 0.5 || dist > push.outlineThickness + 0.5) discard;

    outColor = vec4(1.0, 1.0, 0.0, 1.0); // yellow
}
