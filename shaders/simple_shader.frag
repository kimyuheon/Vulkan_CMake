#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 color;
    int isSelected;
    int hasTexture;
    float textureScale;
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

layout (set = 1, binding = 0) uniform sampler2D texSampler;

void main(){
    // 아웃라인 패스: 단색 출력 후 종료 (2=3D 오브젝트, 3=평면 오브젝트)
    if (push.isSelected == 2 || push.isSelected == 3) {
        outColor = vec4(1.0, 0.8, 0.0, 1.0);  // 주황빛 노란색
        return;
    }

    vec3 baseColor;
    if (push.hasTexture == 1) {
        baseColor = texture(texSampler, fragTexCoord * push.textureScale).rgb;
    } else if (length(push.color) > 0.01) {
        baseColor = push.color;  // GameObject의 color 사용
    } else {
        baseColor = fragColor;   // 버텍스 색상 사용
    }

    vec3 finalColor;

    if (ubo.lightingEnable == 1) {
        // 조명이 켜져 있을 때: 방향성 조명 계산
        // 각 프레임마다 빛까지의 방향 계산
        vec3 diffuseLight = vec3(0.0);
        vec3 specularLight = vec3(0.0);
        vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
        vec3 normalWorld = normalize(fragNormalWorld);

        vec3 cameraRightWorld = ubo.inverseView[3].xyz;
        vec3 viewDirection = normalize(cameraRightWorld - fragNormalWorld);

        for(int i = 0; i < ubo.numLights; i++) {
            PointLight light = ubo.pointLights[i];

            vec3 directionToLight = light.position.xyz - fragPosWorld;
            // 거리 감쇠 계산
            float attenuation = 1.0 / dot(directionToLight, directionToLight);
            // 감쇠가 적용된 빛 색상
            vec3 lightColor = light.color.xyz * light.color.w * attenuation;
        
            vec3 directionToLightNorm = normalize(directionToLight);
            float cosAngIncidence = max(dot(normalWorld, directionToLightNorm), 0);

            diffuseLight += lightColor * cosAngIncidence;

            vec3 halfAngle = normalize(directionToLightNorm + viewDirection);
            float blinnTerm = dot(normalWorld, halfAngle);
            blinnTerm = clamp(blinnTerm, 0.0, 1.0);
            blinnTerm = pow(blinnTerm, 32.0);
            specularLight += lightColor * blinnTerm;
        } 

        finalColor = (diffuseLight + ambientLight) * baseColor + specularLight;
    } else {
        // 조명이 꺼져 있을 때: 균일한 밝기 (플랫 셰이딩)
        finalColor = baseColor * 0.8;
    }

    outColor = vec4(finalColor, 1.0);
}