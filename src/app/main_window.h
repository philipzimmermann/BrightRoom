#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QScrollArea>

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    bool LoadImage(const QString&);
    bool LoadRaw(const QString&);

   private slots:
    void Open();
    void ZoomIn();
    void ZoomOut();
    void NormalSize();
    void FitToWindow();

   private:
    void CreateActions();
    void CreateMenus();
    void UpdateActions();
    void SetImage(const QImage& newImage);
    void ScaleImage(double factor);
    void AdjustScrollBar(QScrollBar* scrollBar, double factor);

    QImage _image;
    QLabel* _imageLabel;
    QScrollArea* _scrollArea;
    double _scaleFactor = 1;

    QAction* _zoomInAct;
    QAction* _zoomOutAct;
    QAction* _normalSizeAct;
    QAction* _fitToWindowAct;
};