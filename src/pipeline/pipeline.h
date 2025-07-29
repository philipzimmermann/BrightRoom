#pragma once

#include <libraw/libraw.h>
#include "types.h"
namespace raw {

struct Parameters {
    float exposure = 1.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;

    auto ToString() const -> std::string {
        return "Exposure: " + std::to_string(exposure) + ", Contrast: " + std::to_string(contrast) +
               ", Saturation: " + std::to_string(saturation);
    }
};

class Pipeline {
   public:
    Pipeline() = default;
    auto Run(LibRaw& rawProcessor, const Parameters& parameters) const -> RgbImage;

   private:
};
}  // namespace raw