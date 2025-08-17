#include <QApplication>
#include <QGuiApplication>
#include "HalideRawPipeline.h"
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    printf("Hello, from brightroom!\n");
    //load_raw();
    QApplication app(argc, argv);
    QGuiApplication::setApplicationDisplayName("BrightRoom");
    auto pipeline = std::make_unique<brightroom::HalideRawPipeline>();
    MainWindow main_window{nullptr, std::move(pipeline)};
    main_window.show();
    return QApplication::exec();
}
