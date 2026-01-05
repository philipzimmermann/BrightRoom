cmake_minimum_required(VERSION 3.14.0)

project(brightroom VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Windows-specific definitions to prevent macro conflicts
if(WIN32)
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

if(WIN32 AND NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/halide/lib/Release/Halide.lib)
  message(STATUS "Halide not found, fetching...")
  execute_process(COMMAND ./scripts/fetch_halide.ps1
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
elseif(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/halide/lib/libHalide.so)
  message(STATUS "Halide not found, fetching...")
  execute_process(COMMAND ./scripts/fetch_halide.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

# Add Tracy profiler
add_library(TracyClient STATIC submodules/thirdparty/tracy/public/TracyClient.cpp
                                submodules/thirdparty/tracy/public/tracy/TracyOpenCL.hpp)
target_include_directories(TracyClient PUBLIC submodules/thirdparty/tracy/public/tracy)
target_compile_definitions(TracyClient PUBLIC TRACY_ENABLE=1)

# LibRaw - build as static library
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(LIBRAW_PATH ${CMAKE_CURRENT_SOURCE_DIR}/submodules/thirdparty/LibRaw)
add_subdirectory(submodules/thirdparty/LibRaw-cmake)

# Halide
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
set(Halide_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/halide/lib/cmake/Halide)
find_package(Halide REQUIRED)

add_subdirectory(src/pipeline)
add_subdirectory(src/gui)
add_subdirectory(src/common)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE gui common)
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)


# ------------------------------ Post-build ------------------------------

# GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Force static libraries on Windows to avoid DLL symbol export issues
if(WIN32)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)
enable_testing()
add_executable(
    loader_test
    test/loader_test.cpp
)
target_link_libraries(
        loader_test
  GTest::gtest_main
)
include(GoogleTest)
# Temporarily disable test discovery due to DLL issues
# gtest_discover_tests(loader_test)