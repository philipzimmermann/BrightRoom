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
    void SaveAs();
    // void Print();
    void Copy();
    void Paste();
    void ZoomIn();
    void ZoomOut();
    void NormalSize();
    void FitToWindow();
    void About();

   private:
    void CreateActions();
    void CreateMenus();
    void UpdateActions();
    bool SaveFile(const QString& fileName);
    void SetImage(const QImage& newImage);
    void ScaleImage(double factor);
    void AdjustScrollBar(QScrollBar* scrollBar, double factor);

    QImage image;
    QLabel* imageLabel;
    QScrollArea* scrollArea;
    double scaleFactor = 1;

    QAction* saveAsAct;
    // QAction* printAct;
    QAction* copyAct;
    QAction* zoomInAct;
    QAction* zoomOutAct;
    QAction* normalSizeAct;
    QAction* fitToWindowAct;
};