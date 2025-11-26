#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 color;
    int isSelected;
} push;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;
    int lightingEnable;
} ubo;

void main(){
    vec3 baseColor;
    if (length(push.color) > 0.01) {
        baseColor = push.color;  // GameObject의 color 사용
    } else {
        baseColor = fragColor;   // 버텍스 색상 사용
    }

    vec3 finalColor;

    if (ubo.lightingEnable == 1) {
        // 조명이 켜져 있을 때: 방향성 조명 계산
        // 각 프레임마다 빛까지의 방향 계산
        vec3 directionToLight = ubo.lightPosition - fragPosWorld;
        // 거리 감쇠 계산
        float attenuation = 1.0 / dot(directionToLight, directionToLight);

        // 감쇠가 적용된 빛 색상
        vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
        vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

        // MoltenVK(macOS) 호환성을 위해 노멀 벡터 명시적 정규화
        vec3 normalWorld = normalize(fragNormalWorld);
        vec3 directionToLightNorm = normalize(directionToLight);

        float cosAngIncidence = max(dot(normalWorld, directionToLightNorm), 0);
        vec3 diffuseLight = lightColor * cosAngIncidence;

        finalColor = (diffuseLight + ambientLight) * baseColor;
    } else {
        // 조명이 꺼져 있을 때: 균일한 밝기 (플랫 셰이딩)
        finalColor = baseColor * 0.8;
    }
    // 선택된 객체라면 하이라이트 효과 적용
    if (push.isSelected == 1) {
        // 노멀 벡터를 이용한 가장자리 감지 (더 강하게)
        vec3 normalWorld = normalize(fragNormalWorld);
        float edge = 1.0 - abs(normalWorld.z);
        edge = pow(edge, 1.0);  // 지수를 낮춰서 더 넓은 영역에 적용
        
        // 밝은 노란색/주황색 테두리
        vec3 edgeColor = vec3(1.0, 0.8, 0.0);  // 주황빛 노란색
        
        // 가장자리는 매우 강하게 표시
        if (edge > 0.2) {
            finalColor = mix(finalColor, edgeColor, 0.8);  // 80% 혼합
            finalColor *= 2.0;  // 2배 밝게
        } else {
            // 중앙부도 약간 밝게
            finalColor = mix(finalColor, vec3(1.0, 1.0, 0.0), 0.2);
            finalColor *= 1.3;
        }
    }

    outColor = vec4(finalColor, 1.0);
}