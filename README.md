# Vulkan - CAD VIEW MODE 

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
│   │   └── Include/
│   │       └── tinyobjloader/
│   │           └── tiny_obj_loader.h
│   ├── Linux/                      # Linux용 SDK
│   │   └── glfw/
│   │   └── include/
│   │       └── tinyobjloader/
│   │           └── tiny_obj_loader.h
│   └── Apple/                      # macOS용 SDK
│       └── glfw/
│       └── include/
│           └── tinyobjloader/
│               └── tiny_obj_loader.h
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

> **Note**: VulkanSdk는 프로젝트 폴더와 같은 레벨에 위치하며, CMakeLists.txt에서 `../VulkanSdk/Win(Linux/Apple)/` 경로로 OS별 참조됩니다.

> **추가**: tiny_obj_loader.h 를 위의 디렉토리를 보고 OS에 따라 복사해주세요.

- 수정사항  
  - 선택 오류 수정 : 모델의 실제 버텍스로부터 바운딩박스 계산

- 실행결과
  - 좌표계를 오일러에서 쿼터시안으로 변경
  - 선택 시 노란색으로 객체 색상 변경
  - 마우스 휠 버튼 : 마우스 커서 중심으로 확대/축소
  - 마우스 좌측 버튼 : 선택
  - 마우스 우측 버튼 : 회전
  - N : 랜덤생성
  - Delete : 선택된 객체 삭제
  - G : 조명 OnOff
  - O : 뷰모드(Orthographic/Perspective) 
  - C : CAD / FPS 모드
  - 1/2/3/4 : 평면도 / 정면도 / 우측면도 / ISO
  
  ---  
  - 윈도우  
      - 실행방법    
        <kbd>PS D:\programming\vulkan\3dEngine></kbd> cd .\build\Debug\  
        <kbd>PS D:\programming\vulkan\3dEngine\build\Debug></kbd> .\VulkanApp.exe
          
        https://github.com/user-attachments/assets/6a19f5d0-6cc6-4711-a78e-fc0f556d5098                    
    
  - MacOS
      - 실행방법  
        <kbd>test@MacBookPro build % </kbd> ./run_vulkan.sh  
      - 검증 레이어(validation layers) 오류시 아래의 해결방법 수행  
        libc++abi: terminating due to uncaught exception of type std::runtime_error: validation layers requested, but not available!  
      - 해결방법 (환경변수 설정 후 실행)  
        <kbd>test@MacBookPro build % </kbd> export VK_LAYER_PATH="/Users/lot700/Desktop/mac_vk/vk_cmake/VulkanSdk/Apple/share/vulkan/explicit_layer.d"          
        <kbd>test@MacBookPro build % </kbd> export VK_ICD_FILENAMES="/Users/lot700/Desktop/mac_vk/vk_cmake/VulkanSdk/Apple/share/vulkan/icd.d/MoltenVK_icd.json"  
        <kbd>test@MacBookPro build % </kbd> ./VulkanApp 

        https://github.com/user-attachments/assets/65f2c866-5f44-4965-b841-53f78207e85f  
        
  - Linux(Ubuntu)
      - 실행방법  
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> ./run_vulkan.sh   
      - 검증 레이어(validation layers) 오류시 해결방법 수행  
        terminate called after throwing an instance of 'std::runtime_error' what():  validation layers requested, but not available!  
      - 해결방법 (환경변수 설정 후 실행)   
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> export VK_LAYER_PATH="/home/lot700/Vulkan/VulkanSdk/Linux/share/vulkan/explicit_layer.d"          
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> export LD_LIBRARY_PATH="/home/lot700/Vulkan/VulkanSdk/Linux/lib:$LD_LIBRARY_PATH"  
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> export XDG_SESSION_TYPE=x11  // x11 창 선택  
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> ./VulkanApp  

        https://github.com/user-attachments/assets/2c9c8b2a-79ff-4b04-80f7-ea27ab1842c2    
        

