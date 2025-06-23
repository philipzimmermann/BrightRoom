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
    void SetImage(const QImage& new_image);
    void ScaleImage(double factor);
    void AdjustScrollBar(QScrollBar* scroll_bar, double factor);

    QImage _fullSizeImage;
    QImage _scaledImage;
    QLabel* _imageLabel;
    QScrollArea* _scrollArea;
    double _scaleFactor = 1;

    QAction* _zoomInAct;
    QAction* _zoomOutAct;
    QAction* _normalSizeAct;
    QAction* _fitToWindowAct;
};