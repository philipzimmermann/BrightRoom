cmake_minimum_required(VERSION 3.13.0)

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
find_package (JPEG REQUIRED)
find_package(OpenCV REQUIRED )
# find_package(OpenMP REQUIRED)

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

# LibRaw
set(LIBRAW_PATH ${CMAKE_CURRENT_SOURCE_DIR}/submodules/thirdparty/LibRaw)
add_subdirectory(submodules/thirdparty/LibRaw-cmake)

# Halide
set(Halide_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/halide/lib/cmake/Halide)
find_package(Halide REQUIRED)

add_subdirectory(src/lib)

add_subdirectory(src/app)


#CTest
include(CTest)
enable_testing()

#CPack
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


# GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
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