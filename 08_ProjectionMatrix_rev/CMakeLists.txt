# CMake 최소 버전 지정
cmake_minimum_required(VERSION 3.10)

# 프로젝트 이름 설정
project(3DEngine LANGUAGES CXX)

# C++ 표준 버전 지정 (Vulkan은 C++17 이상 권장)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options("/utf-8")
    message(STATUS "MSVC detected: UTF-8 encoding enabled")
endif() 
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

    # 상대 경로를 절대 경로로 변환 (CMake 경로 해석 문제 해결)
    get_filename_component(VULKAN_SDK_PATH_ABSOLUTE "${VULKAN_SDK_PATH}" ABSOLUTE)
    
    set(VULKAN_SDK_PATH "${VULKAN_SDK_PATH_ABSOLUTE}")

    message(STATUS "Vulkan SDK relative path: ${VULKAN_SDK_PATH}")
    message(STATUS "Vulkan SDK absolute path: ${VULKAN_SDK_PATH_ABSOLUTE}")
elseif(APPLE)
    # macOS Homebrew로 설치 시 (일반적인 설치 경로)
    set(VULKAN_SDK_PATH "../VulkanSdk/Apple") # Homebrew prefix
    set(VULKAN_INCLUDE "include")
    # 상대 경로를 절대 경로로 변환 (CMake 경로 해석 문제 해결)
    get_filename_component(VULKAN_SDK_PATH_ABSOLUTE "${VULKAN_SDK_PATH}" ABSOLUTE)
    
    set(VULKAN_SDK_PATH "${VULKAN_SDK_PATH_ABSOLUTE}")

    message(STATUS "Vulkan SDK relative path: ${VULKAN_SDK_PATH}")
    message(STATUS "Vulkan SDK absolute path: ${VULKAN_SDK_PATH_ABSOLUTE}")
    # 또는 수동 설치 시:
    # set(VULKAN_SDK_PATH "/Users/youruser/VulkanSDK/1.3.283.0") # <-- 본인 SDK 버전에 맞게 수정
    if(EXISTS "${VULKAN_SDK_PATH}/MoltenVK")
        set(MOLTENVK_PATH "${VULKAN_SDK_PATH}/MoltenVK")
    else()
        set(MOLTENVK_PATH "${VULKAN_SDK_PATH}")
    endif()

    # *** 새로 추가: Validation Layers 환경 변수 설정 ***
    # VK_LAYER_PATH 설정 (빌드 시점에 환경 변수로 설정)
    set(VK_LAYER_PATH_DIR "${VULKAN_SDK_PATH}/share/vulkan/explicit_layer.d")
    set(VK_ICD_PATH "${VULKAN_SDK_PATH}/share/vulkan/icd.d/MoltenVK_icd.json")
    
    if(EXISTS "${VK_LAYER_PATH_DIR}")
        message(STATUS "Found Validation Layers at: ${VK_LAYER_PATH_DIR}")
    else()
        message(WARNING "Validation Layers not found at: ${VK_LAYER_PATH_DIR}")
    endif()
