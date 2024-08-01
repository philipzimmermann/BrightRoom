#include "main_window.h"
#include <qimage.h>

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
#include <iostream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), imageLabel(new QLabel), scrollArea(new QScrollArea) {
  imageLabel->setBackgroundRole(QPalette::Base);
  imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  imageLabel->setScaledContents(true);

  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidget(imageLabel);
  scrollArea->setVisible(false);
  setCentralWidget(scrollArea);

  createActions();

  resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

bool MainWindow::loadImage(const QString& fileName) {
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
  //! [2]
  setImage(newImage);
  setWindowFilePath(fileName);
  const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
                              .arg(QDir::toNativeSeparators(fileName))
                              .arg(image.width())
                              .arg(image.height())
                              .arg(image.depth());
  statusBar()->showMessage(message);
  return true;
}

bool MainWindow::loadRaw(const QString& fileName) {
  std::cout << "0" << std::endl;
  raw::RawLoader loader{};
  std::cout << "01" << std::endl;

  auto raw_file = loader.LoadRaw(fileName.toStdString());
  std::cout << "1" << std::endl;

  const auto& thumbnail = raw_file.thumbnail;
  std::cout << "2" << std::endl;

  const QImage newImage(thumbnail.pixels, thumbnail.width, thumbnail.height,
                        QImage::Format::Format_RGB888);
  std::cout << "3" << std::endl;
  if (newImage.isNull()) {
    QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                             tr("Cannot load %1: %2"));
    return false;
  }
  //! [2]
  setImage(newImage);
  setWindowFilePath(fileName);
  const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
                              .arg(QDir::toNativeSeparators(fileName))
                              .arg(image.width())
                              .arg(image.height())
                              .arg(image.depth());
  statusBar()->showMessage(message);
  return true;
}

void MainWindow::setImage(const QImage& newImage) {
  image = newImage;
  if (image.colorSpace().isValid())
    image.convertToColorSpace(QColorSpace::SRgb);
  imageLabel->setPixmap(QPixmap::fromImage(image));
  //! [4]
  scaleFactor = 1.0;

  scrollArea->setVisible(true);
  printAct->setEnabled(true);
  fitToWindowAct->setEnabled(true);
  updateActions();

  if (!fitToWindowAct->isChecked())
    imageLabel->adjustSize();
}

//! [4]

bool MainWindow::saveFile(const QString& fileName) {
  QImageWriter writer(fileName);

  if (!writer.write(image)) {
    QMessageBox::information(
        this, QGuiApplication::applicationDisplayName(),
        tr("Cannot write %1: %2").arg(QDir::toNativeSeparators(fileName)),
        writer.errorString());
    return false;
  }
  const QString message =
      tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
  statusBar()->showMessage(message);
  return true;
}

//! [1]

