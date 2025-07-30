add_library(${PROJECT_NAME}_lib STATIC
    raw_loader.cpp
    pipeline.cpp
    classic_pipeline.cpp
)
add_halide_generator(generator_target SOURCES generator.cpp)
add_halide_library(raw_pipeline_generator FROM generator_target
AUTOSCHEDULER Halide::Adams2019
SCHEDULE "schedule.txt"
)

target_link_libraries(${PROJECT_NAME}_lib 
    PRIVATE Qt6::Widgets
    PUBLIC libraw::libraw
    PRIVATE ${OpenCV_LIBS}
    PRIVATE Halide::Halide
    PRIVATE raw_pipeline_generator
    # PRIVATE OpenMP::OpenMP_CXX
    TracyClient)
    
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})