elseif(UNIX) # Linux
    # Linux 예시 (배포판 패키지 또는 수동 설치 경로)
    set(VULKAN_SDK_PATH "../VulkanSdk/Linux") # 시스템 설치 경로
    set(VULKAN_INCLUDE "include")

    # 상대 경로를 절대 경로로 변환 (CMake 경로 해석 문제 해결)
    get_filename_component(VULKAN_SDK_PATH_ABSOLUTE "${VULKAN_SDK_PATH}" ABSOLUTE)
    
    set(VULKAN_SDK_PATH "${VULKAN_SDK_PATH_ABSOLUTE}")

    message(STATUS "Vulkan SDK relative path: ${VULKAN_SDK_PATH}")
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
    target_link_libraries(VulkanApp PRIVATE vulkan)
    target_link_libraries(VulkanApp PRIVATE "-framework Cocoa" "-framework QuartzCore" "-framework Metal" "-framework IOKit" "-framework CoreVideo") 
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
    set(GLM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../VulkanSdk/Linux/include")
    
    # 경로 확인
    if(EXISTS "${GLM_INCLUDE_DIR}/glm/glm.hpp")
        include_directories(${GLM_INCLUDE_DIR})
        message(STATUS "GLM include path (Linux): ${GLM_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "GLM not found at ${GLM_INCLUDE_DIR}/glm/glm.hpp")
    endif()
else()
     message(WARNING "GLM include directory not found at ${GLM_INCLUDE_DIR}. Please set GLM_INCLUDE_DIR correctly or install GLM.")
endif()


#---

### 5. 빌드 후 작업 (선택 사항)

### 5. 빌드 후 작업 - 셰이더 컴파일 및 복사

### 5. 빌드 후 작업 - 셰이더 컴파일 및 복사 (간단하고 확실한 버전)

# 먼저 셰이더 컴파일
if(NOT DEFINED VULKAN_SDK_PATH)
    message(WARNING "VULKAN_SDK_PATH is not set. Cannot find glslangValidator for automatic shader compilation.")
else()
    # glslangValidator 찾기
    if(WIN32)
        set(GLSLANG_VALIDATOR "${VULKAN_SDK_PATH}/Bin/glslangValidator.exe")
    else()
        find_program(GLSLANG_VALIDATOR NAMES glslangValidator
                     PATHS "${VULKAN_SDK_PATH}/bin" "${VULKAN_SDK_PATH}/macOS/bin"
                     NO_DEFAULT_PATH)
    endif()
    
    if(GLSLANG_VALIDATOR)
        message(STATUS "Found glslangValidator: ${GLSLANG_VALIDATOR}")
        
        # GLSL 셰이더 파일들 찾기
        file(GLOB_RECURSE GLSL_SHADERS "shaders/*.vert" "shaders/*.frag" "shaders/*.comp")
        if(GLSL_SHADERS)
            set(SPV_FILES)
            
            foreach(SHADER_FILE ${GLSL_SHADERS})
                get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
                set(SPV_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_NAME}.spv")
                
                # SPIR-V 컴파일
                add_custom_command(
                    OUTPUT "${SPV_OUTPUT}"
                    COMMAND "${GLSLANG_VALIDATOR}" -V "${SHADER_FILE}" -o "${SPV_OUTPUT}"
                    DEPENDS "${SHADER_FILE}"
                    COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
                    VERBATIM
                )
                
                list(APPEND SPV_FILES "${SPV_OUTPUT}")
            endforeach()

            # 컴파일 타겟
            add_custom_target(CompileShaders ALL DEPENDS ${SPV_FILES})
            add_dependencies(VulkanApp CompileShaders)
            
            list(LENGTH GLSL_SHADERS SHADER_COUNT)
            message(STATUS "Will compile ${SHADER_COUNT} shader files")
        endif()
    else()
        message(WARNING "glslangValidator not found.")
    endif()
endif()

# 이제 확실하게 폴더 생성하고 파일 복사
add_custom_command(TARGET VulkanApp POST_BUILD
    # 1단계: shaders 폴더 강제 생성
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:VulkanApp>/shaders"
    
    # 2단계: 모든 .spv 파일 복사
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders" 
        "$<TARGET_FILE_DIR:VulkanApp>/shaders"
        
    # 3단계: 확인 메시지
    COMMAND ${CMAKE_COMMAND} -E echo "Shaders copied to: $<TARGET_FILE_DIR:VulkanApp>/shaders"
    
    COMMENT "Creating shaders directory and copying all shader files"
    VERBATIM
)

# 추가: 복사할 파일이 있는지 미리 확인
file(GLOB CHECK_SPV_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.spv")
if(CHECK_SPV_FILES)
    list(LENGTH CHECK_SPV_FILES SPV_COUNT)
    message(STATUS "Found ${SPV_COUNT} .spv files to copy")
    foreach(SPV_FILE ${CHECK_SPV_FILES})
        get_filename_component(SPV_NAME ${SPV_FILE} NAME)
        message(STATUS "  - ${SPV_NAME}")
    endforeach()
else()
    message(WARNING "No .spv files found in shaders/ directory")
    message(STATUS "Make sure to build the project to compile shaders first")
endif()

# Windows Visual Studio를 위한 추가 보장
if(WIN32)
    # Visual Studio의 경우 각 구성에 대해서도 복사
    add_custom_command(TARGET VulkanApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Output directory: $<TARGET_FILE_DIR:VulkanApp>"
        COMMAND ${CMAKE_COMMAND} -E echo "Copying shaders from: ${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        COMMAND if exist "${CMAKE_CURRENT_SOURCE_DIR}\\shaders\\*.spv" (
            ${CMAKE_COMMAND} -E echo "SPV files found, copying..."
        ) else (
            ${CMAKE_COMMAND} -E echo "No SPV files found!"
        )
        COMMENT "Debug information for shader copying"
    )
endif()

# macOS용 실행 스크립트 생성 (수정된 안전한 버전)
if(APPLE AND DEFINED VK_LAYER_PATH_DIR)
    # Validation Layers 경로가 실제로 존재하는지 확인
    if(EXISTS "${VK_LAYER_PATH_DIR}")
        # 스크립트 파일들을 직접 생성 (문법 에러 문제 해결)
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/create_scripts.cmake"
            "# macOS 실행 스크립트 생성\n"
            "file(WRITE \"\${SCRIPT_DIR}/run_vulkan.sh\" \"#!/bin/bash\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_vulkan.sh\" \"# macOS Vulkan Application Runner with Validation Layers\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_vulkan.sh\" \"export VK_LAYER_PATH=\\\"\${VK_LAYER_PATH_DIR}\\\"\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_vulkan.sh\" \"export VK_ICD_FILENAMES=\\\"\${VK_ICD_PATH}\\\"\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_vulkan.sh\" \"echo \\\"Starting Vulkan app with validation layers on macOS...\\\"\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_vulkan.sh\" \"echo \\\"VK_LAYER_PATH: $VK_LAYER_PATH\\\"\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_vulkan.sh\" \"echo \\\"VK_ICD_FILENAMES: $VK_ICD_FILENAMES\\\"\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_vulkan.sh\" \"echo \\\"\\\"\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_vulkan.sh\" \"./VulkanApp\n\")\n"
            "execute_process(COMMAND chmod +x \"\${SCRIPT_DIR}/run_vulkan.sh\")\n"
            "\n"
            "file(WRITE \"\${SCRIPT_DIR}/run_simple.sh\" \"#!/bin/bash\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_simple.sh\" \"# Simple macOS Vulkan Application Runner (no validation)\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_simple.sh\" \"echo \\\"Starting Vulkan app without validation layers...\\\"\n\")\n"
            "file(APPEND \"\${SCRIPT_DIR}/run_simple.sh\" \"./VulkanApp\n\")\n"
            "execute_process(COMMAND chmod +x \"\${SCRIPT_DIR}/run_simple.sh\")\n"
        )
        
        add_custom_command(TARGET VulkanApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} 
                -DSCRIPT_DIR=$<TARGET_FILE_DIR:VulkanApp>
                -DVK_LAYER_PATH_DIR=${VK_LAYER_PATH_DIR}
                -DVK_ICD_PATH=${VK_ICD_PATH}
                -P "${CMAKE_CURRENT_BINARY_DIR}/create_scripts.cmake"
            COMMENT "Creating macOS Vulkan run scripts"
            VERBATIM
        )
        
        message(STATUS "macOS validation layers scripts will be generated at build time")
        message(STATUS "VK_LAYER_PATH: ${VK_LAYER_PATH_DIR}")
        message(STATUS "VK_ICD_FILENAMES: ${VK_ICD_PATH}")
    else()
        message(WARNING "Validation layers not found at ${VK_LAYER_PATH_DIR}. Skipping script generation.")
    endif()
else()
    if(NOT APPLE)
        message(STATUS "Not on macOS - skipping validation layers script generation")
    else()
        message(WARNING "VK_LAYER_PATH_DIR not defined - skipping validation layers script generation")
    endif()
endif()

# 정보 출력
message(STATUS "==========================================")
message(STATUS "SHADER COPY SETUP:")
message(STATUS "Source: ${CMAKE_CURRENT_SOURCE_DIR}/shaders")
message(STATUS "Target: <BUILD_DIR>/shaders")
message(STATUS "==========================================")