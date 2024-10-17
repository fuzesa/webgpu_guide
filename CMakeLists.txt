cmake_minimum_required(VERSION 3.29)
project(webgpu_guide_clion
        VERSION 1.0.0
        LANGUAGES CXX C)

# Using Google Dawn instead of Mozilla wgpu
set(WEBGPU_BACKEND DAWN)
#set(WEBGPU_BACKEND WGPU)
#set(WEBGPU_BACKEND EMSCRIPTEN)

message(STATUS "WEBGPU_BACKEND IS ${WEBGPU_BACKEND}")

# When using Google Dawn, make sure to add it BEFORE
# the WebGPU distribution subdir
if (NOT EMSCRIPTEN)
    add_subdirectory(3rdparty/glfw)
else()
    # Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(3rdparty/WebGPU-distribution)

add_subdirectory(3rdparty/glfw3webgpu)

add_executable(${PROJECT_NAME}
        src/app.cc
        src/main.cc)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE webgpu glfw glfw3webgpu)

# In case of Mozilla wgpu, we need to copy the lib
if (WEBGPU_BACKEND MATCHES "WGPU")
    target_copy_webgpu_binaries(${PROJECT_NAME})
endif ()

# Options that are specific to Emscripten
if (WEBGPU_BACKEND MATCHES "EMSCRIPTEN")
    # Generate a full web page rather than a simple WebAssembly module
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

    # Enable the use of emscripten_sleep()
    target_link_options(${PROJECT_NAME} PRIVATE -sASYNCIFY)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_WARNING_AS_ERROR ON)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

if (XCODE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
endif()