add_library(${PROJECT_NAME}_lib STATIC
    loader.cpp
)
target_link_libraries(${PROJECT_NAME}_lib 
    PRIVATE Qt6::Widgets
    PRIVATE libraw::libraw
    PRIVATE ${OpenCV_LIBS})
    
target_include_directories(${PROJECT_NAME}_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})