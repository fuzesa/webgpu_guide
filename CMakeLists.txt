cmake_minimum_required(VERSION 3.29)
project(webgpu_guide_clion
        VERSION 1.0.0
        LANGUAGES CXX C)

# Using Google Dawn instead of Mozilla wgpu
set(WEBGPU_BACKEND DAWN)

add_subdirectory(3rdparty/WebGPU-distribution)

add_executable(${PROJECT_NAME}
        src/main.cc)

target_link_libraries(${PROJECT_NAME} PRIVATE webgpu)

# In case of Mozilla wgpu, we need to copy the lib
if (WEBGPU_BACKEND MATCHES "WGPU")
    target_copy_webgpu_binaries(${PROJECT_NAME})
endif ()

# Options that are specific to Emscripten
if (EMSCRIPTEN)
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