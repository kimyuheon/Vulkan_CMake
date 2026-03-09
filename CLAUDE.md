# CLAUDE.md — Vulkan CAD Engine

> 이 파일은 **Claude Code(CLI)** 와 **Claude.ai(대화)** 가 함께 읽는 공유 컨텍스트입니다.
> - **Claude Code** → 코드 작성/수정/디버깅 시 자동 참조
> - **Claude.ai**   → 설계 논의, 구조 리뷰, 다음 작업 계획 시 참조

---

## 프로젝트 개요

Vulkan + C++17 기반의 3D CAD 엔진.
littleVulkanEngine 튜토리얼(27강)을 기반으로 AutoCAD / 3DS Max 스타일 CAD 기능을 직접 구현하는 장기 학습 & 포트폴리오 프로젝트.

- **GitHub**: https://github.com/kimyuheon/Vulkan_CMake
- **현재 최신 브랜치**: `26_Move_Rotate_Scale` (실질적으로 26번)
- **빌드**: CMake + glslangValidator 자동 셰이더 컴파일
- **플랫폼**: Windows / macOS (MoltenVK) / Linux (Ubuntu)
- **주요 라이브러리**: Vulkan SDK, GLFW, GLM, STB Image, TinyObjLoader

---

## [Claude Code 전용] 코드 작성 규칙

> 코드를 생성/수정할 때 반드시 따라야 할 규칙

### ❌ 절대 하지 말 것
- Euler 각도(`glm::vec3 rotation`) 직접 사용 → **짐벌락 발생**
- 수동 `new` / `delete` → **RAII 패턴만 허용**
- `vkDestroyXxx` 직접 호출 → 소멸자에서만 처리
- `std::vector<LotGameObject>` → **반드시 `LotGameObject::Map` (unordered_map) 사용**
- `setViewYXZ()` 신규 사용 → `setViewFromTransform()` 또는 CAD 모드 사용

### ✅ 반드시 지킬 것
- 회전은 항상 `glm::quat` — `glm::angleAxis()`, `rotateAroundAxis()` 사용
- GLM 헤더 상단: `#define GLM_FORCE_RADIANS`, `#define GLM_FORCE_DEPTH_ZERO_TO_ONE`
- 실험적 GLM 사용 시: `#define GLM_ENABLE_EXPERIMENTAL` + `#include <glm/gtx/quaternion.hpp>`
- 네임스페이스 `lot::` 사용
- 새 오브젝트 추가 후 반드시 `assignDefaultDescriptorSets()` 호출
- 셰이더 수정 후 CMake 리빌드 필요 (자동 .spv 재컴파일)

### 네이밍 컨벤션
| 유형 | 패턴 | 예시 |
|------|------|------|
| 엔진 코어 | `lot_*.h/cpp` | `lot_camera`, `lot_model` |
| 시스템 | `*_system.h/cpp` | `simple_render_system` |
| 관리자 | `*_manager.h/cpp` | `material_manager` |
| 도구 | `*_tool.h/cpp` | `transform_tool` |

---

## [Claude Code 전용] 주요 클래스 & 역할

