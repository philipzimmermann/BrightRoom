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
    LoadRaw("/media/philip/Data SSD/photos/2025/06/22/QI9B7671.CR2");
}

bool MainWindow::LoadImage(const QString& fileName) {
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(
            this, QGuiApplication::applicationDisplayName(),
            tr("Cannot load %1: %2")
                .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }
    SetImage(newImage);
    setWindowFilePath(fileName);
    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
                                .arg(QDir::toNativeSeparators(fileName))
                                .arg(_image.width())
                                .arg(_image.height())
                                .arg(_image.depth());
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
    QImage newImage(processed_image.pixels.data(), processed_image.width,
                    processed_image.height, QImage::Format::Format_RGB888);
    if (newImage.isNull()) {
        QMessageBox::information(this,
                                 QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2"));
        return false;
    }
    SetImage(newImage);

    setWindowFilePath(fileName);
    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
                                .arg(QDir::toNativeSeparators(fileName))
                                .arg(_image.width())
                                .arg(_image.height())
                                .arg(_image.depth());
    statusBar()->showMessage(message);
    return true;
}

void MainWindow::SetImage(const QImage& newImage) {
    _image = newImage;
    if (_image.colorSpace().isValid())
        _image.convertToColorSpace(QColorSpace::SRgb);
    _imageLabel->setPixmap(QPixmap::fromImage(_image));
    _scaleFactor = 1.0;

    _scrollArea->setVisible(true);
    _fitToWindowAct->setEnabled(true);
    UpdateActions();

    if (!_fitToWindowAct->isChecked())
        _imageLabel->adjustSize();
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
    ScaleImage(1.25);
}

void MainWindow::ZoomOut() {
    ScaleImage(0.8);
}

void MainWindow::NormalSize() {
    _imageLabel->adjustSize();
    _scaleFactor = 1.0;
}

void MainWindow::FitToWindow() {
    _scaleFactor =
        _image.width() / _imageLabel->pixmap(Qt::ReturnByValue).width();
    ScaleImage(_scaleFactor);
}

void MainWindow::CreateActions() {
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* OpenAct =
        fileMenu->addAction(tr("&Open..."), this, &MainWindow::Open);
    OpenAct->setShortcut(QKeySequence::Open);

    // printAct = fileMenu->addAction(tr("&Print..."), this, &MainWindow::Print);
    // printAct->setShortcut(QKeySequence::Print);
    // printAct->setEnabled(false);

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

void MainWindow::ScaleImage(double factor) {
    _scaleFactor *= factor;
    _imageLabel->resize(_scaleFactor *
                        _imageLabel->pixmap(Qt::ReturnByValue).size());

    AdjustScrollBar(_scrollArea->horizontalScrollBar(), factor);
    AdjustScrollBar(_scrollArea->verticalScrollBar(), factor);

    _zoomInAct->setEnabled(_scaleFactor < 3.0);
    _zoomOutAct->setEnabled(_scaleFactor > 0.333);
}

void MainWindow::AdjustScrollBar(QScrollBar* scrollBar, double factor) {
    scrollBar->setValue(int(factor * scrollBar->value() +
                            ((factor - 1) * scrollBar->pageStep() / 2)));
}
