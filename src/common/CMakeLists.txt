add_library(common STATIC
    constants.h
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})