| 클래스 | 파일 | 핵심 역할 |
|--------|------|-----------|
| `FirstApp` | `first_app` | 메인 루프, 입력 처리, 시스템 오케스트레이션 |
| `LotCamera` | `lot_camera` | CAD/FPS 모드, 쿼터니언 궤도 회전, 투영 전환 |
| `LotGameObject` | `lot_game_object` | 오브젝트 컨테이너, TransformComponent(quat), 선택 상태 |
| `LotModel` | `lot_model` | 버텍스/인덱스 버퍼, AABB 계산, OBJ 로딩 |
| `LotTexture` | `lot_texture` | STB Image → Vulkan Image/Sampler |
| `LotPipeline` | `lot_pipeline` | Vulkan 그래픽 파이프라인 |
| `LotRenderer` | `lot_renderer` | 스왑체인 렌더 루프, 커맨드 버퍼 |
| `LotDevice` | `lot_device` | Vulkan 인스턴스/디바이스/커맨드풀 |
| `LotBuffer` | `lot_buffer` | Vulkan 버퍼 RAII 래퍼 |
| `LotDescriptors` | `lot_descriptors` | DescriptorSetLayout / Pool / Writer |
| `SimpleRenderSystem` | `simple_render_system` | 3D 렌더 + 스텐실 외곽선 (2패스) |
| `PointLightSystem` | `point_light_system` | 포인트 라이트 빌보드 |
| `ObjectSelectionManager` | `object_selection_manager` | 레이캐스팅 (AABB + Möller-Trumbore) |
| `SketchManager` | `sketch_manager` | 3단계 박스 스케치 |
| `TransformTool` | `transform_tool` | 이동/회전/축척 (2클릭 + 숫자 입력) |
| `MaterialManager` | `material_manager` | textures/ 폴더 자동 스캔 & 적용 |
| `KeyboardMoveCtrl` | `keyboard_move_ctrl` | 키보드/마우스/스크롤 입력 처리 |

---

## [Claude Code 전용] 렌더링 구조

### Descriptor Set 구조
```
set=0 (GlobalUbo)  : 투영행렬, 뷰행렬, 역뷰행렬, 조명 배열, lightingEnabled
set=1 (Texture)    : 오브젝트별 텍스처 sampler2D
PushConstants      : modelMatrix, normalMatrix, color, isSelected, hasTexture, textureScale
```

### 렌더 순서 (매 프레임)
```
1. simpleRenderSystem->renderGameObjects()   // 3D 오브젝트 + stencil=1 마킹
2. simpleRenderSystem->renderHighlights()    // 선택 외곽선 (stencil≠1 영역)
3. sketchManager 프리뷰                     // 스케치 활성 시에만
4. pointLightSystem->render()               // 포인트 라이트 빌보드
```

### 선택 외곽선 (스텐실 2패스)
```
1패스: 오브젝트 렌더 → isSelected=true면 stencil=1 기록
2패스 (3D):  cullMode=NONE, 법선 방향 확장, stencil≠1만 노란색 출력
2패스 (평면): cullMode=NONE, 중심 기준 방사형 확장, stencil≠1만 노란색 출력
```

### isSelected 셰이더 플래그
```
0 = 일반 렌더링
1 = 선택됨 (일반 렌더 + stencil=1 기록)
2 = 3D 오브젝트 아웃라인 패스 → 노란색 고정 출력
3 = 평면 오브젝트 아웃라인 패스 → 노란색 고정 출력
```

---

## [Claude Code 전용] 빌드 & 실행

```bash
# 빌드
cmake -B build
cmake --build build

# macOS / Linux
cd build
./run_vulkan.sh      # Validation Layer 포함 (권장)
./run_simple.sh      # Validation Layer 제외

# Windows
cd build\Debug
.\VulkanApp.exe
```

---

## [Claude.ai 전용] 설계 컨텍스트

> Claude.ai와 설계/구조를 논의할 때 참조하는 섹션
> Claude Code는 이 섹션을 건너뛰어도 됨

### 현재까지의 설계 결정과 이유

| 결정 | 이유 |
|------|------|
| 쿼터니언 회전 | Euler 짐벌락 완전 제거, 무제한 360도 회전 |
| Y-up 좌표계 유지 | Vulkan 표준, 추후 Z-up 전환 예정 |
| Push Constants (per-object) | 오브젝트별 행렬 전송, 빠른 업데이트 |
| GlobalUbo | 조명/카메라 데이터 전체 공유, 프레임별 1회 업데이트 |
| 스텐실 2패스 외곽선 | 재질이 보이면서 외곽선만 표시, 전문 CAD 스타일 |
| AABB + 삼각형 교차 혼용 | 단순 오브젝트는 AABB, 복잡 메시는 Möller-Trumbore |
| 스케치 평면 3종 (XY/YZ/XZ) | CAD 뷰 타입(Front/Right/Top)과 자동 연동 |

### 알려진 한계 & 기술 부채

