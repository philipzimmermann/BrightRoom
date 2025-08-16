#include "main_window.h"
#include <qimage.h>
#include <iostream>
#include "pipeline.h"
#include "raw_loader.h"

#include <QApplication>
#include <QClipboard>
#include <QColorSpace>
#include <QDir>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

#include "Tracy.hpp"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), _imageLabel(new QLabel), _scrollArea(new QScrollArea) {
    setWindowTitle("BrightRoom");
    _imageLabel->setBackgroundRole(QPalette::Base);
    _imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    _imageLabel->setScaledContents(true);

    _scrollArea->setBackgroundRole(QPalette::Dark);
    _scrollArea->setWidget(_imageLabel);
    _scrollArea->setVisible(true);
    _scrollArea->viewport()->installEventFilter(this);
    setCentralWidget(_scrollArea);

    // Initialize the refresh timer
    _refreshTimer = new QTimer(this);
    _refreshTimer->setSingleShot(true);
    _refreshTimer->setInterval(REFRESH_DELAY_MS);  // 150ms debounce delay
    connect(_refreshTimer, &QTimer::timeout, this, &MainWindow::RefreshImage);

    CreateAdjustmentsDock();
    CreateActions();

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
    // QTimer::singleShot(0, this, [this]() { LoadRaw("/media/philip/Data SSD/photos/2025/06/22/QI9B7671.CR2"); });
}

auto MainWindow::CreateAdjustmentSlider(QWidget* parent, const QString& label, QVBoxLayout* layout) -> MySlider* {
    auto* sliderLabel = new QLabel(label, parent);
    auto* slider = new MySlider(Qt::Horizontal, parent);
    constexpr int kSliderRange = 100;
    slider->setRange(-kSliderRange, kSliderRange);
    slider->setValue(0);
    slider->setTickPosition(QSlider::TicksBelow);
    slider->setTickInterval(SLIDER_TICK_INTERVAL);

    layout->addWidget(sliderLabel);
    layout->addWidget(slider);

    return slider;
}

void MainWindow::CreateAdjustmentsDock() {
    _adjustmentsDock = new QDockWidget(tr("Adjustments"), this);
    _adjustmentsDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    auto* adjustmentsWidget = new QWidget(_adjustmentsDock);
    auto* layout = new QVBoxLayout(adjustmentsWidget);
    adjustmentsWidget->setMinimumWidth(200);

    // Create sliders
    _exposureSlider = CreateAdjustmentSlider(adjustmentsWidget, tr("Exposure"), layout);
    _contrastSlider = CreateAdjustmentSlider(adjustmentsWidget, tr("Contrast"), layout);
    _saturationSlider = CreateAdjustmentSlider(adjustmentsWidget, tr("Saturation"), layout);

    layout->addStretch();
    adjustmentsWidget->setLayout(layout);
    _adjustmentsDock->setWidget(adjustmentsWidget);
    addDockWidget(Qt::RightDockWidgetArea, _adjustmentsDock);

    // Connect sliders
    ConnectSlider(_exposureSlider,
                  [this](float value) { _parameters.exposure = std::pow(2.0f, value / SLIDER_TICK_INTERVAL); });
    ConnectSlider(_contrastSlider,
                  [this](float value) { _parameters.contrast = std::pow(1.5f, value / SLIDER_TICK_INTERVAL); });
    ConnectSlider(_saturationSlider,
                  [this](float value) { _parameters.saturation = std::pow(2.0f, value / SLIDER_TICK_INTERVAL); });
}

// Helper method for connecting sliders
void MainWindow::ConnectSlider(MySlider* slider, std::function<void(float)> valueChanged) {
    connect(slider, &QSlider::valueChanged, this, [this, valueChanged, slider]() {
        valueChanged(static_cast<float>(slider->value()));
        QueueImageRefresh();
    });
    connect(slider, &MySlider::doubleClicked, this, [this, slider]() {
        slider->setValue(0);
        QueueImageRefresh();
    });
}

void MainWindow::QueueImageRefresh() {
    _refreshTimer->start();
}

