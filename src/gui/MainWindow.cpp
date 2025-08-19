#include "MainWindow.h"
#include <qimage.h>
#include <iostream>
#include "RawLoader.h"

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
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent, std::unique_ptr<brightroom::IRawPipeline> pipeline)
    : QMainWindow(parent), _imageLabel(new QLabel), _scrollArea(new QScrollArea), _pipeline(std::move(pipeline)) {
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
    _refreshTimer->setInterval(kDebounceDelayMs);
    connect(_refreshTimer, &QTimer::timeout, this, &MainWindow::RefreshImage);

    CreateEditDock();
    CreateActions();

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
    // QTimer::singleShot(0, this, [this]() { LoadRaw("/media/philip/Data SSD/photos/2025/06/22/QI9B7671.CR2"); });
}

auto MainWindow::CreateAdjustmentSlider(QWidget* parent, const QString& label, QVBoxLayout* layout) -> MySlider* {
    auto* slider_label = new QLabel(label, parent);
    auto* slider = new MySlider(Qt::Horizontal, parent);
    constexpr int kSliderRange = 100;
    slider->setRange(-kSliderRange, kSliderRange);
    slider->setValue(0);
    slider->setTickPosition(QSlider::TicksBelow);
    slider->setTickInterval(kSliderTickInterval);

    layout->addWidget(slider_label);
    layout->addWidget(slider);

    return slider;
}

