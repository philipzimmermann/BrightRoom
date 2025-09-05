#pragma once

#include <libraw/libraw.h>
#include <string>
#include "types.h"

namespace brightroom {

struct Parameters {
    float exposure = 0.0f;
    float contrast = 0.0f;
    float saturation = 0.0f;

    auto ToString() const -> std::string {
        return "Exposure: " + std::to_string(exposure) + ", Contrast: " + std::to_string(contrast) +
               ", Saturation: " + std::to_string(saturation);
    }
};

class IRawPipeline {
   public:
    virtual void Preprocess(LibRaw& raw_data) = 0;
    virtual auto Process(LibRaw& raw_data, const Parameters& parameters) -> RgbImage = 0;
    virtual auto GetHistogram() -> Histogram = 0;
    virtual ~IRawPipeline() = default;
};

}  // namespace brightroom