add_library(${PROJECT_NAME}_lib STATIC
    raw_loader.cpp
    raw_loader.h
    pipeline.cpp
    pipeline.h
    types.h
    halide_pipeline.h
    classic_pipeline.cpp
    classic_pipeline.h
)
add_halide_generator(generator_target SOURCES generator.cpp)
add_halide_library(raw_pipeline_generator FROM generator_target)

target_link_libraries(${PROJECT_NAME}_lib 
    PRIVATE Qt6::Widgets
    PUBLIC libraw::libraw
    PRIVATE ${OpenCV_LIBS}
    PRIVATE Halide::Halide
    PRIVATE raw_pipeline_generator
    # PRIVATE OpenMP::OpenMP_CXX
    TracyClient)
    
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})