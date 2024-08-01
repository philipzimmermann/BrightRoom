#include <stdio.h>
#include <QApplication>
#include "main_window.h"

int main(int argc, char* argv[]) {
  printf("Hello, from raw_editor!\n");
  //load_raw();
  QApplication app(argc, argv);
  QGuiApplication::setApplicationDisplayName(MainWindow::tr("BrightRoom"));
  MainWindow main_window;
  main_window.show();
  return app.exec();
}
