#pragma once

#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QScrollArea>
#include <QSlider>
#include "libraw/libraw.h"
#include "pipeline.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    bool LoadImage(const QString&);
    bool LoadRaw(const QString&);

   protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

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
    void ScaleImage(double requested_zoom);
    void AdjustScrollBar(QScrollBar* scroll_bar, double zoom_change);
    void CreateAdjustmentsDock();
    void RefreshImage();
    void QueueImageRefresh();

    QImage _fullSizeImage;
    QImage _scaledImage;
    QLabel* _imageLabel;
    QScrollArea* _scrollArea;
    double _zoom = 1;
    double _fit_zoom = 1;

    QAction* _zoomInAct;
    QAction* _zoomOutAct;
    QAction* _normalSizeAct;
    QAction* _fitToWindowAct;

    bool _isDragging = false;
    QPoint _lastDragPos;

    QDockWidget* _adjustmentsDock;
    QSlider* _exposureSlider;
    QTimer* _refreshTimer;

    std::unique_ptr<LibRaw> _currentRaw;
    raw::Parameters _parameters;
    raw::Pipeline _pipeline;
};