bool MainWindow::LoadImage(const QString& fileName) {
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage new_image = reader.read();
    if (new_image.isNull()) {
        QMessageBox::information(
            this, QGuiApplication::applicationDisplayName(),
            tr("Cannot load %1: %2").arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }
    SetImage(new_image, true);
    setWindowFilePath(fileName);
    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
                                .arg(QDir::toNativeSeparators(fileName))
                                .arg(_fullSizeImage.width())
                                .arg(_fullSizeImage.height())
                                .arg(_fullSizeImage.depth());
    statusBar()->showMessage(message);
    return true;
}

bool MainWindow::LoadRaw(const QString& fileName) {
    ZoneScopedN("LoadRaw");
    raw::RawLoader loader{};
    _currentRaw = loader.LoadRaw(fileName.toStdString());

    auto processed_image = _pipeline.Run(*_currentRaw, _parameters);
    QImage new_image(processed_image.pixels.data(), processed_image.width, processed_image.height,
                     QImage::Format::Format_RGB888);
    if (new_image.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(), tr("Cannot load %1: %2"));
        return false;
    }
    SetImage(new_image, true);

    setWindowFilePath(fileName);
    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
                                .arg(QDir::toNativeSeparators(fileName))
                                .arg(_fullSizeImage.width())
                                .arg(_fullSizeImage.height())
                                .arg(_fullSizeImage.depth());
    statusBar()->showMessage(message);
    return true;
}

void MainWindow::SetImage(const QImage& new_image, bool fit_to_window) {
    _fullSizeImage = new_image;
    if (_fullSizeImage.colorSpace().isValid()) {
        _fullSizeImage.convertToColorSpace(QColorSpace::SRgb);
    }
    _imageLabel->setPixmap(QPixmap::fromImage(_fullSizeImage));
    _scrollArea->setVisible(true);
    _fitToWindowAct->setEnabled(true);
    _imageLabel->adjustSize();
    if (fit_to_window) {
        FitToWindow();
    } else {
        ScaleImage(_zoom);
    }
}

static void InitializeLoadRawFileDialog(QFileDialog& dialog) {
    QStringList mimeTypeFilters;
    dialog.setNameFilter(dialog.tr("Raw Images (*.ORF *.RAW *.DNG *.NEF *.CR2)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
}

void MainWindow::Open() {
    QFileDialog dialog(this, tr("Open RAW File"));
    InitializeLoadRawFileDialog(dialog);

    while (dialog.exec() == QDialog::Accepted && !LoadRaw(dialog.selectedFiles().constFirst())) {}
}

void MainWindow::ZoomIn() {
    ScaleImage(_zoom * ZOOM_IN_FACTOR);
}

void MainWindow::ZoomOut() {
    ScaleImage(_zoom * ZOOM_OUT_FACTOR);
}

void MainWindow::NormalSize() {
    _imageLabel->adjustSize();
    _zoom = 1.0;
}

void MainWindow::FitToWindow() {

    // Calculate scale factors for both width and height
    double scale_width = static_cast<double>(_scrollArea->viewport()->size().width()) / _fullSizeImage.width();
    double scale_height = static_cast<double>(_scrollArea->viewport()->size().height()) / _fullSizeImage.height();

    // Use the larger scale factor to ensure the image fills the window
    _fit_zoom = std::min(scale_width, scale_height);

    // Apply the scaling
    ScaleImage(_fit_zoom);
}

void MainWindow::CreateActions() {
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* OpenAct = fileMenu->addAction(tr("&Open..."), this, &MainWindow::Open);
    OpenAct->setShortcut(QKeySequence::Open);

    fileMenu->addSeparator();

    QAction* exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcut(tr("Ctrl+Q"));

    // QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));

    _zoomInAct = viewMenu->addAction(tr("Zoom &In (25%)"), this, &MainWindow::ZoomIn);
    _zoomInAct->setShortcut(QKeySequence::ZoomIn);
    // _zoomInAct->setEnabled(false);

    _zoomOutAct = viewMenu->addAction(tr("Zoom &Out (25%)"), this, &MainWindow::ZoomOut);
    _zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    // _zoomOutAct->setEnabled(false);

    _normalSizeAct = viewMenu->addAction(tr("&Normal Size"), this, &MainWindow::NormalSize);
    _normalSizeAct->setShortcut(tr("Ctrl+S"));
    // _normalSizeAct->setEnabled(false);

    viewMenu->addSeparator();

    _fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this, &MainWindow::FitToWindow);
    // _fitToWindowAct->setEnabled(false);
    _fitToWindowAct->setShortcut(tr("Ctrl+F"));
}

void MainWindow::ScaleImage(double requested_zoom) {
    double old_zoom = _zoom;
    _zoom = std::clamp(requested_zoom, _fit_zoom, 1.0);
    _imageLabel->resize(_zoom * _fullSizeImage.size());

    AdjustScrollBar(_scrollArea->horizontalScrollBar(), _zoom / old_zoom);
    AdjustScrollBar(_scrollArea->verticalScrollBar(), _zoom / old_zoom);
}

void MainWindow::AdjustScrollBar(QScrollBar* scroll_bar, double zoom_change) {
    scroll_bar->setValue(int(zoom_change * scroll_bar->value() + ((zoom_change - 1) * scroll_bar->pageStep() / 2)));
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj != _scrollArea->viewport()) {
        return QMainWindow::eventFilter(obj, event);
    }

    switch (event->type()) {
        case QEvent::Wheel:
            HandleWheelEvent(static_cast<QWheelEvent*>(event));
            return true;
        case QEvent::MouseButtonPress:
            HandleMousePressEvent(static_cast<QMouseEvent*>(event));
            return true;
        case QEvent::MouseButtonRelease:
            HandleMouseReleaseEvent(static_cast<QMouseEvent*>(event));
            return true;
        case QEvent::MouseMove:
            HandleMouseMoveEvent(static_cast<QMouseEvent*>(event));
            return true;
        default:
            return QMainWindow::eventFilter(obj, event);
    }
}

