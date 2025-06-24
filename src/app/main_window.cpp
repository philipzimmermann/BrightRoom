#include "main_window.h"
#include <qimage.h>

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
#include <iostream>

#include "Tracy.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      _imageLabel(new QLabel),
      _scrollArea(new QScrollArea) {
    _imageLabel->setBackgroundRole(QPalette::Base);
    _imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    _imageLabel->setScaledContents(true);

    _scrollArea->setBackgroundRole(QPalette::Dark);
    _scrollArea->setWidget(_imageLabel);
    _scrollArea->setVisible(false);
    _scrollArea->viewport()->installEventFilter(this);
    setCentralWidget(_scrollArea);

    CreateActions();

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
    QTimer::singleShot(0, this, [this]() {
        LoadRaw("/media/philip/Data SSD/photos/2025/06/22/QI9B7671.CR2");
    });
}

bool MainWindow::LoadImage(const QString& fileName) {
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage new_image = reader.read();
    if (new_image.isNull()) {
        QMessageBox::information(
            this, QGuiApplication::applicationDisplayName(),
            tr("Cannot load %1: %2")
                .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }
    SetImage(new_image);
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
    auto raw_file = loader.LoadRaw(fileName.toStdString());
    const auto& thumbnail = raw_file.thumbnail;

    // const QImage newImage(thumbnail.pixels.data(), thumbnail.width,
    //                       thumbnail.height, QImage::Format::Format_RGB888);

    const raw::Pipeline pipeline{};
    auto processed_image = pipeline.Run(raw_file);
    QImage new_image(processed_image.pixels.data(), processed_image.width,
                     processed_image.height, QImage::Format::Format_RGB888);
    if (new_image.isNull()) {
        QMessageBox::information(this,
                                 QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2"));
        return false;
    }
    SetImage(new_image);

    setWindowFilePath(fileName);
    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
                                .arg(QDir::toNativeSeparators(fileName))
                                .arg(_fullSizeImage.width())
                                .arg(_fullSizeImage.height())
                                .arg(_fullSizeImage.depth());
    statusBar()->showMessage(message);
    return true;
}

void MainWindow::SetImage(const QImage& new_image) {
    _fullSizeImage = new_image;
    if (_fullSizeImage.colorSpace().isValid()) {
        _fullSizeImage.convertToColorSpace(QColorSpace::SRgb);
    }
    _imageLabel->setPixmap(QPixmap::fromImage(_fullSizeImage));
    _scrollArea->setVisible(true);
    _fitToWindowAct->setEnabled(true);
    UpdateActions();
    _imageLabel->adjustSize();
    FitToWindow();
}

static void initializeImageFileDialog(QFileDialog& dialog,
                                      QFileDialog::AcceptMode acceptMode) {
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations =
            QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty()
                                ? QDir::currentPath()
                                : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes =
        acceptMode == QFileDialog::AcceptOpen
            ? QImageReader::supportedMimeTypes()
            : QImageWriter::supportedMimeTypes();
    for (const QByteArray& mimeTypeName : supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    dialog.setAcceptMode(acceptMode);
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

static void initializeLoadRawFileDialog(QFileDialog& dialog) {
    QStringList mimeTypeFilters;
    dialog.setNameFilter(
        dialog.tr("Raw Images (*.ORF *.RAW *.DNG *.NEF *.CR2)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
}

void MainWindow::Open() {
    QFileDialog dialog(this, tr("Open RAW File"));
    initializeLoadRawFileDialog(dialog);

    while (dialog.exec() == QDialog::Accepted &&
           !LoadRaw(dialog.selectedFiles().constFirst())) {}
}

void MainWindow::ZoomIn() {
    ScaleImage(_zoom * 1.25);
}

void MainWindow::ZoomOut() {
    ScaleImage(_zoom * 0.8);
}

void MainWindow::NormalSize() {
    _imageLabel->adjustSize();
    _zoom = 1.0;
}

void MainWindow::FitToWindow() {

    // Calculate scale factors for both width and height
    double scale_width =
        static_cast<double>(_scrollArea->viewport()->size().width()) /
        _fullSizeImage.width();
    double scale_height =
        static_cast<double>(_scrollArea->viewport()->size().height()) /
        _fullSizeImage.height();

    // Use the larger scale factor to ensure the image fills the window
    _fit_zoom = std::min(scale_width, scale_height);

    // Apply the scaling
    ScaleImage(_fit_zoom);
}

void MainWindow::CreateActions() {
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* OpenAct =
        fileMenu->addAction(tr("&Open..."), this, &MainWindow::Open);
    OpenAct->setShortcut(QKeySequence::Open);

    fileMenu->addSeparator();

    QAction* exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcut(tr("Ctrl+Q"));

    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));

    _zoomInAct =
        viewMenu->addAction(tr("Zoom &In (25%)"), this, &MainWindow::ZoomIn);
    _zoomInAct->setShortcut(QKeySequence::ZoomIn);
    _zoomInAct->setEnabled(false);

    _zoomOutAct =
        viewMenu->addAction(tr("Zoom &Out (25%)"), this, &MainWindow::ZoomOut);
    _zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    _zoomOutAct->setEnabled(false);

    _normalSizeAct =
        viewMenu->addAction(tr("&Normal Size"), this, &MainWindow::NormalSize);
    _normalSizeAct->setShortcut(tr("Ctrl+S"));
    _normalSizeAct->setEnabled(false);

    viewMenu->addSeparator();

    _fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this,
                                          &MainWindow::FitToWindow);
    _fitToWindowAct->setEnabled(false);
    _fitToWindowAct->setCheckable(true);
    _fitToWindowAct->setShortcut(tr("Ctrl+F"));
}

void MainWindow::UpdateActions() {
    _zoomInAct->setEnabled(!_fitToWindowAct->isChecked());
    _zoomOutAct->setEnabled(!_fitToWindowAct->isChecked());
    _normalSizeAct->setEnabled(!_fitToWindowAct->isChecked());
}

void MainWindow::ScaleImage(double requested_zoom) {
    double old_zoom = _zoom;
    _zoom = std::clamp(requested_zoom, _fit_zoom, 1.0);
    _imageLabel->resize(_zoom * _fullSizeImage.size());

    AdjustScrollBar(_scrollArea->horizontalScrollBar(), _zoom / old_zoom);
    AdjustScrollBar(_scrollArea->verticalScrollBar(), _zoom / old_zoom);
}

void MainWindow::AdjustScrollBar(QScrollBar* scroll_bar, double zoom_change) {
    scroll_bar->setValue(int(zoom_change * scroll_bar->value() +
                             ((zoom_change - 1) * scroll_bar->pageStep() / 2)));
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == _scrollArea->viewport()) {
        switch (event->type()) {
            case QEvent::Wheel: {
                if (!_fitToWindowAct
                         ->isChecked()) {  // Only zoom if not in fit-to-window mode
                    auto* wheel_event = static_cast<QWheelEvent*>(event);
                    if (wheel_event != nullptr) {
                        // Get the mouse position relative to the image
                        QPoint mouse_pos = wheel_event->position().toPoint();

                        // Calculate zoom factor based on wheel delta
                        // Typical mouse wheel step is 120 units
                        double zoom_factor =
                            (wheel_event->angleDelta().y() > 0) ? 1.25 : 0.8;
                        double new_zoom = _zoom * zoom_factor;

                        // Store the mouse position relative to the content before zooming
                        double rel_x =
                            (mouse_pos.x() +
                             _scrollArea->horizontalScrollBar()->value()) /
                            (_zoom * _fullSizeImage.width());
                        double rel_y =
                            (mouse_pos.y() +
                             _scrollArea->verticalScrollBar()->value()) /
                            (_zoom * _fullSizeImage.height());

                        // Apply the zoom
                        ScaleImage(new_zoom);

                        // Calculate and set the new scroll position to keep the mouse position fixed
                        int new_x = static_cast<int>(
                            rel_x * _zoom * _fullSizeImage.width() -
                            mouse_pos.x());
                        int new_y = static_cast<int>(
                            rel_y * _zoom * _fullSizeImage.height() -
                            mouse_pos.y());

                        _scrollArea->horizontalScrollBar()->setValue(new_x);
                        _scrollArea->verticalScrollBar()->setValue(new_y);

                        return true;
                    }
                }
                break;
            }
            case QEvent::MouseButtonPress: {
                auto* mouse_event = static_cast<QMouseEvent*>(event);
                if ((mouse_event != nullptr) &&
                    mouse_event->button() == Qt::LeftButton) {
                    _isDragging = true;
                    _lastDragPos = mouse_event->pos();
                    _scrollArea->viewport()->setCursor(Qt::ClosedHandCursor);
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                auto* mouse_event = static_cast<QMouseEvent*>(event);
                if (mouse_event != nullptr &&
                    mouse_event->button() == Qt::LeftButton) {
                    _isDragging = false;
                    _scrollArea->viewport()->setCursor(Qt::ArrowCursor);
                    return true;
                }
                break;
            }
            case QEvent::MouseMove: {
                if (_isDragging) {
                    auto* mouse_event = static_cast<QMouseEvent*>(event);
                    if (mouse_event != nullptr) {
                        QPoint delta = mouse_event->pos() - _lastDragPos;
                        _scrollArea->horizontalScrollBar()->setValue(
                            _scrollArea->horizontalScrollBar()->value() -
                            delta.x());
                        _scrollArea->verticalScrollBar()->setValue(
                            _scrollArea->verticalScrollBar()->value() -
                            delta.y());
                        _lastDragPos = mouse_event->pos();
                        return true;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}