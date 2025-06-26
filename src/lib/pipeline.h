#pragma once

#include <libraw/libraw.h>
#include "types.h"
namespace raw {

class Pipeline {
   public:
    Pipeline() = default;
    auto Run(LibRaw& rawProcessor) const -> RgbImage;

   private:
};
}  // namespace raw