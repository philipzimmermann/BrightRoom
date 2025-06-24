#pragma once

#include <libraw/libraw.h>
#include "types.h"
namespace raw {

class Pipeline {
   public:
    Pipeline(){};
    RgbImage Run(LibRaw& input) const;

   private:
};
}  // namespace raw