add_executable(${PROJECT_NAME}
    MainWindow.cpp
    main.cpp
    MySlider.cpp
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE Qt6::Widgets
    PRIVATE ${PROJECT_NAME}_lib
    PRIVATE TracyClient
)

# target_include_directories(${PROJECT_NAME} 
#     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib
# )

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)