add_halide_generator(generator_target SOURCES halide/generator.cpp)
add_halide_library(preprocess_raw_generator FROM generator_target
AUTOSCHEDULER Halide::Adams2019
SCHEDULE "schedule.txt"
)
add_halide_library(process_raw_generator FROM generator_target
AUTOSCHEDULER Halide::Adams2019
SCHEDULE "schedule.txt"
)

add_library(pipeline STATIC
    RawLoader.cpp
    HalideRawPipeline.cpp
)
target_link_libraries(pipeline
    PRIVATE Qt6::Widgets
    PUBLIC libraw::libraw
    PRIVATE ${OpenCV_LIBS}
    PUBLIC Halide::Halide
    PRIVATE preprocess_raw_generator
    PRIVATE process_raw_generator
    TracyClient)
    
target_include_directories(pipeline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})