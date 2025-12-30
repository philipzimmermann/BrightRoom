add_library(gui STATIC
    MainWindow.cpp
    MySlider.cpp
    HistogramWidget.cpp
    ImageProcessorWorker.cpp
)

target_include_directories(gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gui 
    PUBLIC Qt6::Widgets
    PUBLIC pipeline
    PUBLIC common
)