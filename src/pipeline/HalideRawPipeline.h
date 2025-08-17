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

   private:
    Halide::Runtime::Buffer<float> _demosaiced_buffer;
};
}  // namespace brightroom