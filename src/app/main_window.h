#pragma once

//#include <QLabel>
#include <QLabel>
#include <QMainWindow>
#include <QScrollArea>

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = nullptr);
  bool loadImage(const QString&);
  bool loadRaw(const QString&);

 private slots:
  void open();
  void saveAs();
  void print();
  void copy();
  void paste();
  void zoomIn();
  void zoomOut();
  void normalSize();
  void fitToWindow();
  void about();

 private:
  void createActions();
  void createMenus();
  void updateActions();
  bool saveFile(const QString& fileName);
  void setImage(const QImage& newImage);
  void scaleImage(double factor);
  void adjustScrollBar(QScrollBar* scrollBar, double factor);

  QImage image;
  QLabel* imageLabel;
  QScrollArea* scrollArea;
  double scaleFactor = 1;

  QAction* saveAsAct;
  QAction* printAct;
  QAction* copyAct;
  QAction* zoomInAct;
  QAction* zoomOutAct;
  QAction* normalSizeAct;
  QAction* fitToWindowAct;
};