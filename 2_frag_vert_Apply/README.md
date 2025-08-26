# Vulkan(frag + vert)

Tool : Visual Studio Code
- 파일 구조
  ## 📂 프로젝트 구조

<pre>
📁 개발폴더/
├── Project/                        # 메인 프로젝트 폴더
│   ├── build/                      # 빌드 시 자동 생성
│   │   └── Debug/
│   │       ├── shaders/            # 빌드 시 복사
│   │       │   ├── simple_shader.frag
│   │       │   ├── simple_shader.vert
│   │       │   └── ...
│   │       └── VulkanApp           # 실행파일
│   ├── shaders/                    # 원본 셰이더 파일들
│   │   ├── simple_shader.frag
│   │   ├── simple_shader.vert
│   │   └── ...
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── first_app.cpp
│   ├── first_app.h
│   ├── lot_device.cpp
│   ├── lot_device.h
│   ├── lot_model.cpp
│   ├── lot_model.h
│   ├── lot_pipeline.cpp
│   ├── lot_pipeline.h
│   ├── lot_swap_chain.cpp
│   ├── lot_swap_chain.h
│   ├── lot_window.cpp
│   ├── lot_window.h
│   └── ...
├── VulkanSdk/                      # Vulkan 라이브러리
│   ├── Win/                        # Windows용 SDK
│   │   └── glfw/
│   ├── Linux/                      # Linux용 SDK
│   │   └── glfw/
│   └── Apple/                      # macOS용 SDK
│       └── glfw/
└── ...
</pre>

### 📝 주요 디렉토리 설명

| 경로 | 설명 |
|------|------|
| `Project/` | 메인 프로젝트 소스 코드 |
| `Project/build/` | CMake 빌드 출력 (자동 생성) |
| `Project/shaders/` | 원본 GLSL 셰이더 파일들 |
| `VulkanSdk/Win/` | Windows용 Vulkan SDK 및 라이브러리 |
| `VulkanSdk/Linux/` | Linux용 Vulkan SDK 및 라이브러리 |
| `VulkanSdk/Apple/` | macOS용 Vulkan SDK 및 라이브러리 |
| `lot_*.cpp/h` | Vulkan 엔진 컴포넌트들 |

> **참고**: VulkanSdk는 프로젝트 폴더와 같은 레벨에 위치하며, CMakeLists.txt에서 `../VulkanSdk/Win(Linux/Apple)/` 경로로 OS별 참조됩니다.

- 실행결과
  
  - 윈도우
  
    <img width="1244" height="636" alt="Image" src="https://github.com/user-attachments/assets/b411e6c8-5cb1-4a57-af62-034f12c1dc07" />

  - MacOS
      - 검증 레이어 오류시 1번 수행
        libc++abi: terminating due to uncaught exception of type std::runtime_error: validation layers requested, but not available!  
      - 실행방법 1 (환경변수 설정 후 실행)
        <font color="red">???@MacBookPro build % </font> 
        ???@MacBookPro build % export VK_LAYER_PATH="/Users/lot700/Desktop/mac_vk/vk_cmake/VulkanSdk/Apple/share/vulkan/explicit_layer.d"          
        ???@MacBookPro build % export   VK_ICD_FILENAMES="/Users/lot700/Desktop/mac_vk/vk_cmake/VulkanSdk/Apple/share/vulkan/icd.d/MoltenVK_icd.json"  
        ???@MacBookPro build % ./VulkanApp
        
      - 실행방법 2 (바로 실행)  
        ???@MacBookPro build % ./VulkanApp

      <img width="1127" height="623" alt="Image" src="https://github.com/user-attachments/assets/f8913c8c-836a-43b1-8706-223a60e5b47b" />
