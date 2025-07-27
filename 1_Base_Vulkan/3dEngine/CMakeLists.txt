# CMake 최소 버전 지정
cmake_minimum_required(VERSION 3.10)

# 프로젝트 이름 설정
project(3DEngine LANGUAGES CXX)

# C++ 표준 버전 지정 (Vulkan은 C++17 이상 권장)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#---

### 1. Vulkan SDK 경로 설정 (상대 경로 또는 고정 경로)

# 프로젝트 루트에서 Vulkan SDK의 상대 경로를 지정합니다.
# 이 경로는 프로젝트의 CMakeLists.txt 파일이 있는 위치를 기준으로 합니다.
# 예시: 프로젝트 폴더와 같은 상위 폴더에 SDK가 있다면 "../VulkanSDK/1.x.xxx.x"
# 예시: C 드라이브 루트 등 특정 고정 경로에 있다면 "C:/VulkanSDK/1.x.xxx.x"
# 예시: macOS Homebrew 설치 시 "/opt/homebrew" 또는 "/usr/local"
# 예시: Linux /usr/share/vulkan 또는 /opt/vulkan-sdk 등
# 이 부분을 개발 환경에 맞춰 정확히 수정해야 합니다.

# 현재 예시에서는 '개발자님이 직접 SDK 경로를 여기에 설정해야 합니다.'
# 이 경로는 절대 경로여도 무방하며, 개발 환경에 따라 가장 적합한 방식을 사용합니다.
# 일반적으로는 환경 변수 (이전 답변 방식)가 더 유연합니다.
# 하지만 고정된 개발 머신에서만 사용한다면 이 방식도 가능합니다.

# !!! 중요: 이 변수를 본인의 Vulkan SDK 설치 경로에 맞게 수정하세요 !!!
if(WIN32)
    # Windows 예시 (C: 드라이브 루트에 설치된 경우)
    set(VULKAN_SDK_PATH "../VulkanSdk/Win") # <-- 본인 SDK 버전에 맞게 수정
    set(VULKAN_INCLUDE "Include")
elseif(APPLE)
    # macOS Homebrew로 설치 시 (일반적인 설치 경로)
    set(VULKAN_SDK_PATH "../VulkanSdk/Apple") # Homebrew prefix
    set(VULKAN_INCLUDE "include")
    # 또는 수동 설치 시:
    # set(VULKAN_SDK_PATH "/Users/youruser/VulkanSDK/1.3.283.0") # <-- 본인 SDK 버전에 맞게 수정
    if(EXISTS "${VULKAN_SDK_PATH}/MoltenVK")
        set(MOLTENVK_PATH "${VULKAN_SDK_PATH}/MoltenVK")
    else()
        set(MOLTENVK_PATH "${VULKAN_SDK_PATH}")
    endif()
elseif(UNIX) # Linux
    # Linux 예시 (배포판 패키지 또는 수동 설치 경로)
    set(VULKAN_SDK_PATH "../VulkanSdk/Linux") # 시스템 설치 경로
    set(VULKAN_INCLUDE "include")
    # 또는 수동 설치 시:
    # set(VULKAN_SDK_PATH "/home/youruser/VulkanSDK/1.3.283.0") # <-- 본인 SDK 버전에 맞게 수정
endif()

# 설정된 경로가 유효한지 간단히 확인
if(NOT EXISTS "${VULKAN_SDK_PATH}/${VULKAN_INCLUDE}/vulkan")
    message(FATAL_ERROR "Vulkan SDK not found at specified path: ${VULKAN_SDK_PATH}. Please verify VULKAN_SDK_PATH in CMakeLists.txt.")
else()
    message(STATUS "Vulkan SDK Path set to: ${VULKAN_SDK_PATH}")
endif()

# Vulkan 헤더 및 라이브러리 경로 추가
include_directories(
    ${VULKAN_SDK_PATH}/Include
)

# Linux/Windows와 macOS의 라이브러리 구조 차이 처리
if(APPLE)
    # MoltenVK 라이브러리 경로 (macOS)
    # Homebrew 설치 시 MoltenVK.dylib는 /opt/homebrew/lib 에 위치
    link_directories(
        ${VULKAN_SDK_PATH}/lib # Homebrew의 경우
        # 또는 수동 설치 SDK 경로:
        # ${VULKAN_SDK_PATH}/macOS/lib
    )
    # MoltenVK Framework를 직접 추가하는 방법 (CMake find_package를 사용하지 않는 경우)
    # target_link_libraries(VulkanApp PRIVATE "-framework MoltenVK")
