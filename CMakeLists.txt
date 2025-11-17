cmake_minimum_required(VERSION 3.10)
project(intelcodec C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find VA-API (Video Acceleration API)
pkg_check_modules(LIBVA REQUIRED libva libva-drm)

# Find EGL and OpenGL ES
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLES2 REQUIRED glesv2)

# Find DRM (Direct Rendering Manager)
pkg_check_modules(LIBDRM REQUIRED libdrm)

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

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBVA_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLES2_INCLUDE_DIRS}
    ${LIBDRM_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${LIBVA_LIBRARIES}
    ${EGL_LIBRARIES}
    ${GLES2_LIBRARIES}
    ${LIBDRM_LIBRARIES}
)

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
target_compile_definitions(${PROJECT_NAME} PRIVATE
    LENGTH(x)=sizeof(x)/sizeof(x[0])
)

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
message(STATUS "  EGL:             ${EGL_LIBRARIES}")
message(STATUS "  GLESv2:          ${GLES2_LIBRARIES}")
message(STATUS "  libdrm:          ${LIBDRM_LIBRARIES}")
if(GBM_FOUND)
message(STATUS "  GBM:             ${GBM_LIBRARIES}")
else()
message(STATUS "  GBM:             Not found (optional)")
endif()
message(STATUS "")
