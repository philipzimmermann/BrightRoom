#pragma once

#include <libraw/libraw.h>
#include <QMutex>
#include <QObject>
#include "IRawPipeline.h"
#include "RawLoader.h"

class ImageProcessorWorker : public QObject {
    Q_OBJECT

   public:
    explicit ImageProcessorWorker(std::unique_ptr<brightroom::IRawPipeline> pipeline, QObject* parent = nullptr);
    ~ImageProcessorWorker();

   public slots:
    void LoadRaw(const QString& fileName, const brightroom::Parameters& parameters);
    void ProcessImage(const brightroom::Parameters& parameters);

   signals:
    void ImageProcessed(brightroom::RgbImage image, brightroom::Histogram histogram);
    void ProcessingFailed(const QString& error);

   private:
    QMutex _mutex;
    std::unique_ptr<brightroom::IRawPipeline> _pipeline;
    std::unique_ptr<LibRaw> _raw_data;
    brightroom::RawLoader _raw_loader{};
};