elseif(WIN32)
    link_directories(
        ${VULKAN_SDK_PATH}/Lib
    )
elseif(UNIX) # Linux
    link_directories(
        ${VULKAN_SDK_PATH}/Lib # SDK를 수동 설치한 경우
        /usr/lib # 시스템 설치 라이브러리 (apt, dnf 등으로 설치 시)
        /usr/local/lib # 시스템 설치 라이브러리
    )
endif()

#---

### 2. 대상 실행 파일 추가

# 여기에 Vulkan 애플리케이션의 소스 파일들을 나열합니다.
# 예: main.cpp, VulkanApp.cpp, VulkanApp.h 등
# 현재 디렉토리의 모든 .cpp 파일을 자동으로 찾습니다.
# 이 방법은 소스 파일이 많을 때 유용합니다.
file(GLOB SOURCE_FILES "*.cpp")

add_executable(VulkanApp
    ${SOURCE_FILES}    
    #main.cpp
    # src/your_vulkan_file.cpp
    # src/your_another_vulkan_file.cpp
)

#---

### 3. Vulkan 라이브러리 연결

# 운영체제에 따라 라이브러리 이름이 다릅니다.
if(WIN32)
    target_link_libraries(VulkanApp PRIVATE vulkan-1)
    # LNK4098 경고 해결: MSVCRT와 충돌하는 기본 라이브러리 무시
    # 일반적으로 LIBCMT(정적 릴리스) 또는 LIBCMTD(정적 디버그)가 MSVCRT와 충돌합니다.
    target_link_options(VulkanApp PRIVATE "/NODEFAULTLIB:LIBCMT" "/NODEFAULTLIB:LIBCMTD")
elseif(APPLE)
    # macOS에서는 MoltenVK 라이브러리를 링크합니다.
    # MoltenVK가 .dylib 파일로 제공되는 경우:
    target_link_libraries(VulkanApp PRIVATE MoltenVK)
    # 또는 MoltenVK가 Framework 형태로 제공되는 경우:
    # target_link_libraries(VulkanApp PRIVATE "-framework MoltenVK")
    # 그리고 Cocoa, QuartzCore, Metal도 필요할 수 있습니다.
    target_link_libraries(VulkanApp PRIVATE "-framework Cocoa" "-framework QuartzCore" "-framework Metal")
elseif(UNIX) # Linux
    target_link_libraries(VulkanApp PRIVATE vulkan)
endif()

#---

### 4. 선택 사항: 서드파티 라이브러리 연동 (예: GLFW, SDL2, GLM)

# Vulkan 튜토리얼에서는 보통 창 생성을 위해 SDL2나 GLFW를 사용합니다.
# 이 부분은 find_package를 사용하여 크로스 플랫폼으로 라이브러리를 찾는 것이 가장 좋습니다.

