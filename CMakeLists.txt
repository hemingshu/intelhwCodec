cmake_minimum_required(VERSION 3.10)
project(intelcodec C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find VA-API (Video Acceleration API)
pkg_check_modules(LIBVA REQUIRED libva libva-drm)

# Find EGL and OpenGL ES
# Try pkg-config first, then fall back to manual detection
pkg_check_modules(EGL egl)
pkg_check_modules(GLES2 glesv2)

# Find DRM (Direct Rendering Manager)
# Try pkg-config first, then fall back to manual detection
pkg_check_modules(LIBDRM libdrm)

# Optional: Find GBM (Generic Buffer Management)
# Uncomment if not using EGL_MESA_PLATFORM_SURFACELESS
pkg_check_modules(GBM gbm)

# Source files
set(SOURCES
    main.c
    bitstream.c
    encode.c
    gpu.c
    hevc.c
    proto.c
)

# Header files
set(HEADERS
    bitstream.h
    colorspace.h
    encode.h
    gpu.h
    hevc.h
    proto.h
)

# Shader files
set(SHADER_FILES
    vertex.glsl
    luma.glsl
    chroma.glsl
)

# Convert shader files to object files using objcopy/ld
set(SHADER_OBJECTS "")
foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.o")
    add_custom_command(
        OUTPUT ${SHADER_OBJ}
        COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && ld -r -b binary -o ${SHADER_OBJ} ${SHADER}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        COMMENT "Embedding shader ${SHADER}"
    )
    list(APPEND SHADER_OBJECTS ${SHADER_OBJ})
endforeach()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${SHADER_OBJECTS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBVA_INCLUDE_DIRS}
)

# Add DRM include directories if found
if(LIBDRM_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBDRM_INCLUDE_DIRS})
endif()

# Add EGL and GLES2 include directories if found
if(EGL_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${EGL_INCLUDE_DIRS})
endif()
if(GLES2_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GLES2_INCLUDE_DIRS})
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${LIBVA_LIBRARIES}
)

# Link DRM if found via pkg-config, otherwise use default library
if(LIBDRM_FOUND)
    target_link_libraries(${PROJECT_NAME} ${LIBDRM_LIBRARIES})
else()
    # Fall back to manual DRM library linking
    target_link_libraries(${PROJECT_NAME} drm)
    message(STATUS "libdrm not found via pkg-config, using default drm library")
endif()

# Link EGL and GLES2 if found via pkg-config, otherwise use default libraries
if(EGL_FOUND)
    target_link_libraries(${PROJECT_NAME} ${EGL_LIBRARIES})
else()
    # Fall back to manual EGL library linking
    target_link_libraries(${PROJECT_NAME} EGL)
    message(STATUS "EGL not found via pkg-config, using default EGL library")
endif()

if(GLES2_FOUND)
    target_link_libraries(${PROJECT_NAME} ${GLES2_LIBRARIES})
else()
    # Fall back to manual GLESv2 library linking
    target_link_libraries(${PROJECT_NAME} GLESv2)
    message(STATUS "GLESv2 not found via pkg-config, using default GLESv2 library")
endif()

# Add GBM if available and not using surfaceless platform
if(GBM_FOUND AND NOT USE_EGL_MESA_PLATFORM_SURFACELESS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GBM_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${GBM_LIBRARIES})
endif()

# Compiler definitions
# Uncomment the following line to use EGL_MESA_PLATFORM_SURFACELESS
# target_compile_definitions(${PROJECT_NAME} PRIVATE USE_EGL_MESA_PLATFORM_SURFACELESS)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Add LENGTH macro definition (used in the code)
# Note: LENGTH is typically defined as a C macro, not a CMake definition
# The proper way is to define it in the C code or pass it correctly
# Removed problematic CMake definition - should be handled in C headers

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project Name:    ${PROJECT_NAME}")
message(STATUS "  C Standard:      ${CMAKE_C_STANDARD}")
message(STATUS "  Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix:  ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  libva:           ${LIBVA_LIBRARIES}")
if(LIBDRM_FOUND)
message(STATUS "  libdrm:          ${LIBDRM_LIBRARIES}")
else()
message(STATUS "  libdrm:          Using default drm library")
endif()
if(EGL_FOUND)
message(STATUS "  EGL:             ${EGL_LIBRARIES}")
else()
message(STATUS "  EGL:             Using default EGL library")
endif()
if(GLES2_FOUND)
message(STATUS "  GLESv2:          ${GLES2_LIBRARIES}")
else()
message(STATUS "  GLESv2:          Using default GLESv2 library")
endif()
if(GBM_FOUND)
message(STATUS "  GBM:             ${GBM_LIBRARIES}")
else()
message(STATUS "  GBM:             Not found (optional)")
endif()
message(STATUS "")
