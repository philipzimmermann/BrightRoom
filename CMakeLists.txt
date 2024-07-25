cmake_minimum_required(VERSION 3.13.0)

project(raw_editor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package (JPEG REQUIRED)
find_package(OpenCV REQUIRED )

set(LIBRAW_PATH ${CMAKE_CURRENT_SOURCE_DIR}/LibRaw)
add_subdirectory(LibRaw-cmake)
# add_subdirectory(rawspeed)

add_subdirectory(src/app)

add_subdirectory(src/lib)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${PROJECT_NAME}_lib
        Qt6::Widgets)


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
gtest_discover_tests(loader_test)