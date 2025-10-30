# Vulkan(Obj 파일 로드) - TinyObjLoder.h 로 obj 파일 로드하기

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
│   ├── models/                    # .obj 파일들
│   │   ├── smooth_vase.obj
│   │   ├── cube.obj
│   │   ├── colored_cube.obj
│   │   ├── flat_vase.obj
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
| `VulkanSdk/Apple,Win,Linux/tinyobjloader/.h` | tiny_obj_loader.h 헤더파일 |

> **참고**: VulkanSdk는 프로젝트 폴더와 같은 레벨에 위치하며, CMakeLists.txt에서 `../VulkanSdk/Win(Linux/Apple)/` 경로로 OS별 참조됩니다.

- 실행결과
  - 좌표계를 오일러에서 쿼터시안으로 변경
  - 선택 시 노란색으로 객체 색상 변경
  - 마우스 휠 버튼 : 줌 확대/축소
  - 마우스 좌측 버튼 : 선택
  - 마우스 우측 버튼 : 회전
  - N : 랜덤생성
  - Delete : 선택된 객체 삭제
  - obj 확장자 파일 로드 
  ---  
  - 윈도우  
      - 실행방법    
        <kbd>PS D:\programming\vulkan\3dEngine></kbd> cd .\build\Debug\  
        <kbd>PS D:\programming\vulkan\3dEngine\build\Debug></kbd> .\VulkanApp.exe
          
        https://github.com/user-attachments/assets/5af2ab51-bea5-4c01-ab21-e9c39a58c44c                  
    
  - MacOS
      - 실행방법  
        <kbd>test@MacBookPro build % </kbd> ./VulkanApp  
      - 검증 레이어(validation layers) 오류시 아래의 해결방법 수행  
        libc++abi: terminating due to uncaught exception of type std::runtime_error: validation layers requested, but not available!  
      - 해결방법 (환경변수 설정 후 실행)  
        <kbd>test@MacBookPro build % </kbd> export VK_LAYER_PATH="/Users/lot700/Desktop/mac_vk/vk_cmake/VulkanSdk/Apple/share/vulkan/explicit_layer.d"          
        <kbd>test@MacBookPro build % </kbd> export VK_ICD_FILENAMES="/Users/lot700/Desktop/mac_vk/vk_cmake/VulkanSdk/Apple/share/vulkan/icd.d/MoltenVK_icd.json"  
        <kbd>test@MacBookPro build % </kbd> ./VulkanApp 

        https://github.com/user-attachments/assets/c7e9b9e4-c2a6-40f6-aa82-ec89dffecd34    
        
  - Linux(Ubuntu)
      - 실행방법(일부 수정 - vkDeviceWaitIdle(lotDevice.device()); 추가)  
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> ./VulkanApp   
      - 검증 레이어(validation layers) 오류시 해결방법 수행  
        terminate called after throwing an instance of 'std::runtime_error' what():  validation layers requested, but not available!  
      - 해결방법 (환경변수 설정 후 실행)   
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> export VK_LAYER_PATH="/home/lot700/Vulkan/VulkanSdk/Linux/share/vulkan/explicit_layer.d"          
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> export LD_LIBRARY_PATH="/home/lot700/Vulkan/VulkanSdk/Linux/lib:$LD_LIBRARY_PATH"  
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> export XDG_SESSION_TYPE=x11  // x11 창 선택  
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> ./VulkanApp  

        https://github.com/user-attachments/assets/14c7e23d-669c-49de-80d3-1258ea9e697a
        

        