void MainWindow::CreateEditDock() {
    // Rename dock
    _editDock = new QDockWidget(tr("Edit"), this);
    _editDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    auto* dockWidget = new QWidget(_editDock);
    auto* dockLayout = new QVBoxLayout(dockWidget);
    dockWidget->setMinimumWidth(200);

    // Button row for layout switching
    auto* buttonLayout = new QHBoxLayout();
    auto* adjustmentsBtn = new QPushButton(tr("Adjustments"), dockWidget);
    auto* metadataBtn = new QPushButton(tr("Metadata"), dockWidget);
    auto* cropBtn = new QPushButton(tr("Crop"), dockWidget);

    buttonLayout->addWidget(adjustmentsBtn);
    buttonLayout->addWidget(cropBtn);
    buttonLayout->addWidget(metadataBtn);

    dockLayout->addLayout(buttonLayout);

    // Stacked widget to hold different layouts
    auto* stackedWidget = new QStackedWidget(dockWidget);

    // --- Adjustments layout ---
    auto* adjustmentsWidget = new QWidget(stackedWidget);
    auto* adjustmentsLayout = new QVBoxLayout(adjustmentsWidget);

    _exposureSlider = CreateAdjustmentSlider(adjustmentsWidget, tr("Exposure"), adjustmentsLayout);
    _contrastSlider = CreateAdjustmentSlider(adjustmentsWidget, tr("Contrast"), adjustmentsLayout);
    _saturationSlider = CreateAdjustmentSlider(adjustmentsWidget, tr("Saturation"), adjustmentsLayout);

    adjustmentsLayout->addStretch();
    adjustmentsWidget->setLayout(adjustmentsLayout);
    stackedWidget->addWidget(adjustmentsWidget);

    // --- Metadata layout (placeholder) ---
    auto* metadataWidget = new QWidget(stackedWidget);
    auto* metadataLayout = new QVBoxLayout(metadataWidget);
    metadataLayout->addWidget(new QLabel(tr("Metadata content goes here"), metadataWidget));
    metadataWidget->setLayout(metadataLayout);
    stackedWidget->addWidget(metadataWidget);

    // --- Crop layout (placeholder) ---
    auto* cropWidget = new QWidget(stackedWidget);
    auto* cropLayout = new QVBoxLayout(cropWidget);
    cropLayout->addWidget(new QLabel(tr("Crop tools go here"), cropWidget));
    cropWidget->setLayout(cropLayout);
    stackedWidget->addWidget(cropWidget);

    // Add stacked widget to dock layout
    dockLayout->addWidget(stackedWidget);
    dockWidget->setLayout(dockLayout);
    _editDock->setWidget(dockWidget);
    addDockWidget(Qt::RightDockWidgetArea, _editDock);

    // Button connections
    connect(adjustmentsBtn, &QPushButton::clicked, [stackedWidget]() { stackedWidget->setCurrentIndex(0); });
    connect(metadataBtn, &QPushButton::clicked, [stackedWidget]() { stackedWidget->setCurrentIndex(1); });
    connect(cropBtn, &QPushButton::clicked, [stackedWidget]() { stackedWidget->setCurrentIndex(2); });

    // Connect sliders (same as before)
    ConnectSlider(_exposureSlider,
                  [this](float value) { _parameters.exposure = std::pow(2.0f, value / kSliderTickInterval); });
    ConnectSlider(_contrastSlider,
                  [this](float value) { _parameters.contrast = std::pow(1.5f, value / kSliderTickInterval); });
    ConnectSlider(_saturationSlider,
                  [this](float value) { _parameters.saturation = std::pow(2.0f, value / kSliderTickInterval); });
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
    brightroom::RawLoader loader{};
    _currentRaw = loader.LoadRaw(fileName.toStdString());

    _pipeline->Preprocess(*_currentRaw);
    auto processed_image = _pipeline->Process(*_currentRaw, _parameters);
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
    QStringList mime_type_filters;
    dialog.setNameFilter(QFileDialog::tr("Raw Images (*.ORF *.RAW *.DNG *.NEF *.CR2)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
}

void MainWindow::Open() {
    QFileDialog dialog(this, tr("Open RAW File"));
    InitializeLoadRawFileDialog(dialog);

    while (dialog.exec() == QDialog::Accepted && !LoadRaw(dialog.selectedFiles().constFirst())) {}
}

void MainWindow::ZoomIn() {
    ScaleImage(_zoom * kZoomInFactor);
}

void MainWindow::ZoomOut() {
    ScaleImage(_zoom * kZoomOutFactor);
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
    QMenu* file_menu = menuBar()->addMenu(tr("&File"));

    QAction* open_act = file_menu->addAction(tr("&Open..."), this, &MainWindow::Open);
    open_act->setShortcut(QKeySequence::Open);

    file_menu->addSeparator();

    QAction* exit_act = file_menu->addAction(tr("E&xit"), this, &QWidget::close);
    exit_act->setShortcut(tr("Ctrl+Q"));

    // QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));

    QMenu* view_menu = menuBar()->addMenu(tr("&View"));

    _zoomInAct = view_menu->addAction(tr("Zoom &In (25%)"), this, &MainWindow::ZoomIn);
    _zoomInAct->setShortcut(QKeySequence::ZoomIn);
    // _zoomInAct->setEnabled(false);

    _zoomOutAct = view_menu->addAction(tr("Zoom &Out (25%)"), this, &MainWindow::ZoomOut);
    _zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    // _zoomOutAct->setEnabled(false);

    _normalSizeAct = view_menu->addAction(tr("&Normal Size"), this, &MainWindow::NormalSize);
    _normalSizeAct->setShortcut(tr("Ctrl+S"));
    // _normalSizeAct->setEnabled(false);

    view_menu->addSeparator();

    _fitToWindowAct = view_menu->addAction(tr("&Fit to Window"), this, &MainWindow::FitToWindow);
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
    QPoint mouse_pos = event->position().toPoint();
    double zoom_factor = (event->angleDelta().y() > 0) ? kZoomInFactor : kZoomOutFactor;
    double new_zoom = _zoom * zoom_factor;

    // Calculate relative position before zoom
    double rel_x = (mouse_pos.x() + _scrollArea->horizontalScrollBar()->value()) / (_zoom * _fullSizeImage.width());
    double rel_y = (mouse_pos.y() + _scrollArea->verticalScrollBar()->value()) / (_zoom * _fullSizeImage.height());

    ScaleImage(new_zoom);

    // Maintain mouse position after zoom
    int new_x = static_cast<int>(rel_x * _zoom * _fullSizeImage.width() - mouse_pos.x());
    int new_y = static_cast<int>(rel_y * _zoom * _fullSizeImage.height() - mouse_pos.y());

    _scrollArea->horizontalScrollBar()->setValue(new_x);
    _scrollArea->verticalScrollBar()->setValue(new_y);
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
    // TODO: This should run in a separate worker thread
    if (!_currentRaw) {
        return;
    }

    // std::cout << "Sleeping 1000ms\n";
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "Generating image with params: " << _parameters.ToString() << std::endl;
    auto processed_image = _pipeline->Process(*_currentRaw, _parameters);
    QImage new_image(processed_image.pixels.data(), processed_image.width, processed_image.height,
                     QImage::Format::Format_RGB888);

    if (!new_image.isNull()) {
        SetImage(new_image, false);
    }
}