#include <QApplication>
#include <QGuiApplication>
#include "HalideRawPipeline.h"
#include "IRawPipeline.h"
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    printf("Hello, from brightroom!\n");
    //load_raw();
    QApplication app(argc, argv);
    QGuiApplication::setApplicationDisplayName("BrightRoom");

    // Register custom types with Qt's meta-type system
    qRegisterMetaType<brightroom::Parameters>("brightroom::Parameters");
    qRegisterMetaType<brightroom::RgbImage>("brightroom::RgbImage");
    qRegisterMetaType<brightroom::Histogram>("brightroom::Histogram");

    auto pipeline = std::make_unique<brightroom::HalideRawPipeline>();
    MainWindow main_window{nullptr, std::move(pipeline)};
    main_window.show();
    return QApplication::exec();
}
