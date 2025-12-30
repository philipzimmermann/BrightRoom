#pragma once

#include <HalideBuffer.h>
#include <libraw/libraw.h>
#include "IRawPipeline.h"
#include "types.h"

namespace brightroom {

class HalideRawPipeline : public IRawPipeline {
   public:
    HalideRawPipeline() = default;
    void Preprocess(LibRaw& raw_data) override;
    auto Process(LibRaw& raw_data, const Parameters& parameters) -> RgbImage override;
    auto GetHistogram() -> Histogram override;

   private:
    Halide::Runtime::Buffer<float> _preprocessed_buffer;
    std::vector<uint8_t> _rgb8_vector;
    std::vector<uint8_t> _downsampled_vector;
    Halide::Runtime::Buffer<uint8_t> _rgb8_buffer;
    Halide::Runtime::Buffer<uint8_t> _downsampled_buffer;
    Halide::Runtime::Buffer<uint32_t> _histogram_buffer = Halide::Runtime::Buffer<uint32_t>(kHistogramBins);
    Histogram _histogram;
    int _downsample_factor = 8;
    int _width = 0;
    int _height = 0;
};
}  // namespace brightroom