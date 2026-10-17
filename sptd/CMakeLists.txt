# south_park_td - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(south_park_td VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# TRG Launcher (third_party/trg-launcher, a submodule). It draws inside the
# runtime's own Dear ImGui frame, so it must use the ImGui compiled into
# rexruntime (one ImGui context), never a downloaded copy.
add_library(sptd_imgui INTERFACE)
target_link_libraries(sptd_imgui INTERFACE rex::runtime)
set(TRG_LAUNCHER_IMGUI_TARGET sptd_imgui CACHE STRING "" FORCE)
set(TRG_LAUNCHER_STANDALONE OFF CACHE BOOL "" FORCE)
set(TRG_LAUNCHER_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(TRG_LAUNCHER_FETCH_DEPS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/trg-launcher trg-launcher)
# The launcher library is written for C++17 (in C++20 and later, u8string() is
# a different type, which breaks its Linux code paths).
set_target_properties(trg_launcher PROPERTIES CXX_STANDARD 17)

# Sources
set(SOUTHPARKTD_SOURCES
    src/main.cpp
    src/stubs.cpp
    src/profile_fix.cpp
    src/null_trace.cpp
    src/fpe_guard.cpp
    src/frame_stats.cpp
    src/sampler.cpp
    src/settings.cpp
    src/input_remap.cpp
    src/launcher.cpp
    src/platform.cpp
    src/stfs.cpp
    src/art.cpp
    src/overlay.cpp
    src/toast.cpp
    src/updater.cpp
)

# Developer-only tracing hooks (never in release builds): -DSPTD_DEV_TOOLS=ON.
option(SPTD_DEV_TOOLS "Build developer tracing hooks" OFF)
if(SPTD_DEV_TOOLS)
    list(APPEND SOUTHPARKTD_SOURCES src/dev_trace.cpp)
endif()

if(WIN32)
    enable_language(RC)
    list(APPEND SOUTHPARKTD_SOURCES src/app.rc)  # the .exe icon
    add_executable(south_park_td WIN32 ${SOUTHPARKTD_SOURCES})
else()
    add_executable(south_park_td ${SOUTHPARKTD_SOURCES})
endif()

rexglue_setup_target(south_park_td GPU_PLUGINS xenos)
target_link_libraries(south_park_td PRIVATE trg::launcher)
# The launcher's updater compares this with the latest GitHub release tag.
target_compile_definitions(south_park_td PRIVATE SPTD_VERSION="${PROJECT_VERSION}")
if(SPTD_DEV_TOOLS)
    target_compile_definitions(south_park_td PRIVATE SPTD_DEV_TOOLS)
endif()

# SEH macros for the generated code (src/seh_prelude.h explains).
target_compile_options(south_park_td_recomp PRIVATE
    "SHELL:-include \"${CMAKE_CURRENT_SOURCE_DIR}/src/seh_prelude.h\"")
