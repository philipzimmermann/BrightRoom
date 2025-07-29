#include <stdio.h>
#include <QApplication>
#include <QGuiApplication>
#include "main_window.h"

int main(int argc, char* argv[]) {
    printf("Hello, from brightroom!\n");
    //load_raw();
    QApplication app(argc, argv);
    QGuiApplication::setApplicationDisplayName("BrightRoom");
    MainWindow main_window;
    main_window.show();
    return app.exec();
}