- **Y-up 좌표계**: CAD 표준(Z-up)과 달라 스케치/뷰 계산이 직관적이지 않음
- **Descriptor Pool 고정 크기**: `MAX_TEXTURE_DESCRIPTORS=200` 하드코딩
- **스케치 도구**: 박스만 구현, 선/원/호 미구현
- **Transform Gizmo**: 화면 위 3축 핸들 미구현, 현재 키보드(T/R/S)로만 조작
- **macOS**: MoltenVK 색공간 차이 → 프래그먼트 셰이더에서 명시적 정규화 필요

---

## [Claude Code 전용] 대용량 씬 처리 원칙

> 오브젝트 수가 수천~수만 개로 늘어날 것을 항상 전제하고 설계할 것

### 필수 원칙

- **매 프레임 전체 순회 금지**: O(n) 이상의 작업은 반드시 공간 분할 또는 컬링으로 범위 축소
- **2단계 컬링 패턴** (모든 공간 질의에 적용):
  ```
  1단계 Broad Phase  : AABB 중심을 스크린에 투영 → 관심 반경 이외 조기 탈락
  2단계 Narrow Phase : 통과한 소수 오브젝트만 세부 계산
  ```
- **스냅/선택/레이캐스트** 등 마우스 기반 질의는 반드시 Broad Phase 먼저
- **공간 자료구조** (오브젝트 수 > 1000 예상 시 도입):
  - Octree 또는 BVH (Bounding Volume Hierarchy) 권장
  - Grid 방식은 오브젝트 크기가 균일할 때만 적합

### 현재 구현별 성능 상태

| 시스템 | 현재 방식 | 대용량 대비 |
|--------|-----------|------------|
| 오브젝트 선택 (레이캐스트) | 전체 순회 O(n) | ⚠️ Broad Phase 필요 |
| 렌더링 | 전체 순회 O(n) | ⚠️ Frustum Culling 필요 |
| OSnapManager (예정) | 2단계 컬링 설계 | ✅ Broad Phase 포함 |

### OSnapManager 설계 원칙 (구현 예정)

```
Broad Phase  : 오브젝트 AABB 중심 → 스크린 투영 → 마우스 100px 이상이면 Skip
Narrow Phase : 통과 오브젝트의 27개 후보만 계산
               (Center×1, Endpoint×8, EdgeMid×12, FaceCenter×6)
우선순위     : Center > FaceCenter > Endpoint > Midpoint
```

### 참고 레퍼런스
- littleVulkanEngine: https://github.com/blurrypiano/littleVulkanEngine
- AutoCAD ObjectARX (CAD 아키텍처 패턴)
- CC0 Textures / Poly Haven (텍스처 소스)

---

## 브랜치 히스토리

| 브랜치 | 주요 구현 |
|--------|-----------|
| `26_Move_Rotate_Scale` | TransformTool — 이동(T)/회전(R)/축척(S), 2클릭, 숫자 입력, ESC 복원, 선택 외곽선 개선, OutlineType 파이프라인 통합 |
| `25_Material_Mapping` | STB Image 텍스처, MaterialManager, 스텐실 외곽선, AABB 수정 |

---

## 단축키 전체 목록

| 키 | 기능 |
|----|------|
| `C` | CAD / FPS 모드 전환 |
| `O` | Orthographic / Perspective 전환 |
| `1/2/3/4` | 정면도 / 평면도 / 우측면도 / Isometric |
| `B` | 박스 스케치 시작 |
| `T` | 이동 도구 |
| `R` | 회전 도구 |
| `S` | 축척 도구 |
| `M` | 선택 오브젝트 텍스처 순환 적용 |
| `U` | 선택 오브젝트 텍스처 제거 |
| `[` / `]` | 텍스처 스케일 조절 |
| `G` | 조명 On/Off |
| `N` | 랜덤 큐브 생성 |
| `Delete` | 선택 오브젝트 삭제 |
| `ESC` | 도구 취소 / 선택 해제 |
| 우클릭 드래그 | 카메라 궤도 회전 |
| 중간 클릭 드래그 | 카메라 Pan |
| 마우스 휠 | 줌 (마우스 커서 중심) |
| `Ctrl + 좌클릭` | 다중 선택 |