void MainWindow::HandleWheelEvent(QWheelEvent* event) {
    QPoint mousePos = event->position().toPoint();
    double zoomFactor = (event->angleDelta().y() > 0) ? ZOOM_IN_FACTOR : ZOOM_OUT_FACTOR;
    double newZoom = _zoom * zoomFactor;

    // Calculate relative position before zoom
    double relX = (mousePos.x() + _scrollArea->horizontalScrollBar()->value()) / (_zoom * _fullSizeImage.width());
    double relY = (mousePos.y() + _scrollArea->verticalScrollBar()->value()) / (_zoom * _fullSizeImage.height());

    ScaleImage(newZoom);

    // Maintain mouse position after zoom
    int newX = static_cast<int>(relX * _zoom * _fullSizeImage.width() - mousePos.x());
    int newY = static_cast<int>(relY * _zoom * _fullSizeImage.height() - mousePos.y());

    _scrollArea->horizontalScrollBar()->setValue(newX);
    _scrollArea->verticalScrollBar()->setValue(newY);
}

void MainWindow::HandleMousePressEvent(QMouseEvent* event) {
    _isDragging = true;
    _lastDragPos = event->pos();
    _scrollArea->viewport()->setCursor(Qt::ClosedHandCursor);
}

void MainWindow::HandleMouseReleaseEvent(QMouseEvent* event) {

    _isDragging = false;
    _scrollArea->viewport()->setCursor(Qt::ArrowCursor);
}

void MainWindow::HandleMouseMoveEvent(QMouseEvent* event) {
    if (!_isDragging || !event) {
        return;
    }

    QPoint delta = event->pos() - _lastDragPos;
    _scrollArea->horizontalScrollBar()->setValue(_scrollArea->horizontalScrollBar()->value() - delta.x());
    _scrollArea->verticalScrollBar()->setValue(_scrollArea->verticalScrollBar()->value() - delta.y());
    _lastDragPos = event->pos();
}

void MainWindow::RefreshImage() {
    if (!_currentRaw) {
        return;
    }

    std::cout << "Generating image with params: " << _parameters.ToString() << std::endl;
    auto processed_image = _pipeline.Run(*_currentRaw, _parameters);
    QImage new_image(processed_image.pixels.data(), processed_image.width, processed_image.height,
                     QImage::Format::Format_RGB888);

    if (!new_image.isNull()) {
        SetImage(new_image, false);
    }
}