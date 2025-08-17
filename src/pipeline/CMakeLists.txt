add_library(${PROJECT_NAME}_lib STATIC
    RawLoader.cpp
    HalideRawPipeline.cpp
)
add_halide_generator(generator_target SOURCES halide/generator.cpp)
add_halide_library(preprocess_raw_generator FROM generator_target
AUTOSCHEDULER Halide::Adams2019
SCHEDULE "schedule.txt"
)
add_halide_library(process_raw_generator FROM generator_target
AUTOSCHEDULER Halide::Adams2019
SCHEDULE "schedule.txt"
)

target_link_libraries(${PROJECT_NAME}_lib
    PRIVATE Qt6::Widgets
    PUBLIC libraw::libraw
    PRIVATE ${OpenCV_LIBS}
    PUBLIC Halide::Halide
    PRIVATE preprocess_raw_generator
    PRIVATE process_raw_generator
    TracyClient)
    
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})