---

## TODO

### 높은 우선순위
- [ ] **ImGui 통합** — 스냅 심볼(□△○⊕) 오버레이, 속성 패널 기반
- [ ] **OSnapManager** — Center/Endpoint/Midpoint/FaceCenter 스냅 (2단계 컬링, ImGui 심볼)
- [ ] **좌표계 Z-up 전환** (CAD 표준)
  - 수정 파일: `lot_camera.h/cpp`, `sketch_manager.cpp`, `lot_game_object.h`, `first_app.cpp`
  - 핵심: `up = (0,0,1)`, 중력 방향 = `-Z`
- [ ] **Transform Gizmo** — 화면 위 X/Y/Z 축 핸들 (드래그로 직접 조작)

### 중간 우선순위
- [ ] **Dear ImGui** — 오브젝트 속성 패널, 재질 목록, 씬 트리
- [ ] **추가 스케치 도구** — 선, 폴리라인, 원, 호

### 장기 목표
- [ ] **WebGPU / Emscripten 포팅** — 브라우저 실행, 별도 레포 또는 조건부 컴파일

---

## [공통] DLL API 설계 원칙

> 새 기능을 개발할 때마다 반드시 함께 고려해야 할 규칙
> Claude Code와 Claude.ai 둘 다 이 섹션을 참조

### 아키텍처 구조

```
VulkanCAD_Core.dll   ← CAD 엔진 전체 (FirstApp, 렌더링, 시스템)
VulkanCAD.exe        ← 얇은 런처 (DLL 로드 + API 호출만)
plugins/
  ArchBuilder.dll    ← 기능 확장 플러그인 (IPlugin 구현)
  WallBuilder.dll
  ...
```

### ⚠️ 현재 파일 존재 여부

| 파일 | 상태 | 설명 |
|------|------|------|
| `lot_plugin_sdk.h` | ❌ 미생성 | IEngineAPI, IPlugin 인터페이스 — 설계 완료, 파일 아직 없음 |
| `lot_plugin_manager.h` | ❌ 미생성 | DLL 로더 — 설계 완료, 파일 아직 없음 |
| `VulkanCAD_API.h` | ❌ 미생성 | C API 선언 — 설계 완료, 파일 아직 없음 |
| `VulkanCAD_API.cpp` | ❌ 미생성 | C API 구현 — 설계 완료, 파일 아직 없음 |

> **Claude Code 주의**: 위 파일들은 아직 존재하지 않음.
> 코드 작성 전 반드시 새로 생성해야 하며, 기존 파일 수정으로 착각하지 말 것.

---

### 새 기능 개발 시 체크리스트

기능을 하나 추가할 때마다 아래 3단계를 함께 진행한다.

**1단계 — 내부 구현** (현재 가능)
- `lot_*.h/cpp` 또는 `*_manager.h/cpp` 에 기능 구현 (기존 패턴 유지)

**2단계 — IEngineAPI 노출** (lot_plugin_sdk.h 생성 후 가능)
- `lot_plugin_sdk.h` 의 `IEngineAPI` 에 순수 가상 함수 추가
- `FirstApp` 에서 해당 함수 구현

**3단계 — C API 노출** (VulkanCAD_API.h 생성 후 가능)
- `VulkanCAD_API.h` 에 `extern "C"` 함수 추가
- `VulkanCAD_API.cpp` 에서 `g_app->` 로 위임하는 래퍼 구현

### C API 네이밍 규칙

```cpp
// 접두사: CAD_
// 형식:   CAD_[동사][대상]
CAD_CreateBox(...)          // 생성
CAD_DeleteObject(...)       // 삭제
CAD_SetPosition(...)        // 설정
CAD_GetPosition(...)        // 조회
CAD_RegisterCommand(...)    // 등록
CAD_SetOnObjectCreated(...) // 콜백
```

### C API 작성 규칙

