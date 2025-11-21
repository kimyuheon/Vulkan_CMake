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
    mat4 projectionView;
    vec4 ambientLightColor;
    vec3 lightDirection;
    int lightingEnable;
} ubo;

void main(){
    vec3 finalColor;

    if (ubo.lightingEnable == 1) {
        // 조명이 켜져 있을 때: 방향성 조명 계산
        vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
        vec3 directionToLight = normalize(-ubo.lightDirection);
        float diffuseStrength = max(dot(fragNormalWorld, directionToLight), 0);
        vec3 diffuseLight = vec3(1.0) * diffuseStrength;

        finalColor = (diffuseLight + ambientLight) * fragColor;
    } else {
        // 조명이 꺼져 있을 때: 균일한 밝기 (플랫 셰이딩)
        finalColor = fragColor * 0.7;
    }
    // 선택된 객체라면 하이라이트 효과 적용
    if (push.isSelected == 1) {
        // 노멀 벡터를 이용한 가장자리 감지 (더 강하게)
        float edge = 1.0 - abs(fragNormalWorld.z);
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