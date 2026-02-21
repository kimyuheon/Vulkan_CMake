#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragPosWorld;
layout (location = 2) out vec3 fragNormalWorld;
layout (location = 3) out vec2 fragTexCoord;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 color;
    int isSelected;
} push;

struct PointLight {
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
    //vec3 lightPosition;
    //vec4 lightColor;
    PointLight pointLights[10];
    int numLights;
    int lightingEnable;
} ubo;


void main() {
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

    if (push.isSelected == 2) {
        // 3D 오브젝트: 클립스페이스 노멀 방향으로 확장 (vase, cube 등)
        gl_Position = ubo.projection * ubo.view * positionWorld;
        vec3 worldNormal = normalize(mat3(push.normalMatrix) * normal);
        vec3 viewNormal = mat3(ubo.view) * worldNormal;
        vec2 screenDir = vec2(
            ubo.projection[0][0] * viewNormal.x,
            ubo.projection[1][1] * viewNormal.y
        );
        float len = length(screenDir);
        if (len > 0.001) {
            screenDir /= len;
            gl_Position.xy += screenDir * 0.007 * gl_Position.w;
        }
    } else if (push.isSelected == 3) {
        // 평면 오브젝트: 오브젝트 중심에서 방사형 확장 (floor 등)
        gl_Position = ubo.projection * ubo.view * positionWorld;
        vec4 clipCenter = ubo.projection * ubo.view * vec4(push.modelMatrix[3].xyz, 1.0);
        vec2 screenDir = gl_Position.xy / gl_Position.w - clipCenter.xy / clipCenter.w;
        float len = length(screenDir);
        if (len > 0.001) {
            screenDir /= len;
            gl_Position.xy += screenDir * 0.007 * gl_Position.w;
        }
    } else {
        gl_Position = ubo.projection * ubo.view * positionWorld;
    }

    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
    fragTexCoord = uv;
}