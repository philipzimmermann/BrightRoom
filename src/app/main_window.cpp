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
    ScaleImage(_scaleFactor * 1.25);
}

void MainWindow::ZoomOut() {
    ScaleImage(_scaleFactor * 0.8);
}

void MainWindow::NormalSize() {
    _imageLabel->adjustSize();
    _scaleFactor = 1.0;
}

void MainWindow::FitToWindow() {
    // Get the size of the scroll area viewport (the visible area)

    // Calculate scale factors for both width and height
    double scaleWidth =
        static_cast<double>(_scrollArea->viewport()->size().width()) /
        _fullSizeImage.width();
    double scaleHeight =
        static_cast<double>(_scrollArea->viewport()->size().height()) /
        _fullSizeImage.height();
    std::cout << "Scale width: " << scaleWidth << std::endl;
    std::cout << "Scale height: " << scaleHeight << std::endl;

    // Use the larger scale factor to ensure the image fills the window
    double scale = std::min(scaleWidth, scaleHeight);

    // Apply the scaling
    ScaleImage(scale);
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

void MainWindow::ScaleImage(double scale) {
    _scaleFactor = std::clamp(scale, 0.1, 1.0);
    _imageLabel->resize(_scaleFactor * _fullSizeImage.size());

    AdjustScrollBar(_scrollArea->horizontalScrollBar(), scale);
    AdjustScrollBar(_scrollArea->verticalScrollBar(), scale);
}

void MainWindow::AdjustScrollBar(QScrollBar* scrollBar, double scale) {
    scrollBar->setValue(int(scale * scrollBar->value() +
                            ((scale - 1) * scrollBar->pageStep() / 2)));
}