static void initializeImageFileDialog(QFileDialog& dialog,
                                      QFileDialog::AcceptMode acceptMode) {
  static bool firstDialog = true;

  if (firstDialog) {
    firstDialog = false;
    const QStringList picturesLocations =
        QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath()
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
  dialog.setNameFilter(dialog.tr("Raw Images (*.ORF *.RAW)"));
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
}

void MainWindow::open() {
  QFileDialog dialog(this, tr("Open RAW File"));
  initializeLoadRawFileDialog(dialog);

  while (dialog.exec() == QDialog::Accepted &&
         !loadRaw(dialog.selectedFiles().constFirst())) {}
}
//! [1]

void MainWindow::saveAs() {
  QFileDialog dialog(this, tr("Save File As"));
  initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

  while (dialog.exec() == QDialog::Accepted &&
         !saveFile(dialog.selectedFiles().constFirst())) {}
}

//! [5]
void MainWindow::print()
//! [5] //! [6]
{
  Q_ASSERT(!imageLabel->pixmap(Qt::ReturnByValue).isNull());
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
  //! [6] //! [7]
  QPrintDialog dialog(&printer, this);
  //! [7] //! [8]
  if (dialog.exec()) {
    QPainter painter(&printer);
    QPixmap pixmap = imageLabel->pixmap(Qt::ReturnByValue);
    QRect rect = painter.viewport();
    QSize size = pixmap.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
    painter.setWindow(pixmap.rect());
    painter.drawPixmap(0, 0, pixmap);
  }
#endif
}
//! [8]

void MainWindow::copy() {
#ifndef QT_NO_CLIPBOARD
  QGuiApplication::clipboard()->setImage(image);
#endif  // !QT_NO_CLIPBOARD
}

#ifndef QT_NO_CLIPBOARD
static QImage clipboardImage() {
  if (const QMimeData* mimeData = QGuiApplication::clipboard()->mimeData()) {
    if (mimeData->hasImage()) {
      const QImage image = qvariant_cast<QImage>(mimeData->imageData());
      if (!image.isNull())
        return image;
    }
  }
  return QImage();
}
#endif  // !QT_NO_CLIPBOARD

void MainWindow::paste() {
#ifndef QT_NO_CLIPBOARD
  const QImage newImage = clipboardImage();
  if (newImage.isNull()) {
    statusBar()->showMessage(tr("No image in clipboard"));
  } else {
    setImage(newImage);
    setWindowFilePath(QString());
    const QString message =
        tr("Obtained image from clipboard, %1x%2, Depth: %3")
            .arg(newImage.width())
            .arg(newImage.height())
            .arg(newImage.depth());
    statusBar()->showMessage(message);
  }
#endif  // !QT_NO_CLIPBOARD
}

//! [9]
void MainWindow::zoomIn()
//! [9] //! [10]
{
  scaleImage(1.25);
}

void MainWindow::zoomOut() {
  scaleImage(0.8);
}

//! [10] //! [11]
void MainWindow::normalSize()
//! [11] //! [12]
{
  imageLabel->adjustSize();
  scaleFactor = 1.0;
}
//! [12]

//! [13]
void MainWindow::fitToWindow()
//! [13] //! [14]
{
  bool fitToWindow = fitToWindowAct->isChecked();
  scrollArea->setWidgetResizable(fitToWindow);
  if (!fitToWindow)
    normalSize();
  updateActions();
}
//! [14]

//! [15]
void MainWindow::about()
//! [15] //! [16]
{
  QMessageBox::about(
      this, tr("About Image Viewer"),
      tr("<p>The <b>Image Viewer</b> example shows how to combine QLabel "
         "and QScrollArea to display an image. QLabel is typically used "
         "for displaying a text, but it can also display an image. "
         "QScrollArea provides a scrolling view around another widget. "
         "If the child widget exceeds the size of the frame, QScrollArea "
         "automatically provides scroll bars. </p><p>The example "
         "demonstrates how QLabel's ability to scale its contents "
         "(QLabel::scaledContents), and QScrollArea's ability to "
         "automatically resize its contents "
         "(QScrollArea::widgetResizable), can be used to implement "
         "zooming and scaling features. </p><p>In addition the example "
         "shows how to use QPainter to print an image.</p>"));
}
//! [16]

//! [17]
void MainWindow::createActions()
//! [17] //! [18]
{
  QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

  QAction* openAct =
      fileMenu->addAction(tr("&Open..."), this, &MainWindow::open);
  openAct->setShortcut(QKeySequence::Open);

  saveAsAct = fileMenu->addAction(tr("&Save As..."), this, &MainWindow::saveAs);
  saveAsAct->setEnabled(false);

  printAct = fileMenu->addAction(tr("&Print..."), this, &MainWindow::print);
  printAct->setShortcut(QKeySequence::Print);
  printAct->setEnabled(false);

  fileMenu->addSeparator();

  QAction* exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
  exitAct->setShortcut(tr("Ctrl+Q"));

  QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));

  copyAct = editMenu->addAction(tr("&Copy"), this, &MainWindow::copy);
  copyAct->setShortcut(QKeySequence::Copy);
  copyAct->setEnabled(false);

  QAction* pasteAct =
      editMenu->addAction(tr("&Paste"), this, &MainWindow::paste);
  pasteAct->setShortcut(QKeySequence::Paste);

  QMenu* viewMenu = menuBar()->addMenu(tr("&View"));

  zoomInAct =
      viewMenu->addAction(tr("Zoom &In (25%)"), this, &MainWindow::zoomIn);
  zoomInAct->setShortcut(QKeySequence::ZoomIn);
  zoomInAct->setEnabled(false);

  zoomOutAct =
      viewMenu->addAction(tr("Zoom &Out (25%)"), this, &MainWindow::zoomOut);
  zoomOutAct->setShortcut(QKeySequence::ZoomOut);
  zoomOutAct->setEnabled(false);

  normalSizeAct =
      viewMenu->addAction(tr("&Normal Size"), this, &MainWindow::normalSize);
  normalSizeAct->setShortcut(tr("Ctrl+S"));
  normalSizeAct->setEnabled(false);

  viewMenu->addSeparator();

  fitToWindowAct =
      viewMenu->addAction(tr("&Fit to Window"), this, &MainWindow::fitToWindow);
  fitToWindowAct->setEnabled(false);
  fitToWindowAct->setCheckable(true);
  fitToWindowAct->setShortcut(tr("Ctrl+F"));

  QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

  helpMenu->addAction(tr("&About"), this, &MainWindow::about);
  helpMenu->addAction(tr("About &Qt"), this, &QApplication::aboutQt);
}
//! [18]

//! [21]
void MainWindow::updateActions()
//! [21] //! [22]
{
  saveAsAct->setEnabled(!image.isNull());
  copyAct->setEnabled(!image.isNull());
  zoomInAct->setEnabled(!fitToWindowAct->isChecked());
  zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
  normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}
//! [22]

//! [23]
void MainWindow::scaleImage(double factor)
//! [23] //! [24]
{
  scaleFactor *= factor;
  imageLabel->resize(scaleFactor *
                     imageLabel->pixmap(Qt::ReturnByValue).size());

  adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
  adjustScrollBar(scrollArea->verticalScrollBar(), factor);

  zoomInAct->setEnabled(scaleFactor < 3.0);
  zoomOutAct->setEnabled(scaleFactor > 0.333);
}
//! [24]

//! [25]
void MainWindow::adjustScrollBar(QScrollBar* scrollBar, double factor)
//! [25] //! [26]
{
  scrollBar->setValue(int(factor * scrollBar->value() +
                          ((factor - 1) * scrollBar->pageStep() / 2)));
}
//! [26]
