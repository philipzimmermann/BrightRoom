add_library(${PROJECT_NAME}_lib STATIC
    raw_loader.cpp
    raw_loader.h
    pipeline.cpp
    pipeline.h
    types.h
)
target_link_libraries(${PROJECT_NAME}_lib 
    PRIVATE Qt6::Widgets
    PUBLIC libraw::libraw
    PRIVATE ${OpenCV_LIBS}
    TracyClient)
    
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})