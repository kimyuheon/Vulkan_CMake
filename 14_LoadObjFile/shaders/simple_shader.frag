#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 transform;
    vec3 color;
    int isSelected;  // 선택 상태를 나타내는 플래그
} push;

void main(){
    vec3 baseColor = fragColor;

    // 선택된 객체라면 하이라이트 효과 적용
    if (push.isSelected == 1) {
        // 밝은 노란색 테두리 효과
        vec3 highlightColor = vec3(1.0, 1.0, 0.0);  // 노란색
        // 기본 색상과 하이라이트 색상을 섞어서 밝게 만들기
        baseColor = mix(baseColor, highlightColor, 0.4);
        // 전체적으로 더 밝게
        baseColor = baseColor * 1.3;
    }

    outColor = vec4(baseColor, 1.0);
}