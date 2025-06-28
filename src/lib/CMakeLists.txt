add_library(${PROJECT_NAME}_lib STATIC
    raw_loader.cpp
    raw_loader.h
    pipeline.cpp
    pipeline.h
    types.h
    pipeline/bayer_color.h
    pipeline/black_level.h
    pipeline/white_level.h
    pipeline/demosaic_bilinear.h
    pipeline/white_balance.h
    pipeline/exposure.h
    pipeline/tone_mapping.h
    pipeline/halide_pipeline.h
)
target_link_libraries(${PROJECT_NAME}_lib 
    PRIVATE Qt6::Widgets
    PUBLIC libraw::libraw
    PRIVATE ${OpenCV_LIBS}
    PRIVATE Halide::Halide
    TracyClient)
    
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})