#include <stdio.h>
#include <QApplication>
#include "loader.h"
#include "main_window.h"

int main(int argc, char* argv[]) {
  printf("Hello, from raw_editor!\n");
  QApplication app(argc, argv);
  QGuiApplication::setApplicationDisplayName(MainWindow::tr("Image Viewer"));
  MainWindow main_window;
  main_window.show();
  return app.exec();
}