```cpp
// ✅ 반드시 extern "C" 블록 안에 선언 (C#/Python P/Invoke 호환)
extern "C" {
    CAD_API uint32_t CAD_CreateBox(float x, float y, float z,
                                    float sx, float sy, float sz);
}

// ✅ 포인터 반환 금지 — ID(uint32_t) 또는 값 타입만 반환
// ✅ glm 타입 노출 금지 — float 개별 파라미터로 분해
// ✅ std::string 노출 금지 — const char* 사용
// ✅ 함수 실패 시 반환값: bool false / uint32_t 0 / int -1

// ❌ 이렇게 하지 말 것
CAD_API glm::vec3 CAD_GetPosition(uint32_t id);   // glm 노출
CAD_API void* CAD_GetObject(uint32_t id);          // 포인터 반환
CAD_API std::string CAD_GetName(uint32_t id);      // std 타입 노출
```

### 콜백 패턴 (엔진 → 외부 알림)

```cpp
// VulkanCAD_API.h 에 추가
CAD_API void CAD_SetOn[이벤트명](void(*cb)([파라미터]));

// 예시
CAD_API void CAD_SetOnObjectCreated (void(*cb)(uint32_t id));
CAD_API void CAD_SetOnObjectDeleted (void(*cb)(uint32_t id));
CAD_API void CAD_SetOnObjectSelected(void(*cb)(uint32_t id));
CAD_API void CAD_SetOnFrameEnd      (void(*cb)());
```

### 기능별 API 노출 현황

> 현재 `lot_plugin_sdk.h`, `VulkanCAD_API.h` 파일이 없으므로 전부 미노출 상태

| 기능 | 내부 구현 | IEngineAPI | C API | 비고 |
|------|-----------|-----------|-------|------|
| 오브젝트 생성/삭제 | ✅ | ❌ | ❌ | FirstApp 내부에만 존재 |
| 트랜스폼 | ✅ | ❌ | ❌ | FirstApp 내부에만 존재 |
| 선택 | ✅ | ❌ | ❌ | FirstApp 내부에만 존재 |
| 명령어 등록 | ❌ | ❌ | ❌ | 미구현 |
| 텍스처/재질 | ✅ | ❌ | ❌ | MaterialManager 내부에만 존재 |
| 카메라 제어 | ✅ | ❌ | ❌ | LotCamera 내부에만 존재 |
| 뷰 전환 (Top/Front 등) | ✅ | ❌ | ❌ | FirstApp 내부에만 존재 |
| 스케치 도구 | ✅ | ❌ | ❌ | SketchManager 내부에만 존재 |
| 조명 | ✅ | ❌ | ❌ | PointLightSystem 내부에만 존재 |

> ✅ 구현됨 / ❌ 미노출 (파일 생성 후 순차적으로 추가 예정)

### C# 연동 예시 (P/Invoke)

```csharp
// C++ DLL을 C#에서 호출하는 방법
// → VulkanCAD_API.h 의 extern "C" 함수를 그대로 사용

using System.Runtime.InteropServices;

public static class CadApi {
    const string DLL = "VulkanCAD_Core.dll";

    [DllImport(DLL)] public static extern bool     CAD_Init(int w, int h, string title);
    [DllImport(DLL)] public static extern void     CAD_Run();
    [DllImport(DLL)] public static extern void     CAD_Shutdown();
    [DllImport(DLL)] public static extern uint     CAD_CreateBox(
                                                     float x, float y, float z,
                                                     float sx, float sy, float sz);
    [DllImport(DLL)] public static extern void     CAD_SetPosition(uint id, float x, float y, float z);
    [DllImport(DLL)] public static extern void     CAD_GetPosition(uint id,
                                                     out float x, out float y, out float z);
    [DllImport(DLL)] public static extern void     CAD_SelectObject(uint id, bool additive);
    [DllImport(DLL)] public static extern bool     CAD_ExecuteCommand(string name);
}

// 사용
CadApi.CAD_Init(1280, 720, "VulkanCAD");
uint id = CadApi.CAD_CreateBox(0, 0, 0, 1, 1, 1);
CadApi.CAD_SetPosition(id, 5, 0, 3);
```
