# Vulkan(ResizeWindow)

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
      - 실행방법(실행 후 윈도우 크기 변경하기)  
        <kbd>PS D:\programming\vulkan\3dEngine></kbd> cd .\build\Debug\  
        <kbd>PS D:\programming\vulkan\3dEngine\build\Debug></kbd> .\VulkanApp.exe
          
        <img width="1238" height="629" alt="Image" src="https://github.com/user-attachments/assets/09aa8d96-39d8-4c42-8002-6e4ec59c2a53" />  
        <img width="1238" height="629" alt="Image" src="https://github.com/user-attachments/assets/54fc1242-8596-4139-bfaa-ba42500d89d3" />
        <img width="1450" height="817" alt="Image" src="https://github.com/user-attachments/assets/e91de7c2-6d35-4955-af69-806263ca3be6" />
          
    
  - MacOS
      - 실행방법
        <kbd>test@MacBookPro build % </kbd> ./VulkanApp  
      - 검증 레이어(validation layers) 오류시 아래의 해결방법 수행  
        libc++abi: terminating due to uncaught exception of type std::runtime_error: validation layers requested, but not available!  
      - 해결방법 (환경변수 설정 후 실행)  
        <kbd>test@MacBookPro build % </kbd> export VK_LAYER_PATH="/Users/lot700/Desktop/mac_vk/vk_cmake/VulkanSdk/Apple/share/vulkan/explicit_layer.d"          
        <kbd>test@MacBookPro build % </kbd> export VK_ICD_FILENAMES="/Users/lot700/Desktop/mac_vk/vk_cmake/VulkanSdk/Apple/share/vulkan/icd.d/MoltenVK_icd.json"  
        <kbd>test@MacBookPro build % </kbd> ./VulkanApp 

        <img width="1052" height="626" alt="Image" src="https://github.com/user-attachments/assets/0aa4fe3f-e04f-4dc3-9abc-5b0810beda36" />
        
        <img width="936" height="609" alt="Image" src="https://github.com/user-attachments/assets/62cb72aa-ccde-4477-a62b-e7e15bfcf139" />  

        <img width="1132" height="727" alt="Image" src="https://github.com/user-attachments/assets/65797a8e-b9b6-40a4-af73-d2239f9759dd" />  

  - Linux(Ubuntu)
      - 검증 레이어(validation layers) 오류시 1번 수행  
        terminate called after throwing an instance of 'std::runtime_error' what():  validation layers requested, but not available!  
      - 실행방법 1 (환경변수 설정 후 실행)   
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> export VK_LAYER_PATH="/home/lot700/Vulkan/VulkanSdk/Linux/share/vulkan/explicit_layer.d"          
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> export LD_LIBRARY_PATH="/home/lot700/Vulkan/VulkanSdk/Linux/lib:$LD_LIBRARY_PATH"  
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> export XDG_SESSION_TYPE=x11  // x11 창 선택  
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> ./VulkanApp  
        
      - 실행방법 2 (바로 실행)  
        <kbd>test@test-IdeaPad-1-15ALC7:~/Vulkan/3dEngine/build$ </kbd> ./VulkanApp

    <img width="1346" height="986" alt="Image" src="https://github.com/user-attachments/assets/7592e9d0-69ba-4adc-bd5b-9ce058da9eec" />
