#pragma once

#include <qboxlayout.h>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QScrollArea>
#include <QSlider>
#include <QThread>
#include "IRawPipeline.h"
#include "ImageProcessorWorker.h"
#include "MySlider.h"

#include "HistogramWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent, std::unique_ptr<brightroom::IRawPipeline> pipeline);
    ~MainWindow();
    bool LoadImage(const QString&);
    bool LoadRaw(const QString&);

   protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

   signals:
    void LoadRawRequested(const QString& fileName, const brightroom::Parameters& parameters);
    void ProcessImageRequested(const brightroom::Parameters& parameters);

   private slots:
    void Open();
    void ZoomIn();
    void ZoomOut();
    void NormalSize();
    void FitToWindow();
    void OnImageProcessed(brightroom::RgbImage image, brightroom::Histogram histogram);
    void OnProcessingFailed(const QString& error);

   private:
    void CreateActions();
    void SetImage(const QImage& new_image);
    void ScaleImage(double requested_zoom);
    void AdjustScrollBar(QScrollBar* scroll_bar, double zoom_change);
    void UpdateFitZoom();
    void CreateEditDock();
    void RequestProcessImage();
    void OnDebounceTimeout();
    void ResetSliders();
    void QueueImageRefresh();
    void ConnectSlider(MySlider* slider, std::function<void(float)> value_changed);
    void HandleWheelEvent(QWheelEvent* event);
    void HandleMousePressEvent(QMouseEvent* event);
    void HandleMouseReleaseEvent(QMouseEvent* event);
    void HandleMouseMoveEvent(QMouseEvent* event);
    auto CreateAdjustmentSlider(QWidget* parent, const QString& label, QVBoxLayout* layout) -> MySlider*;

    QImage _fullSizeImage;
    QImage _scaledImage;
    QLabel* _imageLabel;
    QScrollArea* _scrollArea;

    QAction* _zoomInAct;
    QAction* _zoomOutAct;
    QAction* _normalSizeAct;
    QAction* _fitToWindowAct;

    QPoint _lastDragPos;
    QTimer* _refreshTimer;

    QDockWidget* _editDock;
    MySlider* _exposureSlider;
    MySlider* _contrastSlider;
    MySlider* _saturationSlider;
    std::vector<MySlider*> _sliders;

    QDockWidget* _histogramDock;
    HistogramWidget* _histogramWidget;

    brightroom::Parameters _parameters{};

    // Background processing
    QThread* _workerThread;
    ImageProcessorWorker* _imageProcessorWorker;
    bool _processingInProgress = false;
    bool _newImage = false;

    bool _isDragging = false;
    double _zoom = 1;
    double _fit_zoom = 1;

    static constexpr double kZoomInFactor = 1.25;
    static constexpr double kZoomOutFactor = 0.8;
    static constexpr int kSliderTickInterval = 50;
    static constexpr int kSliderRangeMin = -150;
    static constexpr int kSliderRangeMax = 150;
    static constexpr int kDebounceDelayMs = 100;
};