# 예시: GLFW 연동
if(WIN32)
    # Windows용 GLFW 경로
    # 예: D:/programming/vulkan/VulkanSdk/glfw-3.4.bin.WIN64 (실제 경로에 맞게 수정)
    set(GLFW_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../VulkanSdk/Win/glfw-3.4.bin.WIN64")
    set(GLFW_LIB_DIR "lib-vc2022") # Windows MSVC용 라이브러리 폴더
    set(GLFW_STATIC_LIB_NAME "glfw3") # Release 모드용 정적 라이브러리 이름
    set(GLFW_DEBUG_LIB_NAME "glfw3d") # Debug 모드용 정적 라이브러리 이름
    # MSVC 라이브러리는 .lib 확장자가 필요함
    set(GLFW_STATIC_LIB_FULL_NAME "${GLFW_STATIC_LIB_NAME}.lib")
    set(GLFW_DEBUG_LIB_FULL_NAME "${GLFW_DEBUG_LIB_NAME}.lib")
elseif(APPLE)
    # macOS용 GLFW 경로
    # 예: /opt/homebrew/Cellar/glfw/3.4 (Homebrew 설치 시) 또는 수동 설치 경로
    set(GLFW_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../VulkanSdk/Apple/GLFW") # 또는 당신의 macOS GLFW 경로
    set(GLFW_LIB_DIR "lib") # macOS 라이브러리 폴더 (일반적으로)
    set(GLFW_STATIC_LIB_NAME "glfw3") # macOS 정적 라이브러리 이름 (libglfw3.a)
    set(GLFW_DEBUG_LIB_NAME "glfw3") # macOS는 보통 Debug/Release 구분 없이 동일한 정적 라이브러리 사용
    # macOS 라이브러리는 .a 확장자가 필요함
    set(GLFW_STATIC_LIB_FULL_NAME "lib${GLFW_STATIC_LIB_NAME}.a")
    set(GLFW_DEBUG_LIB_FULL_NAME "lib${GLFW_DEBUG_LIB_NAME}.a")
elseif(UNIX) # Linux
    # Linux용 GLFW 경로
    # 예: /usr/local/lib (시스템 설치 시) 또는 수동 설치 경로
    set(GLFW_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../VulkanSdk/Linux/glfw") # 또는 당신의 Linux GLFW 경로
    set(GLFW_LIB_DIR "lib") # Linux 라이브러리 폴더 (일반적으로)
    set(GLFW_STATIC_LIB_NAME "glfw3") # Linux 정적 라이브러리 이름 (libglfw.a)
    set(GLFW_DEBUG_LIB_NAME "glfw3") # Linux는 보통 Debug/Release 구분 없이 동일한 정적 라이브러리 사용
    # Linux 라이브러리는 .a 확장자가 필요함
    set(GLFW_STATIC_LIB_FULL_NAME "lib${GLFW_STATIC_LIB_NAME}.a")
    set(GLFW_DEBUG_LIB_FULL_NAME "lib${GLFW_DEBUG_LIB_NAME}.a")
endif()

# GLFW 경로가 설정되었는지 확인
if(NOT DEFINED GLFW_ROOT_DIR)
    message(FATAL_ERROR "Unsupported operating system for GLFW manual setup. Please add your OS to GLFW_ROOT_DIR configuration.")
endif()

# 정확한 경로 확인을 위해 `include` 폴더와 라이브러리 파일이 실제로 존재하는지 확인합니다.
# 주의: 이 존재 여부 검사는 실제 GLFW 라이브러리 파일이 해당 경로에 있는지 확인하는 것입니다.
if(EXISTS "${GLFW_ROOT_DIR}/include" AND EXISTS "${GLFW_ROOT_DIR}/${GLFW_LIB_DIR}/${GLFW_STATIC_LIB_FULL_NAME}")
    message(STATUS "Manually found GLFW at: ${GLFW_ROOT_DIR}")

    # GLFW 헤더 파일 경로 추가
    include_directories(${GLFW_ROOT_DIR}/include)

    # OS별 라이브러리 링크
    if(WIN32)
        # Windows MSVC용 라이브러리 명시적 링크
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_link_libraries(VulkanApp PRIVATE "${GLFW_ROOT_DIR}/${GLFW_LIB_DIR}/${GLFW_DEBUG_LIB_FULL_NAME}")
        else()
            target_link_libraries(VulkanApp PRIVATE "${GLFW_ROOT_DIR}/${GLFW_LIB_DIR}/${GLFW_STATIC_LIB_FULL_NAME}")
        endif()
        # GLFW는 Windows에서 user32와 gdi32에 의존합니다. 명시적으로 링크합니다.
        target_link_libraries(VulkanApp PRIVATE user32 gdi32)
    elseif(APPLE)
        # macOS용 GLFW 라이브러리 링크
        target_link_libraries(VulkanApp PRIVATE "${GLFW_ROOT_DIR}/${GLFW_LIB_DIR}/${GLFW_STATIC_LIB_FULL_NAME}")
        # macOS는 GLFW가 Cocoa, IOKit, CoreVideo, OpenGL 프레임워크에 의존합니다.
        target_link_libraries(VulkanApp PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo" "-framework OpenGL")
    elseif(UNIX) # Linux
        # Linux용 GLFW 라이브러리 링크
        target_link_libraries(VulkanApp PRIVATE "${GLFW_ROOT_DIR}/${GLFW_LIB_DIR}/${GLFW_STATIC_LIB_FULL_NAME}")
        # Linux는 X11, Xrandr, Xxf86vm, Xinerama, Xi, Xcursor, GL, pthread, m, dl 라이브러리에 의존합니다.
        target_link_libraries(VulkanApp PRIVATE X11 Xrandr Xxf86vm Xinerama Xi Xcursor GL pthread m dl)
    endif()
else()
    # 오류 메시지 개선: 어떤 경로가 문제인지 좀 더 구체적으로 명시
    message(FATAL_ERROR "GLFW (manual setup) not found for this OS.
    Please verify GLFW_ROOT_DIR (${GLFW_ROOT_DIR}), GLFW_LIB_DIR (${GLFW_ROOT_DIR}/${GLFW_LIB_DIR}),
    and ensure 'include' folder and '${GLFW_STATIC_LIB_FULL_NAME}' exist within them.")
endif()

# 예시: GLM 연동 (수학 라이브러리)
# GLM은 헤더 전용 라이브러리이므로 간단히 include_directories만 추가하면 됩니다.
# 여기서는 프로젝트 내부에 GLM을 포함하거나, 미리 정해진 상대 경로에 GLM이 있다고 가정합니다.
# 예를 들어, 프로젝트 루트에 'external/glm' 폴더에 GLM이 있다고 가정하면:

if(WIN32)
    set(GLM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../VulkanSdk/Win/Include/glm")
    include_directories(${GLM_INCLUDE_DIR})
elseif(APPLE) #APPLE
    set(GLM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../VulkanSdk/Apple/include/glm")
    include_directories(${GLM_INCLUDE_DIR})
elseif(UNIX) # Linux
    set(GLM_INCLUDE_DIR "../VulkanSdk/Linux/include/glm")
    include_directories(${GLM_INCLUDE_DIR})
else()
     message(WARNING "GLM include directory not found at ${GLM_INCLUDE_DIR}. Please set GLM_INCLUDE_DIR correctly or install GLM.")
endif()


#---

### 5. 빌드 후 작업 (선택 사항)

# 빌드 시 필요한 파일 (예: 셰이더) 복사
file(GLOB SHADERS_GLSL "shaders/*.vert" "shaders/*.frag" "shaders/*.comp")
if(SHADERS_GLSL)
    add_custom_command(
        TARGET VulkanApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADERS_GLSL} "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMENT "Copying shaders to build directory"
    )
endif()

# SPV 셰이더 컴파일 자동화
# glslangValidator의 경로도 VULKAN_SDK_PATH를 기준으로 찾습니다.
if(NOT DEFINED VULKAN_SDK_PATH) # VULKAN_SDK_PATH가 설정되지 않았다면 (위에서 FATAL_ERROR로 막았겠지만 안전장치)
    message(WARNING "VULKAN_SDK_PATH is not set. Cannot find glslangValidator for automatic shader compilation.")
else()
    find_program(GLSLANG_VALIDATOR NAMES glslangValidator
                 PATHS "${VULKAN_SDK_PATH}/Bin" # Windows
                       "${VULKAN_SDK_PATH}/bin" # Linux
                       "${VULKAN_SDK_PATH}/macOS/bin" # macOS
                 NO_DEFAULT_PATH)

    if(GLSLANG_VALIDATOR)
        message(STATUS "Found glslangValidator: ${GLSLANG_VALIDATOR}")
        file(GLOB_RECURSE GLSL_SHADERS "shaders/*.vert" "shaders/*.frag" "shaders/*.comp")
        if(GLSL_SHADERS)
            add_custom_target(CompileShaders ALL)
            foreach(SHADER_FILE ${GLSL_SHADERS})
                get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
                add_custom_command(
                    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv"
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
                    COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_FILE} -o "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv"
                    DEPENDS ${SHADER_FILE}
                    COMMENT "Compiling ${SHADER_FILE} to SPIR-V"
                )
                add_dependencies(CompileShaders "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
            endforeach()
            add_dependencies(VulkanApp CompileShaders)
        endif()
    else()
        message(WARNING "glslangValidator not found. SPIR-V shaders will not be compiled automatically.")
        message(WARNING "Please ensure glslangValidator is in a Bin directory within your VULKAN_SDK_PATH.")
    endif()
endif()