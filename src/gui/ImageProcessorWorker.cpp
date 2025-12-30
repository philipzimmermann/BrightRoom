#include "ImageProcessorWorker.h"
#include <iostream>
#include "RawLoader.h"

ImageProcessorWorker::ImageProcessorWorker(std::unique_ptr<brightroom::IRawPipeline> pipeline, QObject* parent)
    : QObject(parent), _pipeline(std::move(pipeline)) {}

ImageProcessorWorker::~ImageProcessorWorker() {}

void ImageProcessorWorker::LoadRaw(const QString& fileName, const brightroom::Parameters& parameters) {
    _raw_data = _raw_loader.LoadRaw(fileName.toStdString());
    std::cout << "Worker thread: Loaded raw data with width: " << _raw_data->imgdata.sizes.width
              << " and height: " << _raw_data->imgdata.sizes.height << std::endl;
    _pipeline->Preprocess(*_raw_data);
    std::cout << "Worker thread: Preprocessed raw data" << std::endl;
    ProcessImage(parameters);
}

void ImageProcessorWorker::ProcessImage(const brightroom::Parameters& parameters) {

    std::cout << "Worker thread: Generating image with params: " << parameters.ToString() << std::endl;

    // Process the image
    auto processed_image = _pipeline->Process(*_raw_data, parameters);

    // Get histogram
    auto histogram = _pipeline->GetHistogram();

    emit ImageProcessed(std::move(processed_image), histogram);
}
