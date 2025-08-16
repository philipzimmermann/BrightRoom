#pragma once

#include <qboxlayout.h>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QScrollArea>
#include <QSlider>
#include "libraw/libraw.h"
#include "my_slider.h"
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
    void SetImage(const QImage& new_image, bool fit_to_window);
    void ScaleImage(double requested_zoom);
    void AdjustScrollBar(QScrollBar* scroll_bar, double zoom_change);
    void CreateAdjustmentsDock();
    void RefreshImage();
    void QueueImageRefresh();
    void ConnectSlider(MySlider* slider, std::function<void(float)> value_changed);
    void HandleWheelEvent(QWheelEvent* event);
    void HandleMousePressEvent(QMouseEvent* event);
    void HandleMouseReleaseEvent(QMouseEvent* event);
    void HandleMouseMoveEvent(QMouseEvent* event);
    MySlider* CreateAdjustmentSlider(QWidget* parent, const QString& label, QVBoxLayout* layout);

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
    QTimer* _refreshTimer;

    QDockWidget* _adjustmentsDock;
    MySlider* _exposureSlider;
    MySlider* _contrastSlider;
    MySlider* _saturationSlider;

    std::unique_ptr<LibRaw> _currentRaw;
    raw::Parameters _parameters{};
    raw::Pipeline _pipeline;

    // Add these constants
    static constexpr double ZOOM_IN_FACTOR = 1.25;
    static constexpr double ZOOM_OUT_FACTOR = 0.8;
    static constexpr int SLIDER_TICK_INTERVAL = 33;
    static constexpr int REFRESH_DELAY_MS = 150;
};